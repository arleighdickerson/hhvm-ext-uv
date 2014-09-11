#include <openssl/ssl.h>
#include "ext.h"
#include "hphp/runtime/base/thread-init-fini.h"
#include "uv_tcp.h"

#ifdef OPENSSL_FOUND
    #define check_ssl_support()
#else
    #define check_ssl_support() raise_error("please recompile hhvm libuv extension with opeensl support.")
#endif

typedef struct {
    SSL_CTX* ctx;
    SSL* ssl;
    BIO* read_bio;
    BIO* write_bio;
} ssl_ext_t;

namespace HPHP {
    
    ALWAYS_INLINE ssl_ext_t *fetchSSLResource(const Object &obj){
        TcpResourceData *resource_data = FETCH_RESOURCE(obj, TcpResourceData, s_uvtcp);
        uv_tcp_ext_t *tcp_handle = (uv_tcp_ext_t *) resource_data->getInternalResourceData();        
        return (ssl_ext_t *) tcp_handle->custom_data;    
    }
    
    static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
        buf->base = new char[suggested_size];
        buf->len = suggested_size;
    }

    int64_t write_bio_to_socket(uv_tcp_ext_t *tcp_handle){
        ssl_ext_t *ssl = (ssl_ext_t *)tcp_handle->custom_data;
        char buf[1024];
        int hasread, ret;
        while(true){
            hasread  = BIO_read(ssl->write_bio, buf, sizeof(buf));
            if(hasread <= 0){
                return 0;
            }
            ret = tcp_write_raw((uv_stream_t *) tcp_handle, buf, hasread);
            if(ret != 0){
                break;
            }
        }
        return ret;
    }    

    static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
        TcpResourceData *tcp_resource_data = FETCH_RESOURCE(((uv_tcp_ext_t *) stream)->tcp_object_data, TcpResourceData, s_uvtcp);
        uv_tcp_ext_t *tcp_handle = (uv_tcp_ext_t *) tcp_resource_data->getInternalResourceData();
        ssl_ext_t *ssl = (ssl_ext_t *)tcp_handle->custom_data;        
        char read_buf[1024];
        int size, err, ret;
        if(nread > 0){
            BIO_write(ssl->read_bio, buf->base, nread);
            if (!SSL_is_init_finished(ssl->ssl)) {
                ret = SSL_do_handshake(ssl->ssl);
                write_bio_to_socket(tcp_handle);
                if(ret != 1){
                    err = SSL_get_error(ssl->ssl, ret);
                    if (err == SSL_ERROR_WANT_READ) {
                        return;
                    }
                    else if(err == SSL_ERROR_WANT_WRITE){
                        write_bio_to_socket(tcp_handle);
                    }
                }
                return;
            }
            tcp_handle->flag |= UV_TCP_WRITE_CALLBACK_ENABLE;
            size = SSL_read(ssl->ssl, read_buf, sizeof(read_buf));
            if(size > 0 && !tcp_resource_data->getReadCallback().isNull()){                            
                vm_call_user_func(tcp_resource_data->getReadCallback(), make_packed_array(((uv_tcp_ext_t *) stream)->tcp_object_data, StringData::Make(read_buf, size, CopyString)));
            }
        }
        else if(nread<0){
            if(!tcp_resource_data->getErrorCallback().isNull()){
                vm_call_user_func(tcp_resource_data->getErrorCallback(), make_packed_array(((uv_tcp_ext_t *) stream)->tcp_object_data, nread));
            }
            tcp_close_socket(tcp_handle);
        }
        delete buf->base;
    }                                 
    
    ALWAYS_INLINE void initSSL(const Object &obj) {
        TcpResourceData *resource_data = FETCH_RESOURCE(obj, TcpResourceData, s_uvtcp);
        uv_tcp_ext_t *tcp_handle = (uv_tcp_ext_t *) resource_data->getInternalResourceData();
        ssl_ext_t *ssl = new ssl_ext_t();
        tcp_handle->custom_data = (void *) ssl;
        memset(ssl, '\0', sizeof(ssl_ext_t));
    }
    
    static void HHVM_METHOD(UVSSL, __construct){
        check_ssl_support();
        ssl_ext_t *ssl;    
        HHVM_MN(UVTcp, __construct)(this_);
        initSSL(this_);
        ssl = fetchSSLResource(this_);
        ssl->ctx = SSL_CTX_new(SSLv23_method());            
    }
    
    static void HHVM_METHOD(UVSSL, __destruct){
        ssl_ext_t *ssl = fetchSSLResource(this_);
        if(ssl){
        //use SSL_free instead
/*
            if(ssl->read_bio){
                BIO_free(ssl->read_bio);                
            }
            if(ssl->write_bio){
                BIO_free(ssl->write_bio);
            }*/
            if(ssl->ctx){
                SSL_CTX_free(ssl->ctx);
            }
            if(ssl->ssl){
                SSL_free(ssl->ssl);
            }
            delete ssl;
        }
        HHVM_MN(UVTcp, __destruct)(this_);
    }

    static bool HHVM_METHOD(UVSSL, setCertFile, const String &certFile){
        ssl_ext_t *ssl = fetchSSLResource(this_);
        return SSL_CTX_use_certificate_file(ssl->ctx, certFile.c_str(), SSL_FILETYPE_PEM) == 1;
    }
    
    static bool HHVM_METHOD(UVSSL, setCertChainFile, const String &certChainFile){
        ssl_ext_t *ssl = fetchSSLResource(this_);
        return SSL_CTX_use_certificate_chain_file(ssl->ctx, certChainFile.c_str()) == 1;
    }    
    
    static bool HHVM_METHOD(UVSSL, setPrivateKeyFile, const String &privateKeyFile){
        ssl_ext_t *ssl = fetchSSLResource(this_);
        return SSL_CTX_use_PrivateKey_file(ssl->ctx, privateKeyFile.c_str(), SSL_FILETYPE_PEM) == 1;
    }    
    
    static Object HHVM_METHOD(UVSSL, accept){
        ssl_ext_t *server_ssl, *ssl;
        Object obj = make_accepted_uv_tcp_object(this_, "UVSSL");        
        TcpResourceData *resource_data = FETCH_RESOURCE(obj, TcpResourceData, s_uvtcp);
        uv_tcp_ext_t *tcp_handle = (uv_tcp_ext_t *) resource_data->getInternalResourceData();
        uv_read_start((uv_stream_t *) tcp_handle, alloc_cb, read_cb);
        initSSL(obj);
        server_ssl = fetchSSLResource(this_);                
        ssl = fetchSSLResource(obj);
        ssl->ssl = SSL_new(server_ssl->ctx);
        ssl->read_bio = BIO_new(BIO_s_mem());
        ssl->write_bio = BIO_new(BIO_s_mem());        
        SSL_set_bio(ssl->ssl, ssl->read_bio, ssl->write_bio);
        SSL_set_accept_state(ssl->ssl);
        tcp_handle->flag |= (UV_TCP_HANDLE_START|UV_TCP_READ_START);        
        return obj;
    }

    static int64_t HHVM_METHOD(UVSSL, setCallback, const Variant &onReadCallback, const Variant &onWriteCallback, const Variant &onErrorCallback) {
        TcpResourceData *resource_data = FETCH_RESOURCE(this_, TcpResourceData, s_uvtcp);
        resource_data->setReadCallback(onReadCallback);  
        resource_data->setWriteCallback(onWriteCallback);
        resource_data->setErrorCallback(onErrorCallback);                                            
        return 0;
    }
    
    static int64_t HHVM_METHOD(UVSSL, write, const String &data) {
        ssl_ext_t *ssl;
        int size;
        TcpResourceData *resource_data = FETCH_RESOURCE(this_, TcpResourceData, s_uvtcp);
        uv_tcp_ext_t *tcp_handle = (uv_tcp_ext_t *) resource_data->getInternalResourceData();
        ssl = (ssl_ext_t *) tcp_handle->custom_data;
        size = SSL_write(ssl->ssl, data.data(), data.size()); 
        if(size > 0){
            return write_bio_to_socket(tcp_handle);                        
        }
        return 0;
    }
    
    void uvExtension::_initUVSSLClass() {
        HHVM_ME(UVSSL, accept);
        HHVM_ME(UVSSL, setCallback);
        HHVM_ME(UVSSL, write);
        HHVM_ME(UVSSL, __construct);
        HHVM_ME(UVSSL, __destruct);
        HHVM_ME(UVSSL, setCertFile);
        HHVM_ME(UVSSL, setCertChainFile);
        HHVM_ME(UVSSL, setPrivateKeyFile);                        
    }
}