#include <string.h>
#include <ctype.h>
#include <stdlib.h>

char*
strdup(char *p)
{
	int n;
	char *np;

	n = strlen(p)+1;
	np = malloc(n);
	if(np)
		memmove(np, p, n);
	return np;
}
