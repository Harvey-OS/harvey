#include	<stdlib.h>

int
atoi(const char *s)
{
	return(strtol(s, (char **)0, 10));
}
