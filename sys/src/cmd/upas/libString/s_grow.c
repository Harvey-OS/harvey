#include <u.h>
#include <libc.h>
#include "String.h"

/* grow a String's allocation by at least `incr' bytes */
extern void
s_simplegrow(String *sp, int incr)	
{
	char *cp;
	int size;

	/*
	 *  take a larger increment to avoid mallocing too often
	 */
	size = sp->end-sp->base;
	if(size/2 < incr)
		size += incr;
	else
		size += size/2;

	cp = realloc(sp->base, size);
	if (cp == 0) {
		perror("String:");
		exits("realloc");
	}
	sp->ptr = (sp->ptr - sp->base) + cp;
	sp->end = cp + size;
	sp->base = cp;
}

/* grow a String's allocation */
extern int
s_grow(String *sp, int c)
{
	s_simplegrow(sp, 2);
	s_putc(sp, c);
	return c;
}
