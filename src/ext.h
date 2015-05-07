#ifndef EXT_H_
#define EXT_H_
#include "hhvm_include.h"
#include "resource/InternalResourceData.h"
#include "resource/R3ResourceData.h"
#include "common.h"
#include "util.h"
#include <uv.h>

namespace HPHP
{
    class uvExtension: public Extension
    {
        public:
            uvExtension(): Extension("uv"){}
            virtual void moduleInit()
            {
                _initUVUtilClass();            
                _initUVLoopClass();
                _initUVSignalClass();
                _initUVTcpClass();
                _initUVUdpClass();
                _initUVResolverClass();
                _initUVTimerClass();
                _initUVHttpServerClass();
                _initUVSSLClass();
                loadSystemlib();
            }
        private:
            void _initUVUtilClass();
            void _initUVLoopClass();
            void _initUVSignalClass();
            void _initUVTcpClass();
            void _initUVUdpClass();
            void _initUVResolverClass();
            void _initUVTimerClass();
            void _initUVHttpServerClass();
            void _initUVSSLClass();
    };
}
#endif
