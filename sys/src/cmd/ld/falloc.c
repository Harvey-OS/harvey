#include	"l.h"

/*
 * fake malloc
 */
void*
malloc(usize n)
{
	return halloc(n);
}

void
free(void *p)
{
	USED(p);
}

void*
calloc(usize m, usize n)
{
	void *p;

	n *= m;
	p = malloc(n);
	memset(p, 0, n);
	return p;
}

/*
 * not used by compiler or loader, but Windows needs it
 */
void*
realloc(void *p, usize n)
{
	void *new;

	new = malloc(n);
	if(new != nil && p != nil)
		memmove(new, p, n);	/* safe only when adjecent hunks have no gaps */
	return new;
}

void
setmalloctag(void *v, ulong pc)
{
	USED(v);
	USED(pc);
}
