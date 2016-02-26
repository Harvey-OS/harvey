/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define nil		((void*)0)
typedef	unsigned char	uint8_t;
typedef signed char	int8_t;
typedef	unsigned short	uint16_t;
typedef	signed short	int16_t;
typedef unsigned int	uint32_t;
typedef unsigned int	uint;
typedef signed int	int32_t;
typedef	unsigned long long uint64_t;
typedef	long long	int64_t;
typedef uint64_t uintptr;
typedef uint64_t uintptr_t;
typedef uint32_t	usize;
typedef int64_t size_t;
typedef	uint32_t		Rune;
typedef union FPdbleword FPdbleword;
// This is a guess! Assumes float!
typedef uintptr		jmp_buf[28]; // for registers.

#define	JMPBUFSP	13
#define	JMPBUFPC	0
#define	JMPBUFARG1	1
#define	JMPBUFARG2	2

#define	JMPBUFDPC	0 // What? 
typedef unsigned int	mpdigit;	/* for /sys/include/mp.h */

union FPdbleword
{
	double	x;
	struct {	/* little endian */
		uint lo;
		uint hi;
	};
};

typedef __builtin_va_list va_list;

#define va_start(v,l)	__builtin_va_start(v,l)
#define va_end(v)	__builtin_va_end(v)
#define va_arg(v,l)	__builtin_va_arg(v,l)
#define va_copy(v,l)	__builtin_va_copy(v,l)

