/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define nil ((void *)0)
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef unsigned int uint;
typedef signed int int32_t;
typedef unsigned long long uint64_t;
typedef long long int64_t;
typedef uint64_t uintptr;
typedef uint64_t uintptr_t;
typedef uint32_t usize;
typedef int64_t size_t;
typedef uint32_t Rune;
typedef union FPdbleword FPdbleword;
typedef uintptr jmp_buf[10]; // for registers.

#define JMPBUFSP 6
#define JMPBUFPC 7
#define JMPBUFARG1 8
#define JMPBUFARG2 9

#define JMPBUFDPC 0
typedef unsigned int mpdigit; /* for /sys/include/mp.h */

/* MXCSR */
/* fcr */
#define FPFTZ (1 << 15) /* amd64 */
#define FPINEX (1 << 12)
#define FPUNFL (1 << 11)
#define FPOVFL (1 << 10)
#define FPZDIV (1 << 9)
#define FPDNRM (1 << 8) /* amd64 */
#define FPINVAL (1 << 7)
#define FPDAZ (1 << 6) /* amd64 */
#define FPRNR (0 << 13)
#define FPRZ (3 << 13)
#define FPRPINF (2 << 13)
#define FPRNINF (1 << 13)
#define FPRMASK (3 << 13)
#define FPPEXT 0
#define FPPSGL 0
#define FPPDBL 0
#define FPPMASK 0
/* fsr */
#define FPAINEX (1 << 5)
#define FPAUNFL (1 << 4)
#define FPAOVFL (1 << 3)
#define FPAZDIV (1 << 2)
#define FPADNRM (1 << 1) /* not in plan 9 */
#define FPAINVAL (1 << 0)
union FPdbleword {
	double x;
	struct {/* little endian */
		uint lo;
		uint hi;
	};
};

/*
#if 0
typedef	char*	va_list;
#define va_start(list, start) list =\
	(sizeof(start) < 8?\
		(char*)((int64_t*)&(start)+1):\
		(char*)(&(start)+1))
#define va_end(list)\
	USED(list)
#define va_arg(list, mode)\
	((sizeof(mode) == 1)?\
		((list += 8), (mode*)list)[-8]:\
	(sizeof(mode) == 2)?\
		((list += 8), (mode*)list)[-4]:\
	(sizeof(mode) == 4)?\
		((list += 8), (mode*)list)[-2]:\
		((list += sizeof(mode)), (mode*)list)[-1])
#endif
*/

typedef __builtin_va_list va_list;

#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v, l) __builtin_va_arg(v, l)
#define va_copy(v, l) __builtin_va_copy(v, l)
