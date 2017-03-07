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

/* return uniform random [0..n-1] */
mpint*
mpnrand(mpint *n, void (*gen)(uint8_t*, int), mpint *b)
{
	int bits;

	bits = mpsignif(n);
	if(bits == 0)
		abort();
	if(b == nil){
		b = mpnew(bits);
		setmalloctag(b, getcallerpc());
	}
	do {
		mprand(bits, gen, b);
	} while(mpmagcmp(b, n) >= 0);

	return b;
}
