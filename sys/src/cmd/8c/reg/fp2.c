#include <u.h>
#include <libc.h>

double
fun(void)
{
	double g;

	g = 4215866817.;
	USED(g);
	return g;
}

void
main(void)
{
	int x;
	double g;

	x = (uint)fun();
	USED(x);
	g = 1.;
	exits("");
}
