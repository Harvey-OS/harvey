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
