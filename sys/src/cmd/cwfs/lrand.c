/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "all.h"

/*
 *	algorithm by
 *	D. P. Mitchell & J. A. Reeds
 */
enum {
	LEN	= 607,
	TAP	= 273,
	MASK	= 0x7fffffffL,
	A	= 48271,
	M	= 2147483647,
	Q	= 44488,
	R	= 3399,
};

#define	NORM	(1.0/(1.0+MASK))

static	u32	rng_vec[LEN];
static	u32*	rng_tap = rng_vec;
static	u32*	rng_feed = 0;
static	Lock	lk;

static void
isrand(i32 seed)
{
	i32 lo, hi, x;
	int i;

	rng_tap = rng_vec;
	rng_feed = rng_vec+LEN-TAP;
	seed %= M;
	if(seed < 0)
		seed += M;
	if(seed == 0)
		seed = 89482311;
	x = seed;
	/*
	 *	Initialize by x[n+1] = 48271 * x[n] mod (2**31 - 1)
	 */
	for(i = -20; i < LEN; i++) {
		hi = x / Q;
		lo = x % Q;
		x = A*lo - R*hi;
		if(x < 0)
			x += M;
		if(i >= 0)
			rng_vec[i] = x;
	}
}

void
srand(i32 seed)
{
	lock(&lk);
	isrand(seed);
	unlock(&lk);
}

i32
lrand(void)
{
	u32 x;

	lock(&lk);

	rng_tap--;
	if(rng_tap < rng_vec) {
		if(rng_feed == 0) {
			isrand(1);
			rng_tap--;
		}
		rng_tap += LEN;
	}
	rng_feed--;
	if(rng_feed < rng_vec)
		rng_feed += LEN;
	x = (*rng_feed + *rng_tap) & MASK;
	*rng_feed = x;

	unlock(&lk);
	return x;
}
