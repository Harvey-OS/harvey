#include	<stdlib.h>

long
atol(const char *s)
{
	return(strtol(s, (char **)0, 10));
}
