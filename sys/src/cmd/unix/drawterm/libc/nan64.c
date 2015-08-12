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

#include <u.h>
#include <libc.h>
#include "fmtdef.h"

#if defined(__APPLE__) || (__powerpc__)
#define _NEEDLL
#endif

static uint64_t uvnan = ((uint64_t)0x7FF00000 << 32) | 0x00000001;
static uint64_t uvinf = ((uint64_t)0x7FF00000 << 32) | 0x00000000;
static uint64_t uvneginf = ((uint64_t)0xFFF00000 << 32) | 0x00000000;

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
	return (ulong)(x >> 32) == 0x7FF00000 && !__isInf(d, 0);
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
