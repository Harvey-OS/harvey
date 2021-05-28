#ifndef __UREG_H
#define __UREG_H
#if !defined(_PLAN9_SOURCE)
    This header file is an extension to ANSI/POSIX
#endif

struct Ureg
{
	union {
		unsigned long	pc;
		unsigned long regs[1];
	};
	unsigned long	r1;		/* link */
	union{
		unsigned long	r2;
		unsigned long	sp;
		unsigned long	usp;
	};
	unsigned long	r3;		/* sb */
	unsigned long	r4;
	unsigned long	r5;
	unsigned long	r6;		/* up in kernel */
	unsigned long	r7;		/* m in kernel */
	union{
		unsigned long	r8;
		unsigned long arg;
		unsigned long ret;
	};
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

	/* csrs: generally supervisor ones */
	unsigned long	status;
	unsigned long	ie;
	union {
		unsigned long	cause;
		unsigned long	type;
	};
	unsigned long	tval;			/* faulting address */
};

#endif
