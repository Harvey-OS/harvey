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
	unsigned long	a0;
	unsigned long	a1;
	unsigned long	a2;
	unsigned long	a3;
	unsigned long	a4;
	unsigned long	a5;
	unsigned long	a6;
	unsigned long	sp;
	unsigned long	usp;
	unsigned long	magic;		/* for db to find bottom of ureg */
	unsigned short	sr;
	unsigned long	pc;
	unsigned short	vo;
#ifndef	UREGVARSZ
#define	UREGVARSZ 23		/* for 68040; 15 is enough on 68020 */
#endif
	unsigned char	microstate[UREGVARSZ];	/* variable-sized portion */
};

#endif
