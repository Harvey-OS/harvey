/*
 * asin(arg) and acos(arg) return the arcsin, arccos,
 * respectively of their arguments.
 *
 * Arctan is called after appropriate range reduction.
 */

#include <u.h>
#include <libc.h>

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
	if(arg > 1)
		return NaN();
	temp = sqrt(1 - arg*arg);
	if(arg > 0.7)
		temp = PIO2 - atan(temp/arg);
	else
		temp = atan(arg/temp);
	if(sign)
		temp = -temp;
	return temp;
}

double
acos(double arg)
{
	if(arg > 1 || arg < -1)
		return NaN();
	return PIO2 - asin(arg);
}
