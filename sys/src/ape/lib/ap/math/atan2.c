#include <math.h>
#include <errno.h>
/*
	atan2 discovers what quadrant the angle
	is in and calls atan.
*/
#define pio2 1.5707963267948966192313217
#define pi   3.1415926535897932384626434;

double
atan2(double arg1, double arg2)
{

	if(arg1 == 0.0 && arg2 == 0.0){
		errno = EDOM;
		return 0.0;
	}
	if(arg1+arg2 == arg1) {
		if(arg1 >= 0)
			return pio2;
		return -pio2;
	}
	arg1 = atan(arg1/arg2);
	if(arg2 < 0) {
		if(arg1 <= 0)
			return arg1 + pi;
		return arg1 - pi;
	}
	return arg1;
}
