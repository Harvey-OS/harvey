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

struct coord center;

static int
Xelliptic(struct place *place, double *x, double *y)
{
	double r1,r2;
	r1 = acos(place->nlat.c*(place->wlon.c*center.c
		- place->wlon.s*center.s));
	r2 = acos(place->nlat.c*(place->wlon.c*center.c
		+ place->wlon.s*center.s));
	*x = -(r1*r1 - r2*r2)/(4*center.l);
	*y = (r1*r1+r2*r2)/2 - (center.l*center.l+*x**x);
	if(*y < 0)
		*y = 0;
	*y = sqrt(*y);
	if(place->nlat.l<0)
		*y = -*y;
	return(1);
}

proj
elliptic(double l)
{
	l = fabs(l);
	if(l>89)
		return(0);
	if(l<1)
		return(Xazequidistant);
	deg2rad(l,&center);
	return(Xelliptic);
}
