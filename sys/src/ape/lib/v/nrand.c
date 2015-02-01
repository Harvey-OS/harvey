/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdlib.h>

#define	MASK	0x7FFFFFFFL
#define	FRACT	(1.0 / (MASK + 1.0))

extern long lrand(void);

double
frand(void)
{

	return lrand() * FRACT;
}

nrand(int n)
{
	long slop, v;

	slop = MASK % n;
	do
		v = lrand();
	while(v <= slop);
	return v % n;
}
