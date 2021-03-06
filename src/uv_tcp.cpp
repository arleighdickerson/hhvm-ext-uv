#include "uv_tcp.h"

namespace HPHP {
    UVTcpData::~UVTcpData(){
        sweep();
    }
    
    void UVTcpData::sweep()
    {
        connectCallback.unset();
        readCallback.unset();
        writeCallback.unset();
        errorCallback.unset();
        shutdownCallback.unset();

        if(releaseHook){
            releaseHook(this);
        }

        release();

        if(gcHook){
            gcHook(this);
        }

        if(tcp_handle){
            delete tcp_handle;
            tcp_handle = NULL;
        }
    }
    
    void UVTcpData::release()
    {
        if(tcp_handle){
            releaseHandle(tcp_handle);
        }
    }

    uv_tcp_ext_t *initUVTcpObject(ObjectData *objectData, uv_loop_t *loop, uv_tcp_ext_t *tcp_handler) {
        auto* data = Native::data<UVTcpData>(objectData);
        data->tcp_handle = tcp_handler;
        data->tcp_handle->tcp_object_data = objectData;
        uv_tcp_init(loop, data->tcp_handle);
        return data->tcp_handle;        
    }
    
    static void write_cb(uv_write_t *wr, int status) {      
        write_req_t *req = (write_req_t *) wr;
        uv_tcp_ext_t *tcp_handle = (uv_tcp_ext_t *) req->handle;
        auto* data = Native::data<UVTcpData>(tcp_handle->tcp_object_data);
        auto callback = data->writeCallback;
        if(!callback.isNull() && (tcp_handle->flag & UV_TCP_WRITE_CALLBACK_ENABLE)){
            vm_call_user_func(callback, make_packed_array(tcp_handle->tcp_object_data, status, req->buf.len));
        }
        delete req->buf.base;
        delete req;
    }
    
    static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
        buf->base = new char[suggested_size];
        buf->len = suggested_size;
    }

    static void read_cb(uv_tcp_ext_t *tcp_handle, ssize_t nread, const uv_buf_t* buf) {
        auto* data = Native::data<UVTcpData>(tcp_handle->tcp_object_data);
        if(nread > 0){
            if(!data->readCallback.isNull()){
                vm_call_user_func(data->readCallback, make_packed_array(tcp_handle->tcp_object_data, String(buf->base, nread, CopyString)));
            }
        }
        else if(nread < 0){
            if(!data->errorCallback.isNull()){
                vm_call_user_func(data->errorCallback, make_packed_array(tcp_handle->tcp_object_data, nread));
            }
            tcp_close_socket(tcp_handle);
        }
        delete buf->base;
    }
    
    void tcp_close_cb(uv_handle_t* handle) {
       releaseHandle((uv_tcp_ext_t *) handle);    
    }
    
    static void client_connection_cb(uv_connect_t* req, int status) {
        uv_tcp_ext_t *tcp_handle = (uv_tcp_ext_t *) req->handle;
        auto* data = Native::data<UVTcpData>(tcp_handle->tcp_object_data);
        auto callback = data->connectCallback;
        
        if(uv_read_start((uv_stream_t *) data->tcp_handle, alloc_cb, (uv_read_cb) read_cb)){
            return;
        }
        tcp_handle->flag |= (UV_TCP_HANDLE_START|UV_TCP_READ_START);
        if(!callback.isNull()){
            vm_call_user_func(callback, make_packed_array(tcp_handle->tcp_object_data, status));
        }
    }
    
    static void shutdown_cb(uv_shutdown_t* req, int status) {
        uv_tcp_ext_t *tcp_handle = (uv_tcp_ext_t *) req->handle;
        auto* data = Native::data<UVTcpData>(tcp_handle->tcp_object_data);
        auto callback = data->shutdownCallback;
        if(!callback.isNull()){
            vm_call_user_func(callback, make_packed_array(tcp_handle->tcp_object_data, status));
        }
    }    
    
    static void connection_cb(uv_tcp_ext_t *tcp_handle, int status) {
        auto* data = Native::data<UVTcpData>(tcp_handle->tcp_object_data);
        auto callback = data->connectCallback;
        if(!callback.isNull()){
            vm_call_user_func(callback, make_packed_array(tcp_handle->tcp_object_data, status));
        }
    }
            
    void HHVM_METHOD(UVTcp, __construct, const Variant &v_loop) {
        
        if(v_loop.isNull()){
            initUVTcpObject(this_, uv_default_loop());
            return;
        }
        
        Object loop = v_loop.toObject();
        checkUVLoopInstance(loop, 1, s_uvtcp, StaticString("__construct"));
        auto* loop_data = Native::data<UVLoopData>(loop.get());
        SET_LOOP(this_, loop, s_uvtcp);
        initUVTcpObject(this_, loop_data->loop);
    }
    
    int64_t HHVM_METHOD(UVTcp, listen, const String &host, int64_t port, const Variant &onConnectCallback) {
        int64_t ret;
        struct sockaddr_in addr;
        auto* data = Native::data<UVTcpData>(this_);
        if((ret = uv_ip4_addr(host.c_str(), port&0xffff, &addr)) != 0){
            return ret;
        }

        if((ret = uv_tcp_bind(data->tcp_handle, (const struct sockaddr*) &addr, 0)) != 0){
            return ret;
        }
        
        if((ret = uv_listen((uv_stream_t *) data->tcp_handle, SOMAXCONN, (uv_connection_cb) connection_cb)) != 0){
            return ret;
        }
        data->connectCallback = onConnectCallback;
        data->tcp_handle->flag |= UV_TCP_HANDLE_START;
        setSelfReference(data->tcp_handle);
        return ret;
    }
    
    static Object HHVM_METHOD(UVTcp, accept) {
        return Object(make_accepted_uv_tcp_object(this_, StaticString("UVTcp")));
    }
    
    ObjectData *make_accepted_uv_tcp_object(ObjectData *objectData, const String &class_name, uv_tcp_ext_t *tcp_handler) {
        ObjectData *client;
        auto* data = Native::data<UVTcpData>(objectData);
        uv_tcp_ext_t *server_tcp_handle, *client_tcp_handle;
        server_tcp_handle = data->tcp_handle;
        client = makeObject(class_name, false);
        client_tcp_handle = initUVTcpObject(client, server_tcp_handle->loop, tcp_handler);
        if(uv_accept((uv_stream_t *) server_tcp_handle, (uv_stream_t *) client_tcp_handle)) {
            client->decRefAndRelease();
            return NULL;
        }
        if(uv_read_start((uv_stream_t *) client_tcp_handle, alloc_cb, (uv_read_cb) read_cb)){
            client->decRefAndRelease();
            return NULL;
        }
        client->decRefCount();
        client_tcp_handle->flag |= (UV_TCP_HANDLE_START|UV_TCP_READ_START);
        return client;
    }
    
    static void HHVM_METHOD(UVTcp, close) {
        auto* data = Native::data<UVTcpData>(this_);

        tcp_close_socket(data->tcp_handle);
    }
    
    static void HHVM_METHOD(UVTcp, setCallback, const Variant &onReadCallback, const Variant &onWriteCallback, const Variant &onErrorCallback) {
        auto* data = Native::data<UVTcpData>(this_);
        data->readCallback = onReadCallback;
        data->writeCallback = onWriteCallback;
        data->errorCallback = onErrorCallback;
        setSelfReference(data->tcp_handle);
    }
    
    static int64_t HHVM_METHOD(UVTcp, write, const String &message) {
        auto* data = Native::data<UVTcpData>(this_);
        data->tcp_handle->flag |= UV_TCP_WRITE_CALLBACK_ENABLE;
        return tcp_write_raw((uv_stream_t *) data->tcp_handle, message.data(), message.size());
    }
    
    int64_t tcp_write_raw(uv_stream_t * handle, const char *message, int64_t size) {
        write_req_t *req;
        req = new write_req_t();
        req->buf.base = new char[size];
        req->buf.len = size;
        memcpy((void *) req->buf.base, message, size);
        return uv_write((uv_write_t *) req, handle, &req->buf, 1, write_cb);    
    }
    
    static String HHVM_METHOD(UVTcp, getSockname) {
        struct sockaddr addr;
        int addrlen;
        auto* data = Native::data<UVTcpData>(this_);
        
        if(data->tcp_handle->sockPort == 0){
            addrlen = sizeof addr;
            if(uv_tcp_getsockname((const uv_tcp_t *) data->tcp_handle, &addr, &addrlen)){
                return String();
            }
            data->tcp_handle->sockAddr = sock_addr(&addr);
            data->tcp_handle->sockPort = sock_port(&addr);
        }
        
        return data->tcp_handle->sockAddr;
    }

    static String HHVM_METHOD(UVTcp, getPeername) {
        struct sockaddr addr;
        int addrlen;
        auto* data = Native::data<UVTcpData>(this_);
        
        if(data->tcp_handle->peerPort == 0){
            addrlen = sizeof addr;
            if(uv_tcp_getpeername((const uv_tcp_t *) data->tcp_handle, &addr, &addrlen)){
                return String();
            }
            
            data->tcp_handle->peerAddr = sock_addr(&addr);
            data->tcp_handle->peerPort = sock_port(&addr);
        }
        
        return data->tcp_handle->peerAddr;
    }
    
    static int64_t HHVM_METHOD(UVTcp, getSockport) {
        struct sockaddr addr;
        int addrlen;
        auto* data = Native::data<UVTcpData>(this_);

        if(data->tcp_handle->sockPort == 0){
            addrlen = sizeof addr;
            if(uv_tcp_getsockname((const uv_tcp_t *) data->tcp_handle, &addr, &addrlen)){
                return -1;
            }
            data->tcp_handle->sockAddr = sock_addr(&addr);
            data->tcp_handle->sockPort = sock_port(&addr);
        }
        
        return data->tcp_handle->sockPort;
    }    
    
    static int64_t HHVM_METHOD(UVTcp, getPeerport) {
        struct sockaddr addr;
        int addrlen;
        auto* data = Native::data<UVTcpData>(this_);

        if(data->tcp_handle->peerPort == 0){
            addrlen = sizeof addr;
            if(uv_tcp_getpeername((const uv_tcp_t *) data->tcp_handle, &addr, &addrlen)){
                return -1;
            }
            data->tcp_handle->peerAddr = sock_addr(&addr);
            data->tcp_handle->peerPort = sock_port(&addr);
        }
        
        return data->tcp_handle->peerPort;
    }

    static int64_t HHVM_METHOD(UVTcp, connect, const String &host, int64_t port, const Variant &onConnectCallback) {
        int64_t ret;
        struct sockaddr_in addr;
        auto* data = Native::data<UVTcpData>(this_);
        
        if((ret = uv_ip4_addr(host.c_str(), port&0xffff, &addr)) != 0){
            return ret;
        }
        if((ret = uv_tcp_connect(&data->tcp_handle->connect_req, (uv_tcp_t *) data->tcp_handle, (const struct sockaddr*) &addr, client_connection_cb)) != 0){
            return ret;
        }
        
        data->connectCallback = onConnectCallback;
        setSelfReference(data->tcp_handle);
        data->tcp_handle->flag |= UV_TCP_HANDLE_START;
        return ret;
    }

    static int64_t HHVM_METHOD(UVTcp, shutdown, const Variant &onShutdownCallback) {
        int64_t ret;
        auto* data = Native::data<UVTcpData>(this_);

        data->shutdownCallback = onShutdownCallback;        
        if((ret = uv_shutdown(&data->tcp_handle->shutdown_req, (uv_stream_t *) data->tcp_handle, shutdown_cb)) != 0){
            return ret;
        }
        setSelfReference(data->tcp_handle);
        data->tcp_handle->flag |= UV_TCP_HANDLE_START;
        return ret;
    }

    
    void uvExtension::_initUVTcpClass() {
        HHVM_ME(UVTcp, __construct);
        HHVM_ME(UVTcp, listen);
        HHVM_ME(UVTcp, connect);
        HHVM_ME(UVTcp, accept);
        HHVM_ME(UVTcp, close);
        HHVM_ME(UVTcp, shutdown);
        HHVM_ME(UVTcp, setCallback);
        HHVM_ME(UVTcp, write);
        HHVM_ME(UVTcp, getSockname);
        HHVM_ME(UVTcp, getPeername);
        HHVM_ME(UVTcp, getSockport);
        HHVM_ME(UVTcp, getPeerport);                
        Native::registerNativeDataInfo<UVTcpData>(s_uvtcp.get());
    }
}