#include <u.h>
#include <libc.h>
#include <thread.h>

static Lock l;

void
_xinc(long *p)
{

	lock(&l);
	(*p)++;
	unlock(&l);
}

long
_xdec(long *p)
{
	long r;

	lock(&l);
	r = --(*p);
	unlock(&l);
	return r;
}
