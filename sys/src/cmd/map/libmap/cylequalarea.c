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

static double a;

static int
Xcylequalarea(struct place *place, double *x, double *y)
{
	*x = - place->wlon.l * a;
	*y = place->nlat.s;
	return(1);
}

proj
cylequalarea(double par)
{
	struct coord stdp0;
	if(par > 89.0)
		return(0);
	deg2rad(par, &stdp0);
	a = stdp0.c*stdp0.c;
	return(Xcylequalarea);
}
