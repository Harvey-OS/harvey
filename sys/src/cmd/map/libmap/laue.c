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


static int
Xlaue(struct place *place, double *x, double *y)
{
	double r;
	if(place->nlat.l<PI/4+FUZZ)
		return(-1);
	r = tan(PI-2*place->nlat.l);
	if(r>3)
		return(-1);
	*x = - r * place->wlon.s;
	*y = - r * place->wlon.c;
	return(1);
}

proj
laue(void)
{
	return(Xlaue);
}
