#ifndef __UREG_H
#define __UREG_H
#if !defined(_PLAN9_SOURCE)
    This header file is an extension to ANSI/POSIX
#endif

struct Ureg
{
	/* l.s saves 31 64-bit values: */
	unsigned long long	type;
	unsigned long long	a0;
	unsigned long long	a1;
	unsigned long long	a2;

	unsigned long long	r0;
	unsigned long long	r1;
	unsigned long long	r2;
	unsigned long long	r3;
	unsigned long long	r4;
	unsigned long long	r5;
	unsigned long long	r6;
	unsigned long long	r7;
	unsigned long long	r8;
	unsigned long long	r9;
	unsigned long long	r10;
	unsigned long long	r11;
	unsigned long long	r12;
	unsigned long long	r13;
	unsigned long long	r14;
	unsigned long long	r15;

	unsigned long long	r19;
	unsigned long long	r20;
	unsigned long long	r21;
	unsigned long long	r22;
	unsigned long long	r23;
	unsigned long long	r24;
	unsigned long long	r25;
	unsigned long long	r26;
	unsigned long long	r27;
	unsigned long long	r28;
	union {
		unsigned long long	r30;
		unsigned long long	usp;
		unsigned long long	sp;
	};

	/* OSF/1 PALcode frame: */
	unsigned long long	status;	/* PS */
	unsigned long long	pc;
	unsigned long long	r29;		/* GP */
	unsigned long long	r16;		/* a0 */
	unsigned long long	r17;		/* a1 */
	unsigned long long	r18;		/* a2 */
};

#endif
