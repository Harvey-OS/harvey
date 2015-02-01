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

static struct coord stdpar;
static double r0;

static
Xbonne(struct place *place, double *x, double *y)
{
	double r, alpha;
	r = r0 - place->nlat.l;
	if(r<.001)
		if(fabs(stdpar.c)<1e-10)
			alpha = place->wlon.l;
		else if(fabs(place->nlat.c)==0)
			alpha = 0;
		else 
			alpha = place->wlon.l/(1+
				stdpar.c*stdpar.c*stdpar.c/place->nlat.c/3);
	else
		alpha = place->wlon.l * place->nlat.c / r;
	*x = - r*sin(alpha);
	*y = - r*cos(alpha);
	return(1);
}

proj
bonne(double par)
{
	if(fabs(par*RAD) < .01)
		return(Xsinusoidal);
	deg2rad(par, &stdpar);
	r0 = stdpar.c/stdpar.s + stdpar.l;
	return(Xbonne);
}
