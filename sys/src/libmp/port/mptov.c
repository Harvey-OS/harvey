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

#define VLDIGITS (sizeof(int64_t) / sizeof(mpdigit))

/*
 *  this code assumes that a vlong is an integral number of
 *  mpdigits long.
 */
mpint*
vtomp(int64_t v, mpint* b)
{
	int s;
	uint64_t uv;

	if(b == nil)
		b = mpnew(VLDIGITS * sizeof(mpdigit));
	else
		mpbits(b, VLDIGITS * sizeof(mpdigit));
	mpassign(mpzero, b);
	if(v == 0)
		return b;
	if(v < 0) {
		b->sign = -1;
		uv = -v;
	} else
		uv = v;
	for(s = 0; s < VLDIGITS && uv != 0; s++) {
		b->p[s] = uv;
		uv >>= sizeof(mpdigit) * 8;
	}
	b->top = s;
	return b;
}

int64_t
mptov(mpint* b)
{
	uint64_t v;
	int s;

	if(b->top == 0)
		return 0LL;

	mpnorm(b);
	if(b->top > VLDIGITS) {
		if(b->sign > 0)
			return (int64_t)MAXVLONG;
		else
			return (int64_t)MINVLONG;
	}

	v = 0ULL;
	for(s = 0; s < b->top; s++)
		v |= b->p[s] << (s * sizeof(mpdigit) * 8);

	if(b->sign > 0) {
		if(v > MAXVLONG)
			v = MAXVLONG;
	} else {
		if(v > MINVLONG)
			v = MINVLONG;
		else
			v = -(int64_t)v;
	}

	return (int64_t)v;
}
