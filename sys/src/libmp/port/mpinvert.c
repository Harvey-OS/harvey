/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "os.h"
#include <mp.h>

#define iseven(a)	(((a)->p[0] & 1) == 0)

// use extended gcd to find the multiplicative inverse
// res = b**-1 mod m
void
mpinvert(mpint *b, mpint *m, mpint *res)
{
	mpint *dc1, *dc2;	// don't care

	dc1 = mpnew(0);
	dc2 = mpnew(0);
	mpextendedgcd(b, m, dc1, res, dc2);
	if(mpcmp(dc1, mpone) != 0)
		abort();
	mpmod(res, m, res);
	mpfree(dc1);
	mpfree(dc2);
}
