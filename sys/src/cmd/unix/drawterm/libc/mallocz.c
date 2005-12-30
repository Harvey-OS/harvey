#include <u.h>
#include <libc.h>

void*
mallocz(ulong n, int clr)
{
	void *v;

	v = malloc(n);
	if(v && clr)
		memset(v, 0, n);
	return v;
}
