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

int
Xazequidistant(struct place *place, double *x, double *y)
{
	double colat;
	colat = PI/2 - place->nlat.l;
	*x = -colat * place->wlon.s;
	*y = -colat * place->wlon.c;
	return(1);
}

proj
azequidistant(void)
{
	return(Xazequidistant);
}
