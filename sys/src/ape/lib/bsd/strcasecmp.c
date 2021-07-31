#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <bsd.h>

int
strcasecmp(char *a, char *b)
{
	int i;
	char *p;

	a = strdup(a);
	b = strdup(b);

	p = a;
	while(*p)
		*p++ = tolower(*p);
	p = b;
	while(*p)
		*p++ = tolower(*p);
	i = strcmp(a, b);
	free(a);
	free(b);
	return i;
}
