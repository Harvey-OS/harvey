#include <u.h>
#include <libc.h>

double
atof(char *cp)
{
	return strtod(cp, 0);
}
