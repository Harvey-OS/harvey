/*
	asin(arg) and acos(arg) return the arcsin, arccos,
	respectively of their arguments.

	Arctan is called after appropriate range reduction.
 */

#include <math.h>
#include <errno.h>

static double pio2	= 1.570796326794896619231e0;

double
asin(double arg)
{
	double temp;
	int sign;

	sign = 0;
	if(arg < 0) {
		arg = -arg;
		sign++;
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

	if(sign)
		temp = -temp;
	return temp;
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
