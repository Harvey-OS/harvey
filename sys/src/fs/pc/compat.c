/*
 * fs kernel compatibility hacks for drivers from the cpu/terminal kernel
 */
#include "all.h"
#include "io.h"
#include "mem.h"
#include "compat.h"

void
free(void *p)		/* there's a struct member named "free".  sigh. */
{
	USED(p);
}

void *
mallocz(ulong sz, int clr)
{
	void *p = malloc(sz);

	if (clr && p != nil)
		memset(p, '\0', sz);
	return p;
}

void
freeb(Block *b)
{
	mbfree(b);
}

/*
 *  free a list of blocks
 */
void
freeblist(Block *b)
{
	Block *next;

	for(; b != 0; b = next){
		next = b->next;
		b->next = 0;
		freeb(b);
	}
}
