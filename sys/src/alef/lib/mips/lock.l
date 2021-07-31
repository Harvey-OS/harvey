#include <alef.h>
#include "run.h"

enum
{
	Pagesize	= 4096,
	Semperpg	= Pagesize/(16*sizeof(uint)),
	Lockaddr	= 0x60000000
};

intern int faketas;
int ALEF_tas(int*);

void
lockinit(void)
{
	int n;

	n = segattach(0, "lock", (void*)Lockaddr, Pagesize);
	if(n < 0) {
		faketas = 1;
		return;
	}
	memset((void*)Lockaddr, 0, Pagesize);
}

void
Lock.lock(Lock *lk)
{
	int *hwsem;
	int hash;

	if(faketas) {
		while(ALEF_tas(&lk->val))
			sleep(0);
		return;
	}

	/* Use low order lock bits to generate hash */
	hash = ((int)lk/sizeof(int)) & (Semperpg-1);
	hwsem = (int*)Lockaddr+hash;

	for(;;) {
		if((*hwsem & 1) == 0) {
			if(lk->val)
				*hwsem = 0;
			else {
				lk->val = 1;
				*hwsem = 0;
				return;
			}
		}
		while(lk->val)
			;
	}	
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
	tdb->unlock();

	if(tdb->ctask == nil)
		rendezvous(tdb, 0);

	s = ALEF_switch(me, tdb->ctask, nil);
	if(s)
		free(s);
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
