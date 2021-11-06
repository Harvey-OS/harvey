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

static	ulong	rng_vec[LEN];
static	ulong*	rng_tap = rng_vec;
static	ulong*	rng_feed = 0;
static	Lock	lk;

static void
isrand(long seed)
{
	long lo, hi, x;
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
srand(long seed)
{
	lock(&lk);
	isrand(seed);
	unlock(&lk);
}

long
lrand(void)
{
	ulong x;

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
