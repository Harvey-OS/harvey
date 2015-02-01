/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>

#define	MASK	0x7fffffffL

int
nrand(int n)
{
	long slop, v;

	if(n < 0)
		return n;
	if(n == 1)
		return 0;
	/* and if n == 0, you deserve what you get */
	slop = MASK % n;
	do
		v = lrand();
	while(v <= slop);
	return v % n;
}
