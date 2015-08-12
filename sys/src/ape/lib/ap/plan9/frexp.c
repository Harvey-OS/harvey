/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <math.h>
#include <errno.h>
#define _RESEARCH_SOURCE
#include <float.h>

#define MASK 0x7ffL
#define SHIFT 20
#define BIAS 1022L
#define SIG 52

typedef union {
	double d;
	struct {
#ifdef IEEE_8087
		int32_t ls;
		int32_t ms;
#else
		int32_t ms;
		int32_t ls;
#endif
	};
} Cheat;

double
frexp(double d, int* ep)
{
	Cheat x, a;

	*ep = 0;
	/* order matters: only isNaN can operate on NaN */
	if(isNaN(d) || isInf(d, 0) || d == 0)
		return d;
	x.d = d;
	a.d = fabs(d);
	if((a.ms >> SHIFT) == 0) { /* normalize subnormal numbers */
		x.d = (double)(1ULL << SIG) * d;
		*ep = -SIG;
	}
	*ep = ((x.ms >> SHIFT) & MASK) - BIAS;
	x.ms &= ~(MASK << SHIFT);
	x.ms |= BIAS << SHIFT;
	return x.d;
}

double
ldexp(double d, int e)
{
	Cheat x;

	if(d == 0)
		return 0;
	x.d = d;
	e += (x.ms >> SHIFT) & MASK;
	if(e <= 0)
		return 0;
	if(e >= MASK) {
		errno = ERANGE;
		if(d < 0)
			return -HUGE_VAL;
		return HUGE_VAL;
	}
	x.ms &= ~(MASK << SHIFT);
	x.ms |= (int32_t)e << SHIFT;
	return x.d;
}

double
modf(double d, double* ip)
{
	double f;
	Cheat x;
	int e;

	x.d = d;
	e = (x.ms >> SHIFT) & MASK;
	if(e == MASK) {
		*ip = d;
		if(x.ls != 0 || (x.ms & 0xfffffL) != 0) /* NaN */
			return d;
		/* ±Inf */
		x.ms &= 0x80000000L;
		return x.d;
	}
	if(d < 1) {
		if(d < 0) {
			f = modf(-d, ip);
			*ip = -*ip;
			return -f;
		}
		*ip = 0;
		return d;
	}
	e -= BIAS;
	if(e <= SHIFT + 1) {
		x.ms &= ~(0x1fffffL >> e);
		x.ls = 0;
	} else if(e <= SHIFT + 33)
		x.ls &= ~(0x7fffffffL >> (e - SHIFT - 2));
	*ip = x.d;
	return d - x.d;
}
