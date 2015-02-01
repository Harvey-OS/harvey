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
/*  0*/	u64int	cause;	/* trap or interrupt vector */
/*  8*/	u64int	msr; /* SRR1 */
/* 16*/	u64int	pc;	/* SRR0 */
/* 24*/	u64int	unused;
/* 32*/	u64int	lr;
/* 36*/	u32int	pad;
/* 40*/	u32int	cr;
/* 48*/	u64int	xer;
/* 56*/	u64int	ctr;
/* 64*/	u64int	r0;
/* 72*/	union{ u64int r1;	u64int	sp;	u64int	usp; };
/* 80*/	u64int	r2;
/* 88*/	u64int	r3;
/* 96*/	u64int	r4;
/*104*/	u64int	r5;
/*112*/	u64int	r6;
/*120*/	u64int	r7;
/*128*/	u64int	r8;
/*136*/	u64int	r9;
/*144*/	u64int	r10;
/*152*/	u64int	r11;
/*160*/	u64int	r12;
/*168*/	u64int	r13;
/*176*/	u64int	r14;
/*184*/	u64int	r15;
/*192*/	u64int	r16;
/*200*/	u64int	r17;
/*208*/	u64int	r18;
/*216*/	u64int	r19;
/*224*/	u64int	r20;
/*232*/	u64int	r21;
/*240*/	u64int	r22;
/*248*/	u64int	r23;
/*256*/	u64int	r24;
/*264*/	u64int	r25;
/*272*/	u64int	r26;
/*280*/	u64int	r27;
/*288*/	u64int	r28;
/*296*/	u64int	r29;
/*304*/	u64int	r30;
/*312*/	u64int	r31;
};
