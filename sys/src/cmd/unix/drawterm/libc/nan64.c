/*
 * 64-bit IEEE not-a-number routines.
 * This is big/little-endian portable assuming that 
 * the 64-bit doubles and 64-bit integers have the
 * same byte ordering.
 */

#include <u.h>
#include <libc.h>
#include "nan.h"

// typedef unsigned long long uvlong;
// typedef unsigned long ulong;

static uvlong uvnan    = 0x7FF0000000000001ULL;
static uvlong uvinf    = 0x7FF0000000000000ULL;
static uvlong uvneginf = 0xFFF0000000000000ULL;

double
__NaN(void)
{
	return *(double*)(void*)&uvnan;
}

int
__isNaN(double d)
{
	uvlong x = *(uvlong*)(void*)&d;
	return (ulong)(x>>32)==0x7FF00000 && !__isInf(d, 0);
}

double
__Inf(int sign)
{
	if(sign < 0)
		return *(double*)(void*)&uvinf;
	else
		return *(double*)(void*)&uvneginf;
}

int
__isInf(double d, int sign)
{
	uvlong x;

	x = *(uvlong*)(void*)&d;
	if(sign == 0)
		return x==uvinf || x==uvneginf;
	else if(sign > 0)
		return x==uvinf;
	else
		return x==uvneginf;
}


