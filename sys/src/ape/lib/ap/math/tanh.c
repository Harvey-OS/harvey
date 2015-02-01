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
	tanh(arg) computes the hyperbolic tangent of its floating
	point argument.

	sinh and cosh are called except for large arguments, which
	would cause overflow improperly.
 */

double
tanh(double arg)
{

	if(arg < 0) {
		arg = -arg;
		if(arg > 21)
			return -1;
		return -sinh(arg)/cosh(arg);
	}
	if(arg > 21)
		return 1;
	return sinh(arg)/cosh(arg);
}
