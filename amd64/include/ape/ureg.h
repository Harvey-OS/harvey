#ifndef __UREG_H
#define __UREG_H
#if !defined(_PLAN9_SOURCE)
    This header file is an extension to ANSI/POSIX
#endif

struct Ureg {
	unsigned long long	ax;
	unsigned long long	bx;
	unsigned long long	cx;
	unsigned long long	dx;
	unsigned long long	si;
	unsigned long long	di;
	unsigned long long	bp;
	unsigned long long	r8;
	unsigned long long	r9;
	unsigned long long	r10;
	unsigned long long	r11;
	unsigned long long	r12;
	unsigned long long	r13;
	unsigned long long	r14;
	unsigned long long	r15;

	unsigned short		ds;
	unsigned short		es;
	unsigned short		fs;
	unsigned short		gs;

	unsigned long long	type;
	unsigned long long	error;		/* error code (or zero) */
	unsigned long long	ip;		/* pc */
	unsigned long long	cs;		/* old context */
	unsigned long long	flags;		/* old flags */
	unsigned long long	sp;		/* sp */
	unsigned long long	ss;		/* old stack segment */
};

#endif
