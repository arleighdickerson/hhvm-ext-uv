#ifndef RESOURCE_UTIL_H
#define RESOURCE_UTIL_H
#ifdef IMPLEMENT_OBJECT_ALLOCATION
    #define RESOURCEDATA_ALLOCATION(x) IMPLEMENT_OBJECT_ALLOCATION(x)
#else
    #define RESOURCEDATA_ALLOCATION(x) IMPLEMENT_RESOURCE_ALLOCATION(x)
#endif
#endif