#include	<stdlib.h>

long
itol(const char *s)
{
	return(strtol(s, (char **)0, 10));
}
