/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * 64-bit IEEE not-a-number routines.
 * This is big/little-endian portable assuming that
 * the 64-bit doubles and 64-bit integers have the
 * same byte ordering.
 */

#include "nan.h"

typedef unsigned long long uvlong;
typedef unsigned long ulong;

static uint64_t uvnan = 0x7FF0000000000001LL;
static uint64_t uvinf = 0x7FF0000000000000LL;
static uint64_t uvneginf = 0xFFF0000000000000LL;

double
__NaN(void)
{
	uint64_t* p;

	/* gcc complains about "return *(double*)&uvnan;" */
	p = &uvnan;
	return *(double*)p;
}

int
__isNaN(double d)
{
	uint64_t x;
	double* p;

	p = &d;
	x = *(uint64_t*)p;
	return (uint32_t)(x >> 32) == 0x7FF00000 && !__isInf(d, 0);
}

double
__Inf(int sign)
{
	uint64_t* p;

	if(sign < 0)
		p = &uvinf;
	else
		p = &uvneginf;
	return *(double*)p;
}

int
__isInf(double d, int sign)
{
	uint64_t x;
	double* p;

	p = &d;
	x = *(uint64_t*)p;
	if(sign == 0)
		return x == uvinf || x == uvneginf;
	else if(sign > 0)
		return x == uvinf;
	else
		return x == uvneginf;
}
