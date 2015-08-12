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

static struct coord center;

static int
Xbicentric(struct place *place, double *x, double *y)
{
	if(place->wlon.c <= .01 || place->nlat.c <= .01)
		return (-1);
	*x = -center.c * place->wlon.s / place->wlon.c;
	*y = place->nlat.s / (place->nlat.c * place->wlon.c);
	return (*x * *x + *y * *y <= 9);
}

proj
bicentric(double l)
{
	l = fabs(l);
	if(l > 89)
		return (0);
	deg2rad(l, &center);
	return (Xbicentric);
}
