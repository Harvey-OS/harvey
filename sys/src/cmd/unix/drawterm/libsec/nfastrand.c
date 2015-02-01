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
#include <libsec.h>

#define Maxrand	((1UL<<31)-1)

ulong
nfastrand(ulong n)
{
	ulong m, r;
	
	/*
	 * set m to the maximum multiple of n <= 2^31-1
	 * so we want a random number < m.
	 */
	if(n > Maxrand)
		abort();

	m = Maxrand - Maxrand % n;
	while((r = fastrand()) >= m)
		;
	return r%n;
}
