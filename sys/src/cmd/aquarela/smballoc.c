#include "headers.h"

#ifndef LEAK
void *
smbemallocz(ulong size, int clear)
{
	void *p = nbemalloc(size);
	if (clear && p)
		memset(p, 0, size);
	return p;
}

void *
smbemalloc(ulong size)
{
	return smbemallocz(size, 0);
}

char *
smbestrdup(char *p)
{
	char *q;
	q = smbemalloc(strlen(p) + 1);
	return strcpy(q, p);
}
#endif

void
smbfree(void **pp)
{
	void *p = *pp;
	if (p) {
		free(p);
		*pp = nil;
	}
}

void
smberealloc(void **pp, ulong size)
{
	*pp = realloc(*pp, size);
	assert(size == 0 || *pp);
}
