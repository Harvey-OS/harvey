#include <alef.h>
#include "run.h"

int	tas(int*);

void
ALEF_lockinit()
{
}

void
ALEF_lockrele()
{
}

void
Lock.lock(Lock *lk)
{
	while(tas(&lk->val))
		;
}

int
Lock.canlock(Lock *lk)
{
	if(tas(&lk->val))
		return 0;
	return 1;
}

void
Lock.unlock(Lock *lk)
{
	lk->val = 0;
}

void
QLock.lock(QLock *q)
{
	void *s;
	Tdb *tdb;
	Task *me, *rh;

	q->use.lock();
	if(q->used == 0) {
		q->used++;
		q->use.unlock();
		return;
	}

	me = PROC.tdb->ctask;
	me->qlink = q->queue;
	q->queue = me;

	tdb = me->tdb;
	tdb->lock();
	q->use.unlock();

	rh = tdb->runhd;
	if(rh != nil)
		tdb->runhd = rh->link;
	tdb->ctask = rh;
	tdb->sleeper = rh == nil;
	tdb->unlock();

	if(rh == nil)
		rendezvous(tdb, 0);

	s = ALEF_switch(me, tdb->ctask, nil);
	if(s)
		free(s);
}

int
QLock.canlock(QLock *q)
{
	q->use.lock();
	if(q->used == 0) {
		q->used++;
		q->use.unlock();
		return 1;
	}
	q->use.unlock();
	return 0;
}

void
QLock.unlock(QLock *q)
{
	Task *t;

	q->use.lock();
	if(q->queue == nil) {
		q->used = 0;
		q->use.unlock();
		return;
	}
	t = q->queue;
	q->queue = t->qlink;
	q->use.unlock();

	ALEF_sched(t);
}
