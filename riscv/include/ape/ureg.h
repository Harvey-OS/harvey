#ifndef __UREG_H
#define __UREG_H
#if !defined(_PLAN9_SOURCE)
    This header file is an extension to ANSI/POSIX
#endif

#define ulong unsigned long

struct Ureg
{
	union {
		ulong	pc;
		ulong regs[1];
	};
	ulong	r1;		/* link */
	union{
		ulong	r2;
		ulong	sp;
		ulong	usp;
	};
	ulong	r3;		/* sb */
	ulong	r4;
	ulong	r5;
	ulong	r6;		/* up in kernel */
	ulong	r7;		/* m in kernel */
	union{
		ulong	r8;
		ulong arg;
		ulong ret;
	};
	ulong	r9;
	ulong	r10;
	ulong	r11;
	ulong	r12;
	ulong	r13;
	ulong	r14;
	ulong	r15;
	ulong	r16;
	ulong	r17;
	ulong	r18;
	ulong	r19;
	ulong	r20;
	ulong	r21;
	ulong	r22;
	ulong	r23;
	ulong	r24;
	ulong	r25;
	ulong	r26;
	ulong	r27;
	ulong	r28;
	ulong	r29;
	ulong	r30;
	ulong	r31;

	/* csrs: generally supervisor ones */
	ulong	status;
	ulong	ie;
	union {
		ulong	cause;
		ulong	type;
	};
	ulong	tval;			/* faulting address */

	ulong	curmode;
};

#endif
