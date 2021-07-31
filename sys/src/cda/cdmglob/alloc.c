#include "cdmglob.h"

#define MSIZE	8192
#define NSTACK	32


typedef struct {
	char	*m_start;
	char	*m_end;
	char	*m_brk;
} Minfo;



static Minfo	stack[NSTACK];
static char	*mend, *mstart;
static int	nstack;


char *
doalloc (int n)
{
	char	*x;
	unsigned int	size;

	if (n & 3)
		n = 4 + (n & ~3);
	if ((unsigned long) mstart & 3)
		mstart = (char *) (4 + ((unsigned long) mstart & ~3));
	if (mend - mstart < n) {
		size = ((n+3) / MSIZE + 1) * MSIZE;
		if ((mstart = sbrk (size)) != (char *) -1)
			mend = mstart + size;
		else	fatal ("out of space");
		if ((unsigned long) mstart & 3)
			mstart = (char *) (4 + ((unsigned long) mstart & ~3));
	}
	x = mstart;
	mstart += n;
	return (x);
}


void
popmem(void)
{
	if (--nstack < 0)
		fatal ("stack underflow");
	mstart = stack[nstack].m_start;
	return;
}


void
pushmem(void)
{
	if (nstack >= NSTACK)
		fatal ("stack overflow");
	stack[nstack].m_start = mstart;
	stack[nstack++].m_end = mend;
	return;
}


char *
strsave (char *s)
{
	register char	*t, *u;

again:
	for (t = s, u = mstart; u < mend && (*u++ = *t++););

	if (u < mend) {
		t = mstart;
		mstart = u;
		return (t);
	}
	mstart = doalloc (MSIZE);
	goto again;
}
