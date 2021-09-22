/*
 * queueing read, write, and both locks
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

struct {
	ulong rlock;
	ulong rlockq;
	ulong wlock;
	ulong wlockq;
	ulong qlock;
	ulong qlockq;
} rwstats;

static void
userqlock(QLock *lck, char *func, uintptr pc)
{
	panic("%s: user-space QLock* %#p; called from %#p", func, lck, pc);
}

static void
userrwlock(RWlock *lck, char *func, uintptr pc)
{
	panic("%s: user-space Rwlock* %#p; called from %#p", func, lck, pc);
}

void
qlock(QLock *q)
{
	int nlocks;
	uintptr caller;
	Proc *p;

	caller = getcallerpc(&q);
	if (isuseraddr(q))
		userqlock(q, "qlock", caller);
	if(m->ilockdepth != 0)
		panic("qlock: pc %#p: ilockdepth %d", caller, m->ilockdepth);
//		print("qlock: pc %#p: ilockdepth %d\n", caller, m->ilockdepth);
	coherence();		/* ensure changes from other cpus are visible */
	/*
	 * we get called with up==nil, during devtabreset
	 * (e.g., in ether reset routines for ctlr->alock).
	 */
	nlocks = up? up->nlocks: 0;;
	/*
	 * when we allowed this test, we hit it under load on the apu2 at least.
	 */
	if(0 && nlocks)		/* extra debugging: see syscall, kexit */
//		panic("qlock: caller %#p: nlocks %d", caller, nlocks);
		print("qlock: caller %#p: nlocks %d\n", caller, nlocks);
	USED(nlocks);

	lock(&q->use);
	rwstats.qlock++;
	if(!q->locked) {
		q->locked = 1;			/* got qlock on first try */
		unlock(&q->use);
		return;
	}

	/* add this process to this qlock's queue, switch to another process */
	if(up == nil)
		panic("qlock: nil up");
	rwstats.qlockq++;
	p = q->tail;
	if(p == 0)
		q->head = up;
	else
		p->qnext = up;
	q->tail = up;
	up->qnext = 0;
	up->state = Queueing;
	up->qpc = caller;
	unlock(&q->use);
	sched();
}

int
canqlock(QLock *q)
{
	if (isuseraddr(q))
		userqlock(q, "canqlock", getcallerpc(&q));
	if(!canlock(&q->use))
		return 0;

	/* have the q->use lock */
	if(q->locked){
		unlock(&q->use);
		return 0;
	}
	q->locked = 1;
	unlock(&q->use);
	return 1;
}

/* ready calls idlewake to wake up idling cpus probably waiting for qlocks. */
void
qunlock(QLock *q)
{
	Proc *p;

	if (isuseraddr(q))
		userqlock(q, "qunlock", getcallerpc(&q));
	lock(&q->use);
	if (q->locked == 0)
		print("qunlock called with qlock not held, from %#p\n",
			getcallerpc(&q));
	p = q->head;
	if(p){			/* other procs waiting for this qlock? */
		q->head = p->qnext;
		if(q->head == 0)
			q->tail = 0;
		unlock(&q->use);
		ready(p);	/* enable proc at head of q */
	} else {
		q->locked = 0;
		unlock(&q->use);
	}
}

void
rlock(RWlock *q)
{
	Proc *p;

	if (isuseraddr(q))
		userrwlock(q, "rlock", getcallerpc(&q));
	lock(&q->use);
	rwstats.rlock++;
	if(q->writer == 0 && q->head == nil){
		/* no writer, go for it */
		q->readers++;
		unlock(&q->use);
		return;
	}

	rwstats.rlockq++;
	p = q->tail;
	if(up == nil)
		panic("rlock");
	if(p == 0)
		q->head = up;
	else
		p->qnext = up;
	q->tail = up;
	up->qnext = 0;
	up->state = QueueingR;
	unlock(&q->use);
	sched();
}

void
runlock(RWlock *q)
{
	Proc *p;

	if (isuseraddr(q))
		userrwlock(q, "runlock", getcallerpc(&q));
	lock(&q->use);
	p = q->head;
	if(--(q->readers) > 0 || p == nil){
		unlock(&q->use);
		return;
	}

	/* start waiting writer */
	if(p->state != QueueingW)
		panic("runlock");
	q->head = p->qnext;
	if(q->head == 0)
		q->tail = 0;
	q->writer = 1;
	unlock(&q->use);
	ready(p);
}

void
wlock(RWlock *q)
{
	uintptr pc;
	Proc *p;

	pc = getcallerpc(&q);
	if (isuseraddr(q))
		userrwlock(q, "wlock", pc);
	lock(&q->use);
	rwstats.wlock++;
	if(q->readers == 0 && q->writer == 0){
		/* noone waiting, go for it */
		q->wpc = pc;
		q->wproc = up;
		q->writer = 1;
		unlock(&q->use);
		return;
	}

	/* wait */
	rwstats.wlockq++;
	p = q->tail;
	if(up == nil)
		panic("wlock");
	if(p == nil)
		q->head = up;
	else
		p->qnext = up;
	q->tail = up;
	up->qnext = 0;
	up->state = QueueingW;
	unlock(&q->use);
	sched();
}

void
wunlock(RWlock *q)
{
	Proc *p;

	if (isuseraddr(q))
		userrwlock(q, "wunlock", getcallerpc(&q));
	lock(&q->use);
	p = q->head;
	if(p == nil){
		q->writer = 0;
		unlock(&q->use);
		return;
	}
	if(p->state == QueueingW){
		/* start waiting writer */
		q->head = p->qnext;
		if(q->head == nil)
			q->tail = nil;
		unlock(&q->use);
		ready(p);
		return;
	}

	if(p->state != QueueingR)
		panic("wunlock");

	/* waken waiting readers */
	while(q->head != nil && q->head->state == QueueingR){
		p = q->head;
		q->head = p->qnext;
		q->readers++;
		ready(p);
	}
	if(q->head == nil)
		q->tail = nil;
	q->writer = 0;
	unlock(&q->use);
}

/* same as rlock but punts if there are any writers waiting */
int
canrlock(RWlock *q)
{
	if (isuseraddr(q))
		userrwlock(q, "canrlock", getcallerpc(&q));
	lock(&q->use);
	rwstats.rlock++;
	if(q->writer == 0 && q->head == nil){
		/* no writer, go for it */
		q->readers++;
		unlock(&q->use);
		return 1;
	}
	unlock(&q->use);
	return 0;
}
