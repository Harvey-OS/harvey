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

#define Xaitwist Xaitpole.nlat
static struct place Xaitpole;

static int
Xaitoff(struct place *place, double *x, double *y)
{
	struct place p;
	copyplace(place,&p);
	p.wlon.l /= 2.;
	sincos(&p.wlon);
	norm(&p,&Xaitpole,&Xaitwist);
	Xazequalarea(&p,x,y);
	*x *= 2.;
	return(1);
}

proj
aitoff(void)
{
	latlon(0.,0.,&Xaitpole);
	return(Xaitoff);
}
