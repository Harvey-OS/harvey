#include <stdlib.h>
#include <string.h>

void*
_MALLOCZ(int n, int clr)
{
	void *v;

	v = malloc(n);
	if(v && clr)
		memset(v, 0, n);
	return v;
}

