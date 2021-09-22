/*
	sqrt returns the square root of its floating
	point argument. Newton's method.

	calls frexp
*/

#include <u.h>
#include <libc.h>

double
sqrt(double arg)
{
	double x, temp;
	int exp, i, maxiters;

	if(isNaN(arg) || isInf(arg, 1))
		return arg;
	if(arg <= 0) {
		if(arg < 0)
			return NaN();
		return 0;
	}
	/* split into (exp, frac) parts; validate result */
	x = frexp(arg, &exp);
	if(isNaN(x) || isInf(x, 0))
		return x;
	maxiters = 1000;
	while(x < 0.5) {
		x *= 2;
		exp--;
		/* don't loop forever, even if fp errs don't generate notes */
		if (--maxiters <= 0)
			break;
	}
	/*
	 * NOTE
	 * this wont work on 1's comp
	 */
	if(exp & 1) {
		x *= 2;
		exp--;
	}
	temp = 0.5 * (1.0+x);

	while(exp > 60) {
		temp *= (1L<<30);
		exp -= 60;
		if (--maxiters <= 0)
			break;
	}
	while(exp < -60) {
		temp /= (1L<<30);
		exp += 60;
		if (--maxiters <= 0)
			break;
	}
	if(exp >= 0)
		temp *= 1L << (exp/2);
	else
		temp /= 1L << (-exp/2);
	for(i=0; i<=4; i++)
		temp = 0.5*(temp + arg/temp);
	return temp;
}
