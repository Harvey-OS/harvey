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

static struct coord stdpar0, stdpar1;
static double k;
static double yeq;

static int
Xtrapezoidal(struct place *place, double *x, double *y)
{
	*y = yeq + place->nlat.l;
	*x = *y*k*place->wlon.l;
	return 1;
}

proj
trapezoidal(double par0, double par1)
{
	if(fabs(fabs(par0)-fabs(par1))<.1)
		return rectangular(par0);
	deg2rad(par0,&stdpar0);
	deg2rad(par1,&stdpar1);
	if(fabs(par1-par0) < .1)
		k = stdpar1.s;
	else
		k = (stdpar1.c-stdpar0.c)/(stdpar0.l-stdpar1.l);
	yeq = -stdpar1.l - stdpar1.c/k;
	return Xtrapezoidal;
}
