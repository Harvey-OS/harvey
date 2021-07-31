#include <u.h>
#include <libc.h>

int
canlock(Lock *lk)
{
	return !tas(&lk->key);
}

void
lock(Lock *lk)
{
	int i;

	/* easy case */
	if(canlock(lk))
		return;

	/* for multi processor machines */
	for(i=0; i<100; i++)
		if(canlock(lk))
			return;

	for(i=0; i<100; i++) {
		osyield();
		if(canlock(lk))
			return;
	}

	/* looking bad - make sure it is not a priority problem */
	for(i=0; i<12; i++) {
		osmsleep(1<<i);
		if(canlock(lk))
			return;
	}

	/* we are in trouble */
	for(;;) {
		if(canlock(lk))
			return;
		iprint("lock loop %ld: val=%d &lock=%ux pc=%p\n", getpid(), lk->key, lk, getcallerpc(&lk));
		osmsleep(1000);
	}
}

void
unlock(Lock *lk)
{
	assert(lk->key);
	lk->key = 0;
}

void
ilock(Lock *lk)
{
	lock(lk);
}

void
iunlock(Lock *lk)
{
	unlock(lk);
}

