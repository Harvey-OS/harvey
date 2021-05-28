#ifndef __UREG_H
#define __UREG_H
#if !defined(_PLAN9_SOURCE)
    This header file is an extension to ANSI/POSIX
#endif

#define uvlong unsigned long long

struct Ureg
{
	union {
		uvlong	pc;
		uvlong regs[1];
	};
	uvlong	r1;		/* link */
	union{
		uvlong	r2;
		uvlong	sp;
		uvlong	usp;
	};
	uvlong	r3;		/* sb */
	uvlong	r4;
	uvlong	r5;
	uvlong	r6;		/* up in kernel */
	uvlong	r7;		/* m in kernel */
	union{
		uvlong	r8;
		uvlong arg;
		uvlong ret;
	};
	uvlong	r9;
	uvlong	r10;
	uvlong	r11;
	uvlong	r12;
	uvlong	r13;
	uvlong	r14;
	uvlong	r15;
	uvlong	r16;
	uvlong	r17;
	uvlong	r18;
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
	uvlong	r29;
	uvlong	r30;
	uvlong	r31;

	/* csrs: generally supervisor ones */
	uvlong	status;
	uvlong	ie;
	union {
		uvlong	cause;
		uvlong	type;
	};
	uvlong	tval;			/* faulting address */

	uvlong	curmode;
};

#endif
