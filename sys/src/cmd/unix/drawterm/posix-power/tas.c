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

/*
 * first argument (l) is in r3 at entry.
 * r3 contains return value upon return.
 */
int
tas(long *x)
{
	int     v;
	/*
	 * this __asm__ works with gcc 2.95.2 (mac os x 10.1).
	 * this assembly language destroys r0 (0), some other register (v),
	 * r4 (x) and r5 (temp).
	 */
	__asm__("\n	sync\n"
	"	li	r0,0\n"
	"	mr	r4,%1		/* &l->val */\n"
	"	lis	r5,0xdead	/* assemble constant 0xdeaddead */\n"
	"	ori	r5,r5,0xdead	/* \" */\n"
	"tas1:\n"
	"	dcbf	r4,r0	/* cache flush; \"fix for 603x bug\" */\n"
	"	lwarx	%0,r4,r0	/* v = l->val with reservation */\n"
	"	cmp	cr0,0,%0,r0	/* v == 0 */\n"
	"	bne	tas0\n"
	"	stwcx.	r5,r4,r0   /* if (l->val same) l->val = 0xdeaddead */\n"
	"	bne	tas1\n"
	"tas0:\n"
	"	sync\n"
	"	isync\n"
	: "=r" (v)
	: "r"  (x)
	: "cc", "memory", "r0", "r4", "r5"
	);
	switch(v) {
	case 0:		return 0;
	case 0xdeaddead: return 1;
	default:	print("tas: corrupted 0x%lux\n", v);
	}
	return 0;
}
