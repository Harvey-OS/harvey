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
	uint32_t	r0;
	uint32_t	r1;
	uint32_t	r2;
	uint32_t	r3;
	uint32_t	r4;
	uint32_t	r5;
	uint32_t	r6;
	uint32_t	r7;
	uint32_t	a0;
	uint32_t	a1;
	uint32_t	a2;
	uint32_t	a3;
	uint32_t	a4;
	uint32_t	a5;
	uint32_t	a6;
	uint32_t	sp;
	uint32_t	usp;
	uint32_t	magic;		/* for db to find bottom of ureg */
	ushort	sr;
	uint32_t	pc;
	ushort	vo;
#ifndef	UREGVARSZ
#define	UREGVARSZ 23		/* for 68040; 15 is enough on 68020 */
#endif
	uchar	microstate[UREGVARSZ];	/* variable-sized portion */
};
