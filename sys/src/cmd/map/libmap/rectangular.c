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
Xrectangular(struct place *place, double *x, double *y)
{
	*x = -scale*place->wlon.l;
	*y = place->nlat.l;
	return(1);
}

proj
rectangular(double par)
{
	scale = cos(par*RAD);
	if(scale<.1)
		return 0;
	return(Xrectangular);
}
