/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

#define nil ((void *)0)

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef unsigned int uint;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
typedef uint64_t uintptr;
typedef uint64_t uintptr_t;
typedef uint64_t usize;
typedef uint64_t size_t;
typedef uint32_t Rune;

typedef union FPdbleword FPdbleword;
union FPdbleword
{
	double x;
	struct {	// little endian
		uint32_t lo;
                uint32_t hi;
	};
};

typedef uint64_t jmp_buf[32];

#define JMPBUFDPC       0
#define JMPBUFSP        30
#define JMPBUFPC        31
#define JMPBUFARG1      0
#define JMPBUFARG2      1

typedef uint32_t	mpdigit;	// for /sys/include/mp.h

typedef __builtin_va_list va_list;

#define va_start(v,l)   __builtin_va_start(v,l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v,l)     __builtin_va_arg(v,l)
#define va_copy(v,l)    __builtin_va_copy(v,l)

#define getcallerpc()	((uintptr_t)__builtin_return_address(0))
