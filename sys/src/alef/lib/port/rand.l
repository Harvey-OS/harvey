#include	<alef.h>

/*
 *	algorithm by
 *	D. P. Mitchell & J. A. Reeds
 */

enum
{
	LEN	= 607,
	TAP	= 273,
	MASK	= 0x7fffffff,
	A	= 48271,
	M	= 2147483647,
	Q	= 44488,
	R	= 3399,
	NORM	= (1.0/(1.0+MASK))
};

intern	uint	rng_vec[LEN];
intern	uint*	rng_tap = rng_vec;
intern	uint*	rng_feed = nil;

void
srand(uint seed)
{
	uint lo, hi, x;
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

int
lrand()
{
	uint x;

	rng_tap--;
        if(rng_tap < rng_vec) {
                if(rng_feed == nil) {
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
nrand(int n)
{
	int slop, v;

	slop = MASK % n;
	do
		v = lrand();
	while(v <= slop);
	return v % n;
}

float
frand()
{
	float x;

	do {
		x = lrand() * NORM;
		x = (x + lrand()) * NORM;
	} while(x >= 1);
	return x;
}
