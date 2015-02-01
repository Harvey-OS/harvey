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
/* refractive fisheye, not logarithmic */

static double n;

static int
Xfisheye(struct place *place, double *x, double *y)
{
	double r;
	double u = sin(PI/4-place->nlat.l/2)/n;
	if(fabs(u) > .97)
		return -1;
	r = tan(asin(u));
	*x = -r*place->wlon.s;
	*y = -r*place->wlon.c;
	return 1;
}

proj
fisheye(double par)
{
	n = par;
	return n<.1? 0: Xfisheye;
}
