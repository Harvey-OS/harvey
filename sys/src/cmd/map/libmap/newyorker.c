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
Xnewyorker(struct place *place, double *x, double *y)
{
	double r = PI/2 - place->nlat.l;
	double s;
	if(r<.001)	/* cheat to plot center */
		s = 0;
	else if(r<a)
		return -1;
	else
		s = log(r/a);
	*x = -s * place->wlon.s;
	*y = -s * place->wlon.c;
	return(1);
}

proj
newyorker(double a0)
{
	a = a0*RAD;
	return(Xnewyorker);
}
