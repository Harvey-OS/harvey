#include <u.h>
#include	"libp.h"

/*
 * readers/writers lock
 * allows 1 writer or many readers
 */
void
rlock(RWlock *l)
{
	qlock(&l->x);		/* wait here for writers and exclusion */
	lock(&l->ref);
	l->nread++;
	canqlock(&l->k);	/* block writers if we are the first reader */
	unlock(&l->ref);
	qunlock(&l->x);
}

void
runlock(RWlock *l)
{
	/*
	 * it is ok to hold the lock while qunlocking because either:
	 * 1) we will wake up a writer so no readers can be spinning on r
	 * 2) we will wake up nobody so we won't sched
	 */
	lock(&l->ref);
	if(l->nread-- == 1)	/* last reader out allows writers */
		qunlock(&l->k);
	unlock(&l->ref);
}

void
wlock(RWlock *l)
{
	qlock(&l->x);		/* wait here for writers and exclusion */
	qlock(&l->k);		/* wait here for last reader */
}

void
wunlock(RWlock *l)
{
	qunlock(&l->x);
	qunlock(&l->k);
}
