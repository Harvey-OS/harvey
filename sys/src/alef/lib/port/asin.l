/*
 * asin(arg) and acos(arg) return the arcsin, arccos,
 * respectively of their arguments.
 *
 * Arctan is called after appropriate range reduction.
 */

#include <alef.h>

float
asin(float arg)
{
	int sign;
	float temp;

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

float
acos(float arg)
{
	if(arg > 1 || arg < -1)
		return NaN();
	return PIO2 - asin(arg);
}
