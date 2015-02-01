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

// sum = abs(b1) + abs(b2), i.e., add the magnitudes
void
mpmagadd(mpint *b1, mpint *b2, mpint *sum)
{
	int m, n;
	mpint *t;

	// get the sizes right
	if(b2->top > b1->top){
		t = b1;
		b1 = b2;
		b2 = t;
	}
	n = b1->top;
	m = b2->top;
	if(n == 0){
		mpassign(mpzero, sum);
		return;
	}
	if(m == 0){
		mpassign(b1, sum);
		return;
	}
	mpbits(sum, (n+1)*Dbits);
	sum->top = n+1;

	mpvecadd(b1->p, n, b2->p, m, sum->p);
	sum->sign = 1;

	mpnorm(sum);
}

// sum = b1 + b2
void
mpadd(mpint *b1, mpint *b2, mpint *sum)
{
	int sign;

	if(b1->sign != b2->sign){
		if(b1->sign < 0)
			mpmagsub(b2, b1, sum);
		else
			mpmagsub(b1, b2, sum);
	} else {
		sign = b1->sign;
		mpmagadd(b1, b2, sum);
		if(sum->top != 0)
			sum->sign = sign;
	}
}
