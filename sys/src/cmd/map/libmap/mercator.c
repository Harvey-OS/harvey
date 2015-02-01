/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include "map.h"

static int
Xmercator(struct place *place, double *x, double *y)
{
	if(fabs(place->nlat.l) > 80.*RAD)
		return(-1);
	*x = -place->wlon.l;
	*y = 0.5*log((1+place->nlat.s)/(1-place->nlat.s));
	return(1);
}

proj
mercator(void)
{
	return(Xmercator);
}

static double ecc = ECC;

static int
Xspmercator(struct place *place, double *x, double *y)
{
	if(Xmercator(place,x,y) < 0)
		return(-1);
	*y += 0.5*ecc*log((1-ecc*place->nlat.s)/(1+ecc*place->nlat.s));
	return(1);
}

proj
sp_mercator(void)
{
	return(Xspmercator);
}
