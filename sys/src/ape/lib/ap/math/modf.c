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

/* modf suitable for IEEE double-precision */

#define MASK 0x7ffL
#define SIGN 0x80000000
#define SHIFT 20
#define BIAS 1022L

typedef union {
	double d;
	struct {
		int32_t ms;
		int32_t ls;
	} i;
} Cheat;

double
modf(double d, double* ip)
{
	Cheat x;
	int e;

	if(-1 < d && d < 1) {
		*ip = 0;
		return d;
	}
	x.d = d;
	x.i.ms &= ~SIGN;
	e = (x.i.ms >> SHIFT) & MASK;
	if(e == MASK || e == 0) {
		errno = EDOM;
		*ip = (d > 0) ? HUGE_VAL : -HUGE_VAL;
		return 0;
	}
	e -= BIAS;
	if(e <= SHIFT + 1) {
		x.i.ms &= ~(0x1fffffL >> e);
		x.i.ls = 0;
	} else if(e <= SHIFT + 33)
		x.i.ls &= ~(0x7fffffffL >> (e - SHIFT - 2));
	if(d > 0) {
		*ip = x.d;
		return d - x.d;
	} else {
		*ip = -x.d;
		return d + x.d;
	}
}
