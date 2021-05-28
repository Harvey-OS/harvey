/* simple memory allocation */
#include "include.h"

#undef malloc
#undef free

void *
malloc(ulong n)
{
	return ialloc(n, 8);
}

void
free(void *)
{
}

void*
mallocz(ulong size, int clr)
{
	void *v;

	v = malloc(size);
	if(clr && v != nil)
		memset(v, 0, size);
	return v;
}

void*
realloc(void *v, ulong size)
{
	USED(v, size);
	panic("realloc called");
	return 0;
}

void*
calloc(ulong n, ulong szelem)
{
	return mallocz(n * szelem, 1);
}

void
setmalloctag(void *v, ulong pc)
{
	USED(v, pc);
}

void
setrealloctag(void *v, ulong pc)
{
	USED(v, pc);
}

ulong
getmalloctag(void *v)
{
	USED(v);
	assert(0);
	return ~0;
}

ulong
getrealloctag(void *v)
{
	USED(v);
	assert(0);
	return ~0;
}

void*
malloctopoolblock(void *v)
{
	USED(v);
	return nil;
}
