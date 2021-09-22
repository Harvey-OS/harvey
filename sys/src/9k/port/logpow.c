/*
 * log2 and pow2 routines.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

/*
 * these routines should be cheap enough that there will
 * be no hesitation to use them.
 * in 9k, Clzbits is always >= 64.
 */

int
countbits(uvlong u)
{
	int n;

	for (n = 0; u != 0; n++)
		u &= ~(1ULL << (Clzbits - 1 - clz(u))); /* clear highest set bit */
	return n;
}

int
ispow2(uvlong uvl)
{
	return ISPOW2(uvl);
}

/*
 * return exponent of smallest power of 2 ≥ n
 */
int
log2(uvlong n)
{
	int i;

	i = Clzbits - 1 - clz(n);
	if (n == 0 || !ISPOW2(n))
		i++;
	return i;
}
