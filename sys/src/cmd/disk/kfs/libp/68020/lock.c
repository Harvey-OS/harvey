#include	<u.h>
#include	<libc.h>
#include	<libp.h>

char	_tas(ulong *);

void
lockinit(void)
{
}

void
lock(Lock *l)
{
	while(!canlock(l))
		sleep(1);
}

void
unlock(Lock *l)
{
	l->lock = 0;
}

int
canlock(Lock *l)
{
	if(_tas(&l->lock))
		return 0;
	return 1;
}
