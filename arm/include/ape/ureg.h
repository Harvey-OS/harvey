#ifndef __UREG_H
#define __UREG_H
#if !defined(_PLAN9_SOURCE)
    This header file is an extension to ANSI/POSIX
#endif

struct Ureg
{
	unsigned long	r0;
	unsigned long	r1;
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
	unsigned long	r12;	/* sb */
	union {
		unsigned long	r13;
		unsigned long	sp;
	};
	union {
		unsigned long	r14;
		unsigned long	link;
	};
	unsigned long	type;	/* of exception */
	unsigned long	psr;
	unsigned long	pc;	/* interrupted addr */
};

#endif
