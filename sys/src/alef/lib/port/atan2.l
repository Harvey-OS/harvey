#include <alef.h>
/*
	atan2 discovers what quadrant the angle
	is in and calls atan.
*/

float
atan2(float arg1, float arg2)
{

	if(arg1+arg2 == arg1) {
		if(arg1 >= 0)
			return PIO2;
		return -PIO2;
	}
	arg1 = atan(arg1/arg2);
	if(arg2 < 0) {
		if(arg1 <= 0)
			return arg1 + PI;
		return arg1 - PI;
	}
	return arg1;
}
