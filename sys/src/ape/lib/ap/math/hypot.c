/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <math.h>
/*
 * sqrt(a^2 + b^2)
 *	(but carefully)
 */

double
hypot(double a, double b)
{
	double t;

	if(a < 0)
		a = -a;
	if(b < 0)
		b = -b;
	if(a > b) {
		t = a;
		a = b;
		b = t;
	}
	if(b == 0) 
		return 0;
	a /= b;
	/*
	 * pathological overflow possible
	 * in the next line.
	 */
	return b * sqrt(1 + a*a);
}
