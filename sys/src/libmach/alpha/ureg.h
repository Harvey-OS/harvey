/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

struct Ureg
{
	/* l.s saves 31 64-bit values: */
	uvlong	type;
	uvlong	a0;
	uvlong	a1;
	uvlong	a2;

	uvlong	r0;
	uvlong	r1;
	uvlong	r2;
	uvlong	r3;
	uvlong	r4;
	uvlong	r5;
	uvlong	r6;
	uvlong	r7;
	uvlong	r8;
	uvlong	r9;
	uvlong	r10;
	uvlong	r11;
	uvlong	r12;
	uvlong	r13;
	uvlong	r14;
	uvlong	r15;

	uvlong	r19;
	uvlong	r20;
	uvlong	r21;
	uvlong	r22;
	uvlong	r23;
	uvlong	r24;
	uvlong	r25;
	uvlong	r26;
	uvlong	r27;
	uvlong	r28;
	union {
		uvlong	r30;
		uvlong	usp;
		uvlong	sp;
	};

	/* OSF/1 PALcode frame: */
	uvlong	status;	/* PS */
	uvlong	pc;
	uvlong	r29;		/* GP */
	uvlong	r16;		/* a0 */
	uvlong	r17;		/* a1 */
	uvlong	r18;		/* a2 */
};
