#include <alef.h>

/*
 * floating-point mod function without infinity or NaN checking
 */
float
fmod (float x, float y)
{
	int sign, yexp, rexp;
	float r, yfr, rfr;

	if (y == 0)
		return x;
	if (y < 0)
		y = -y;
	yfr = frexp(y, &yexp);
	sign = 0;
	if(x < 0) {
		r = -x;
		sign++;
	} else
		r = x;
	while(r >= y) {
		rfr = frexp(r, &rexp);
		r -= ldexp(y, rexp - yexp - (rfr < yfr));
	}
	if(sign)
		r = -r;
	return r;
}
