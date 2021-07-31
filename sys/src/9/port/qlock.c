#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"


void
qlock(QLock *q)
{
	Proc *p, *mp;

	if(u)
		u->p->qlockpc = getcallerpc(((uchar*)&q) - sizeof(q));

	lock(&q->use);
	if(!q->locked) {
		q->locked = 1;
		unlock(&q->use);
		return;
	}
	p = q->tail;
	mp = u->p;
	if(p == 0)
		q->head = mp;
	else
		p->qnext = mp;
	q->tail = mp;
	mp->qnext = 0;
	mp->state = Queueing;
	unlock(&q->use);
	sched();
}

int
canqlock(QLock *q)
{
	if(!canlock(&q->use))
		return 0;

	if(q->locked){
		unlock(&q->use);
		return 0;
	}
	if(u)
		u->p->qlockpc = getcallerpc(((uchar*)&q) - sizeof(q));
	q->locked = 1;
	unlock(&q->use);
	return 1;
}

void
qunlock(QLock *q)
{
	Proc *p;

	if(u)
		u->p->qlockpc = 0;
	lock(&q->use);
	p = q->head;
	if(p) {
		q->head = p->qnext;
		if(q->head == 0)
			q->tail = 0;
		unlock(&q->use);
		ready(p);
		return;
	}
	q->locked = 0;
	unlock(&q->use);
}

void
rlock(RWlock *l)
{
	qlock(&l->x);		/* wait here for writers and exclusion */
	lock(l);
	l->readers++;
	canqlock(&l->k);	/* block writers if we are the first reader */
	unlock(l);
	qunlock(&l->x);
}

void
runlock(RWlock *l)
{
	lock(l);
	if(--l->readers == 0)	/* last reader out allows writers */
		qunlock(&l->k);
	unlock(l);
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
