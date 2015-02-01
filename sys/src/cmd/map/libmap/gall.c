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

static double scale;

static int
Xgall(struct place *place, double *x, double *y)
{
	/* two ways to compute tan(place->nlat.l/2) */
	if(fabs(place->nlat.s)<.1)
		*y = sin(place->nlat.l/2)/cos(place->nlat.l/2);
	else
		*y = (1-place->nlat.c)/place->nlat.s;
	*x = -scale*place->wlon.l;
	return 1;
}

proj
gall(double par)
{
	double coshalf;
	if(fabs(par)>80)
		return 0;
	par *= RAD;
	coshalf = cos(par/2);
	scale = cos(par)/(2*coshalf*coshalf);
	return Xgall;
}
