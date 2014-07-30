#include "ext.h"
#include <r3/r3.h>
node *n = NULL;
namespace HPHP {
    static Variant HHVM_METHOD(UVHttpServer, _R3RoutesAdd, const Array &routes) {
        Resource resource(NEWOBJ(R3ResourceData(routes.size())));
        SET_RESOURCE(this_, resource, s_uvhttpserver);
        R3ResourceData *resource_data = FETCH_RESOURCE(this_, R3ResourceData, s_uvhttpserver);
        node *n = resource_data->getR3ResourceData();
        for (ArrayIter iter(routes); iter; ++iter) {
            Variant key(iter.first());
            String pattern = routes.rvalAt(key).toArray().rvalAt(0).toString();
            int64_t idx = key.toInt64Val();
            if(r3_tree_insert_routel(n, -1, pattern.c_str(), pattern.size(), (void *) idx) == NULL) {
                return idx;
            }
        }
        
        if(r3_tree_compile(n, NULL) != 0) {
            return -1;
        }
        return true;
    }
    
    static Array HHVM_METHOD(UVHttpServer, _R3Match, const String &uri, int64_t method) {
        R3ResourceData *resource_data = FETCH_RESOURCE(this_, R3ResourceData, s_uvhttpserver);
        node *n = resource_data->getR3ResourceData();
        Array ret;

        match_entry * entry = match_entry_create(uri.c_str());        
        entry->request_method = method;
        route *matched_route = r3_tree_match_route(n, entry);
                
        if(matched_route != NULL) {
            Array params;
            int64_t matched = (int64_t) matched_route->data;
            for(int i=0;i<entry->vars->len;i++){
                params.append(Variant(entry->vars->tokens[i]));
            }
            ret = make_packed_array(matched, params);
        }
        
        match_entry_free(entry);
        return ret;
    }
    
    void uvExtension::_initUVHttpServerClass() {
        HHVM_ME(UVHttpServer, _R3RoutesAdd);
        HHVM_ME(UVHttpServer, _R3Match);
    }
}