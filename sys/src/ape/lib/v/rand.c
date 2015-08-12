/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define _RESEARCH_SOURCE
#include <stdlib.h>
#include <libv.h>
/*
        random number generator from cacm 31 10, oct 88
        for 32 bit integers (called long here)
*/

#ifdef MAIN
#define A 16807
#define M 2147483647
#define Q 127773
#define R 2836
#else
#define A 48271
#define M 2147483647
#define Q 44488
#define R 3399
#endif

static int32_t seed = 1;

void
srand(unsigned int newseed)
{
	seed = newseed;
}

int32_t
lrand(void)
{
	int32_t lo, hi, test;

	hi = seed / Q;
	lo = seed % Q;
	test = A * lo - R * hi;
	if(test > 0)
		seed = test;
	else
		seed = test + M;
	return (seed);
}

int
rand(void)
{
	return lrand() % (RAND_MAX + 1);
}

#ifdef MAIN

main()
{
	int i;

	for(i = 0; i < 10000; i++)
		rand();
	if(seed == 1043618065)
		printf("     rand: pass\n");
	else
		printf("*****rand: fail; seed=%u, should be 1043618065\n",
		       seed);
	exit(0);
}
#endif
