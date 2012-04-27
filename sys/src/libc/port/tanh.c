#include <u.h>
#include <libc.h>

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
