#include "lib9.h"

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

	for(i=0; i<10; i++) {
		p9sleep(0);
		if(canlock(lk))
			return;
	}

	/* looking bad - make sure it is not a priority problem */
	for(i=0; i<12; i++) {
		p9sleep(1<<i);
		if(canlock(lk))
			return;
	}

	/* we are in trouble */
	for(;;) {
		if(canlock(lk))
			return;
		iprint("lock loop %ld: val=%d &lock=%ux pc=%ux\n", getpid(), lk->val, lk, getcallerpc(&lk));
		p9sleep(1000);
	}
}

void
unlock(Lock *lk)
{
	assert(lk->val);
	lk->val = 0;
}
