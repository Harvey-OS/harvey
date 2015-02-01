/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<stdlib.h>

/*
 *	algorithm by
 *	D. P. Mitchell & J. A. Reeds
 */
#define	LEN	607
#define	TAP	273
#define	MASK	0x7fffffffL
#define	A	48271
#define	M	2147483647
#define	Q	44488
#define	R	3399

typedef unsigned long	ulong;

static	ulong	rng_vec[LEN];
static	ulong*	rng_tap = rng_vec;
static	ulong*	rng_feed = 0;

void
srand(unsigned int seed)
{
	long lo, hi, x;
	int i;

	rng_tap = rng_vec;
	rng_feed = rng_vec+LEN-TAP;
	seed = seed%M;
	if(0 && seed < 0)			/* seed is unsigned */
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

static long
lrand(void)
{
	ulong x;

	rng_tap--;
        if(rng_tap < rng_vec) {
                if(rng_feed == 0) {
			srand(1);
			rng_tap--;
		}
                rng_tap += LEN;
        }
	rng_feed--;
        if(rng_feed < rng_vec)
                rng_feed += LEN;
	x = (*rng_feed + *rng_tap) & MASK;
	*rng_feed = x;
        return x;
}

int
rand(void)
{

	return lrand() & 0x7fff;
}
