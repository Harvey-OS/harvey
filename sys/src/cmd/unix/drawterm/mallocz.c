#include "lib9.h"

void*
mallocz(size_t n)
{
	void *p;

	p = malloc(n);
	memset(p, 0, n);

	return p;
}
