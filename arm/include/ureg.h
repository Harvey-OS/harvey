/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Ureg {
	ulong	r0;
	ulong	r1;
	ulong	r2;
	ulong	r3;
	ulong	r4;
	ulong	r5;
	ulong	r6;
	ulong	r7;
	ulong	r8;
	ulong	r9;
	ulong	r10;
	ulong	r11;
	ulong	r12;	/* sb */
	union {
		ulong	r13;
		ulong	sp;
	};
	union {
		ulong	r14;
		ulong	link;
	};
	ulong	type;	/* of exception */
	ulong	psr;
	ulong	pc;	/* interrupted addr */
} Ureg;
