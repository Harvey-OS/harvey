#include	"l.h"

/*
 * fake malloc
 */
void*
malloc(uintptr n)
{
	void *p;

	while(n & 7)
		n++;
	while(nhunk < n)
		gethunk();
	p = hunk;
	nhunk -= n;
	hunk += n;
	return p;
}

void*
mallocz(uintptr n, int clr)
{
	void *p;

	while(n & 7)
		n++;
	while(nhunk < n)
		gethunk();
	p = hunk;
	if (clr)
		memset(p, 0, n);
	nhunk -= n;
	hunk += n;
	return p;
}

void
free(void *p)
{
	USED(p);
}

void*
calloc(uintptr m, uintptr n)
{
	void *p;

	n *= m;
	p = malloc(n);
	memset(p, 0, n);
	return p;
}

void*
realloc(void*, uintptr)
{
	fprint(2, "realloc called\n");
	abort();
	return 0;
}

void
setmalloctag(void *v, uintptr pc)
{
	USED(v, pc);
}

void*
mysbrk(ulong size)
{
	return sbrk(size);
}
