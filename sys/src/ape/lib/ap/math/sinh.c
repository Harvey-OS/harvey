/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
	sinh(arg) returns the hyperbolic sine of its floating-
	point argument.

	The exponential function is called for arguments
	greater in magnitude than 0.5.

	A series is used for arguments smaller in magnitude than 0.5.
	The coefficients are #2029 from Hart & Cheney. (20.36D)

	cosh(arg) is computed from the exponential function for
	all arguments.
*/

#include <math.h>
#include <errno.h>

static double p0 = -0.6307673640497716991184787251e+6;
static double p1 = -0.8991272022039509355398013511e+5;
static double p2 = -0.2894211355989563807284660366e+4;
static double p3 = -0.2630563213397497062819489e+2;
static double q0 = -0.6307673640497716991212077277e+6;
static double q1 = 0.1521517378790019070696485176e+5;
static double q2 = -0.173678953558233699533450911e+3;

double
sinh(double arg)
{
	double temp, argsq;
	int sign;

	sign = 1;
	if(arg < 0) {
		arg = -arg;
		sign = -1;
	}
	if(arg > 21) {
		if(arg >= HUGE_VAL) {
			errno = ERANGE;
			temp = HUGE_VAL;
		} else
			temp = exp(arg) / 2;
		if(sign > 0)
			return temp;
		else
			return -temp;
	}

	if(arg > 0.5)
		return sign * (exp(arg) - exp(-arg)) / 2;

	argsq = arg * arg;
	temp = (((p3 * argsq + p2) * argsq + p1) * argsq + p0) * arg;
	temp /= (((argsq + q2) * argsq + q1) * argsq + q0);
	return sign * temp;
}

double
cosh(double arg)
{
	if(arg < 0)
		arg = -arg;
	if(arg > 21) {
		if(arg >= HUGE_VAL) {
			errno = ERANGE;
			return HUGE_VAL;
		} else
			return (exp(arg) / 2);
	}
	return (exp(arg) + exp(-arg)) / 2;
}
