#include <u.h>
#include <libc.h>

void
lock(Lock *l)
{
	if(ainc(&l->key) == 1)
		return;	/* changed from 0 -> 1: we hold lock */
	/* otherwise wait in kernel */
	while(semacquire(&l->sem, 1) < 0){
		/* interrupted; try again */
	}
}

void
unlock(Lock *l)
{
	if(adec(&l->key) == 0)
		return;	/* changed from 1 -> 0: no contention */
	semrelease(&l->sem, 1);
}

int
canlock(Lock *l)
{
	if(ainc(&l->key) == 1)
		return 1;	/* changed from 0 -> 1: success */
	/* Undo increment (but don't miss wakeup) */
	if(adec(&l->key) == 0)
		return 0;	/* changed from 1 -> 0: no contention */
	semrelease(&l->sem, 1);
	return 0;
}
