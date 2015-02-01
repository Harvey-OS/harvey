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
#include "dat.h"

// remainder = b mod m
//
// knuth, vol 2, pp 398-400

void
mpmod(mpint *b, mpint *m, mpint *remainder)
{
	mpdiv(b, m, nil, remainder);
	if(remainder->sign < 0)
		mpadd(m, remainder, remainder);
}
