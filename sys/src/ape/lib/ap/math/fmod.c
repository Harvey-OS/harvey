/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* floating-point mod function without infinity or NaN checking */
#include <math.h>
double
fmod (double x, double y)
{
	int sign = 0, yexp;
	double r, yfr;

	if (y == 0)
		return 0;
	if (y < 0)
		y = -y;
	yfr = frexp (y, &yexp);
	if (x < 0) {
		sign = 1;
		r = -x;
	} else
		r = x;
	while (r >= y) {
		int rexp;
		double rfr = frexp (r, &rexp);
		r -= ldexp (y, rexp - yexp - (rfr < yfr));
	}
	if (sign)
		r = -r;
	return r;
}
