#include	"cc.h"
#include	"compat"

/*
 * fake mallocs
 */
void*
malloc(uintptr n)
{
	return alloc(n);
}

void*
calloc(uintptr m, uintptr n)
{
	return alloc(m*n);
}

void*
realloc(void*, uintptr)
{
	fprint(2, "realloc called\n");
	abort();
	return 0;
}

void
free(void*)
{
}

/* needed when profiling */
void*
mallocz(uintptr size, int clr)
{
	void *v;

	v = alloc(size);
	if(clr && v != nil)
		memset(v, 0, size);
	return v;
}

void
setmalloctag(void*, uintptr)
{
}
