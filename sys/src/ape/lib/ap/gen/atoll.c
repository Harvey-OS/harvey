#include	<stdlib.h>

long long
atoll(const char *s)
{
	return(strtoll(s, (char **)0, 10));
}
