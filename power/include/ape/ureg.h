/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __UREG_H
#define __UREG_H
#if !defined(_PLAN9_SOURCE)
    This header file is an extension to ANSI/POSIX
#endif

struct Ureg
{	unsigned long	cause;
	union { unsigned long	srr1; unsigned long status;};
	unsigned long	pc;	/* SRR0 */
	unsigned long	pad;
	unsigned long	lr;
	unsigned long	cr;
	unsigned long	xer;
	unsigned long	ctr;
	unsigned long	r0;
	union{ unsigned long r1;	unsigned long	sp;	unsigned long	usp; };
	unsigned long	r2;
	unsigned long	r3;
	unsigned long	r4;
	unsigned long	r5;
	unsigned long	r6;
	unsigned long	r7;
	unsigned long	r8;
	unsigned long	r9;
	unsigned long	r10;
	unsigned long	r11;
	unsigned long	r12;
	unsigned long	r13;
	unsigned long	r14;
	unsigned long	r15;
	unsigned long	r16;
	unsigned long	r17;
	unsigned long	r18;
	unsigned long	r19;
	unsigned long	r20;
	unsigned long	r21;
	unsigned long	r22;
	unsigned long	r23;
	unsigned long	r24;
	unsigned long	r25;
	unsigned long	r26;
	unsigned long	r27;
	unsigned long	r28;
	unsigned long	r29;
	unsigned long	r30;
	unsigned long	r31;
};

#endif
