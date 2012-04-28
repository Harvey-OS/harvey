#include	"cc.h"
#include	"compat"

/*
 * fake mallocs
 */
void*
malloc(ulong n)
{
	return alloc(n);
}

void*
calloc(ulong m, ulong n)
{
	return alloc(m*n);
}

void*
realloc(void*u1, ulong u2)
{
	fprint(2, "realloc called\n");
	abort();
	return 0;
}

void
free(void*v)
{
}

/* needed when profiling */
void*
mallocz(ulong size, int clr)
{
	void *v;

	v = alloc(size);
	if(clr && v != nil)
		memset(v, 0, size);
	return v;
}

void
setmalloctag(void*u1, ulong u2)
{
}
