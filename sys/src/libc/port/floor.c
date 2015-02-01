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
/*
 * floor and ceil-- greatest integer <= arg
 * (resp least >=)
 */

double
floor(double d)
{
	double fract;

	if(d < 0) {
		fract = modf(-d, &d);
		if(fract != 0.0)
			d += 1;
		d = -d;
	} else
		modf(d, &d);
	return d;
}

double
ceil(double d)
{
	return -floor(-d);
}
