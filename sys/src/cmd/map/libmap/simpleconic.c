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

static double r0, a;

static int
Xsimpleconic(struct place *place, double *x, double *y)
{
	double r = r0 - place->nlat.l;
	double t = a*place->wlon.l;
	*x = -r*sin(t);
	*y = -r*cos(t);
	return 1;
}

proj
simpleconic(double par0, double par1)
{
	struct coord lat0;
	struct coord lat1;
	deg2rad(par0,&lat0);
	deg2rad(par1,&lat1);
	if(fabs(lat0.l+lat1.l)<.01)
		return rectangular(par0);
	if(fabs(lat0.l-lat1.l)<.01) {
		a = lat0.s/lat0.l;
		r0 = lat0.c/lat0.s + lat0.l;
	} else {
		a = (lat1.c-lat0.c)/(lat0.l-lat1.l);
		r0 = ((lat0.c+lat1.c)/a + lat1.l + lat0.l)/2;
	}
	return Xsimpleconic;
}
