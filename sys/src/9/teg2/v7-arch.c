/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * arm arch v7 routines other than cache-related ones.
 *
 * calling this arch-v7.c would confuse the mk scripts,
 * to which a filename arch*.c is magic.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"
#include "arm.h"

/*
 * these routines should be cheap enough that there will
 * be no hesitation to use them.
 *
 * once 5c in-lines vlong ops, just use the vlong versions.
 */

/* see Hacker's Delight if this isn't obvious */
#define ISPOW2(i) (((i) & ((i) - 1)) == 0)

int
ispow2(uvlong uvl)
{
	/* see Hacker's Delight if this isn't obvious */
	return ISPOW2(uvl);
}

static int
isulpow2(ulong ul)				/* temporary speed hack */
{
	return ISPOW2(ul);
}

/*
 * return exponent of smallest power of 2 ≥ n
 */
int
log2(ulong n)
{
	int i;

	i = BI2BY*BY2WD - 1 - clz(n);
	if (n == 0 || !ISPOW2(n))
		i++;
	return i;
}
