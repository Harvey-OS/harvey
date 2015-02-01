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
 * floating-point mod function without infinity or NaN checking
 */
double
fmod (double x, double y)
{
	int sign, yexp, rexp;
	double r, yfr, rfr;

	if (y == 0)
		return x;
	if (y < 0)
		y = -y;
	yfr = frexp(y, &yexp);
	sign = 0;
	if(x < 0) {
		r = -x;
		sign++;
	} else
		r = x;
	while(r >= y) {
		rfr = frexp(r, &rexp);
		r -= ldexp(y, rexp - yexp - (rfr < yfr));
	}
	if(sign)
		r = -r;
	return r;
}
