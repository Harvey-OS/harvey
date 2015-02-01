/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <float.h>
#include <math.h>

#define	NANEXP	(2047<<20)
#define	NANMASK	(2047<<20)
#define	NANSIGN	(1<<31)

double
NaN(void)
{
	FPdbleword a;

	a.hi = NANEXP;
	a.lo = 1;
	return a.x;
}

int
isNaN(double d)
{
	FPdbleword a;

	a.x = d;
	if((a.hi & NANMASK) != NANEXP)
		return 0;
	return !isInf(d, 0);
}

double
Inf(int sign)
{
	FPdbleword a;

	a.hi = NANEXP;
	a.lo = 0;
	if(sign < 0)
		a.hi |= NANSIGN;
	return a.x;
}

int
isInf(double d, int sign)
{
	FPdbleword a;

	a.x = d;
	if(a.lo != 0)
		return 0;
	if(a.hi == NANEXP)
		return sign >= 0;
	if(a.hi == (NANEXP|NANSIGN))
		return sign <= 0;
	return 0;
}
