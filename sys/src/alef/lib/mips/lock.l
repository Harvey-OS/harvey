#include <alef.h>
#include "run.h"

enum
{
	Pagesize	= 4096,
	Semperpg	= Pagesize/(16*sizeof(uint)),
	Lockaddr	= 0x60000000,

	POWER		= 0x320,
	MAGNUM		= 0x330,
	MAGNUMII	= 0x340,
	R4K		= 0x500,
};

intern int arch;
int ALEF_3ktas(int*);
int ALEF_4ktas(int*);
int ALEF_fcr0();

void
ALEF_lockinit()
{
	int n;

	arch = ALEF_fcr0();
	switch(arch) {
	case POWER:
		n = segattach(0, "lock", (void*)Lockaddr, Pagesize);
		if(n < 0) {
			arch = MAGNUM;
			break;
		}
		memset((void*)Lockaddr, 0, Pagesize);
		break;
	case MAGNUM:
	case MAGNUMII:
	case R4K:
		break;
	default:
		check 0;
	}
	
}

void
ALEF_lockrele()
{
	switch(arch) {
	case POWER:
		segdetach((void*)Lockaddr);
		break;
	case MAGNUM:
	case MAGNUMII:
	case R4K:
		break;
	default:
		check 0;
	}
}

void
Lock.lock(Lock *lk)
{
	int *hwsem;
	int hash;

	switch(arch) {
	case MAGNUM:
	case MAGNUMII:
		while(ALEF_3ktas(&lk->val))
			sleep(0);
		return;
	case R4K:
		for(;;){
			while(lk->val)
				;
			if(ALEF_4ktas(&lk->val) == 0)
				return;
		}
		break;
	case POWER:
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
}

int
Lock.canlock(Lock *lk)
{
	int *hwsem;
	int hash;

	switch(arch) {
	case MAGNUM:
	case MAGNUMII:
		if(ALEF_3ktas(&lk->val))
			return 0;
		return 1;
	case R4K:
		if(ALEF_4ktas(&lk->val))
			return 0;
		return 1;
	case POWER:
		/* Use low order lock bits to generate hash */
		hash = ((int)lk/sizeof(int)) & (Semperpg-1);
		hwsem = (int*)Lockaddr+hash;

		if((*hwsem & 1) == 0) {
			if(lk->val)
				*hwsem = 0;
			else {
				lk->val = 1;
				*hwsem = 0;
				return 1;
			}
		}
		return 0;
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
