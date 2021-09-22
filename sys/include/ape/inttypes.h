#ifndef _SUSV2_SOURCE
#error "inttypes.h is SUSV2"
#endif

#ifndef _INTTYPES_H_
#define _INTTYPES_H_ 1

#include "_apetypes.h"

#ifdef _BITS64
typedef long long _intptr_t;
typedef unsigned long long _uintptr_t;
#else
typedef int _intptr_t;
typedef unsigned int _uintptr_t;
#endif

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef _intptr_t intptr_t;
typedef _uintptr_t uintptr_t;

#endif
