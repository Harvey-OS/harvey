#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"


void
qlock(QLock *q)
{
	Proc *p;

	if(canlock(&q->use))
		return;
	lock(&q->queue);
	if(canlock(&q->use)){
		unlock(&q->queue);
		return;
	}
	if(u == 0)
		panic("qlock");
	p = q->tail;
	if(p == 0)
		q->head = u->p;
	else
		p->qnext = u->p;
	q->tail = u->p;
	u->p->qnext = 0;
	u->p->state = Queueing;
/*	u->p->qlock = q;	/* DEBUG */
	unlock(&q->queue);
	sched();
}

int
canqlock(QLock *q)
{
	return canlock(&q->use);
}

void
qunlock(QLock *q)
{
	Proc *p;

	lock(&q->queue);
/*	u->p->qlock = 0; /* DEBUG */
	if(q->head){
		p = q->head;
		q->head = p->qnext;
		if(q->head == 0)
			q->tail = 0;
		unlock(&q->queue);
		ready(p);
	}else{
		unlock(&q->use);
		unlock(&q->queue);
	}
}
