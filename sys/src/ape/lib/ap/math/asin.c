/*
	asin(arg) and acos(arg) return the arcsin, arccos,
	respectively of their arguments.

	Arctan is called after appropriate range reduction.
 */

#include <math.h>
#include <errno.h>

static double pio2	= 1.570796326794896619;

double
asin(double arg)
{
	double sign, temp;

	sign = 1;
	if(arg < 0) {
		arg = -arg;
		sign = -1;
	}

	if(arg > 1) {
		errno = EDOM;
		return 0;
	}

	temp = sqrt(1 - arg*arg);
	if(arg > 0.7)
		temp = pio2 - atan(temp/arg);
	else
		temp = atan(arg/temp);

	return sign*temp;
}

double
acos(double arg)
{
	if(arg > 1 || arg < -1) {
		errno = EDOM;
		return 0;
	}
	return pio2 - asin(arg);
}
