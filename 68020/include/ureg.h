struct Ureg
{
	ulong	r0;
	ulong	r1;
	ulong	r2;
	ulong	r3;
	ulong	r4;
	ulong	r5;
	ulong	r6;
	ulong	r7;
	ulong	a0;
	ulong	a1;
	ulong	a2;
	ulong	a3;
	ulong	a4;
	ulong	a5;
	ulong	a6;
	ulong	sp;
	ulong	usp;
	ulong	magic;		/* for db to find bottom of ureg */
	ushort	sr;
	ulong	pc;
	ushort	vo;
#ifndef	UREGVARSZ
#define	UREGVARSZ 23		/* for 68040; 15 is enough on 68020 */
#endif
	uchar	microstate[UREGVARSZ];	/* variable-sized portion */
};
