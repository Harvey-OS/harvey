#include	<u.h>
#include	<libc.h>
#include	<libp.h>

char	_tas(ulong *);
#define	SemPerPg	64		/* hardware semaphores per page */
#define By2Pg		4096

static ulong *locks;

void
lockinit(void)
{
	/*
	 * pick a stupid address because segattach doesn't
	 */
	locks = (ulong*)0x70000000;
	/*
	 * grab a lock segment if on the sgi
	 */
	if(segattach(0, "lock", locks, By2Pg) < 0){
		locks = 0;
		return;
	}
	memset(locks, 0, By2Pg);
}

void
lock(Lock *l)
{
	ulong *sbsem;

	if(!locks){
		while(_tas(&l->lock))
			sleep(0);
		return;
	}
	sbsem = (ulong*)locks + (ulong)l / sizeof(ulong*) % SemPerPg;
	for(;;){
		if(*sbsem & 1){
			if(l->lock)
				*sbsem = 0;
			else{
				l->lock = 1;
				*sbsem = 0;
				return;
			}
		}
		while(l->lock)
			;
	}
}

int
canlock(Lock *l)
{
	ulong *sbsem;

	if(!locks)
		return !_tas(&l->lock);
	sbsem = (ulong*)locks + (ulong)l / sizeof(Lock*) % SemPerPg;
	if(*sbsem & 1)
		return 0;
	if(l->lock){
		*sbsem = 0;
		return 0;
	}
	l->lock = 1;
	*sbsem = 0;
	return 1;
}

void
unlock(Lock *l)
{
	l->lock = 0;
}
