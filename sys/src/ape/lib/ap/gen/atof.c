#include	<stdlib.h>

double
atof(const char *s)
{
	return(strtod(s, (char **)0));
}
