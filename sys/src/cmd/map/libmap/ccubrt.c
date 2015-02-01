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

void
ccubrt(double zr, double zi, double *wr, double *wi)
{
	double r, theta;
	theta = atan2(zi,zr);
	r = cubrt(hypot(zr,zi));
	*wr = r*cos(theta/3);
	*wi = r*sin(theta/3);
}
