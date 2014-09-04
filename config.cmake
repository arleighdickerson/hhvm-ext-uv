set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

include(buildLibuv)
include(buildr3)
set(CLEAN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/build\;${CLEAN_FILES}")
set(CLEAN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/CMakeCache.txt\;${CLEAN_FILES}")
set(CLEAN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake_install.cmake\;${CLEAN_FILES}")
set(CLEAN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt\;${CLEAN_FILES}")

set(HTTP_PARSER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/http-parser")

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CLEAN_FILES})

CONFIGURE_FILE(${CMAKE_CURRENT_BINARY_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

include_directories(${LIBUV_INCLUDE_DIR} ${HTTP_PARSER_INCLUDE_DIR})
set(LIBUV_LIBRARIES ${LIBUV_LIB}/libuv.a ${R3_LIB}/libr3.a)

HHVM_EXTENSION(uv
    src/resource/InternalResourceData.cpp
    src/resource/CallbackResourceData.cpp
    src/resource/TcpResourceData.cpp
    src/resource/UdpResourceData.cpp
    src/resource/ResolverResourceData.cpp
    src/resource/R3ResourceData.cpp
    src/ext.cpp
    src/uv_util.cpp
    src/uv_loop.cpp
    src/uv_signal.cpp
    src/uv_tcp.cpp
    src/uv_udp.cpp
    src/uv_resolver.cpp
    src/uv_timer.cpp
    src/uv_http_server.cpp
    src/uv_http_parser.cpp
    thirdparty/http-parser/http_parser.c
)
HHVM_SYSTEMLIB(uv ext_uv.php)

target_link_libraries(uv ${LIBUV_LIBRARIES})
