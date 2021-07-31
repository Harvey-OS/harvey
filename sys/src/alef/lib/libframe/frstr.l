#include <alef.h>
#include <graphics.h>
#include <frame.h>

/*
 * The code here and elsewhere requires that strings not be gcalloc()ed
 */

#define	CHUNK	16
#define	ROUNDUP(n)	((n+CHUNK)&~(CHUNK-1))

char *
_frallocstr(uint n)
{
	char *p;

	p = malloc(ROUNDUP(n));
	if(p == nil)
		berror("out of memory");
	return p;
}

void
_frinsure(Frame *f, int bn, uint n)
{
	Frbox *b;
	char *p;

	b = &f->box[bn];
	if(b->nrune < 0)
		berror("_frinsure");

	if(ROUNDUP(b->nrune) > n)	/* > guarantees room for terminal NUL */
		return;

	p = _frallocstr(n);
	b = &f->box[bn];
	memmove(p, b->ptr, NBYTE(b)+1);
	free(b->ptr);
	b->ptr = p;
}
