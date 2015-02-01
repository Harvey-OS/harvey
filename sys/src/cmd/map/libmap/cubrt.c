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

double
cubrt(double a)
{
	double x,y,x1;
	if(a==0) 
		return(0.);
	y = 1;
	if(a<0) {
		y = -y;
		a = -a;
	}
	while(a<1) {
		a *= 8;
		y /= 2;
	}
	while(a>1) {
		a /= 8;
		y *= 2;
	}
	x = 1;
	do {
		x1 = x;
		x = (2*x1+a/(x1*x1))/3;
	} while(fabs(x-x1)>10.e-15);
	return(x*y);
}
