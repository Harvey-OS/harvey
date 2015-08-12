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
Xpolyconic(struct place *place, double *x, double *y)
{
	double r, alpha;
	double lat2, lon2;
	if(fabs(place->nlat.l) > .01) {
		r = place->nlat.c / place->nlat.s;
		alpha = place->wlon.l * place->nlat.s;
		*y = place->nlat.l + r *(1 - cos(alpha));
		*x = -r *sin(alpha);
	} else {
		lon2 = place->wlon.l * place->wlon.l;
		lat2 = place->nlat.l * place->nlat.l;
		*y = place->nlat.l *(1 + (lon2 / 2) * (1 - (8 + lon2) * lat2 / 12));
		*x = -place->wlon.l *(1 - lat2 * (3 + lon2) / 6);
	}
	return (1);
}

proj
polyconic(void)
{
	return (Xpolyconic);
}
