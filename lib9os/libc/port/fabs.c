#include <u.h>
#include <libc.h>

double
fabs(double arg)
{

	if(arg < 0)
		return -arg;
	return arg;
}
