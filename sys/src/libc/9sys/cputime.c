/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>

#define	HZ	1000

double
cputime(void)
{
	long t[4];
	int i;

	times(t);
	for(i=1; i<4; i++)
		t[0] += t[i];
	return t[0] / (double)HZ;
}
