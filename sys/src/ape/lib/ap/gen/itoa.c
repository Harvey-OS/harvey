#include	<stdlib.h>

int
itoa(const char *s)
{
	return(strtol(s, (char **)0, 10));
}
