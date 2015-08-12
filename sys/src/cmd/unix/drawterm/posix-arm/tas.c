/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "libc.h"

int
tas(int32_t *x)
{
	int v, t, i = 1;

#if ARMv5
	__asm__(
	    "swp  %0, %1, [%2]"
	    : "=&r"(v)
	    : "r"(1), "r"(x)
	    : "memory");
#else
	__asm__(
	    "1:	ldrex	%0, [%2]\n"
	    "	strex	%1, %3, [%2]\n"
	    "	teq	%1, #0\n"
	    "	bne	1b"
	    : "=&r"(v), "=&r"(t)
	    : "r"(x), "r"(i)
	    : "cc");
#endif
	switch(v) {
	case 0:
	case 1:
		return v;
	default:
		print("canlock: corrupted 0x%lux\n", v);
		return 1;
	}
}
