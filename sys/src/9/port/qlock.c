/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include <trace.h>

QLockstats qlockstats;

static void
lockstat(uintptr_t pc, uint64_t w)
{
	addwaitstat(pc, w, WSqlock);
}

static void
slockstat(uintptr_t pc, uint64_t w)
{
	addwaitstat(pc, w, WSslock);
}

void
qlock(QLock *q)
{
	Proc *up = externup();
	Proc *p;
	uint64_t t0;

	cycles(&t0);

	if(!islo() && machp()->ilockdepth != 0){
		print("qlock with ilockdepth %d,", machp()->ilockdepth);
		stacksnippet();
	}

	if(up != nil && up->nlocks)
		print("qlock: %#p: nlocks %d", getcallerpc(&q), up->nlocks);

	if(!canlock(&q->use)){
		lock(&q->use);
		slockstat(getcallerpc(&q), t0);
	}
	qlockstats.qlock++;
	if(!q->locked) {
		q->locked = 1;
		q->pc = getcallerpc(&q);
		unlock(&q->use);
		return;
	}
	if(up == nil)
		panic("qlock");
	qlockstats.qlockq++;
	p = q->tail;
	if(p == 0)
		q->head = up;
	else
		p->qnext = up;
	q->tail = up;
	up->qnext = 0;
	up->state = Queueing;
	up->qpc = getcallerpc(&q);
	if(up->trace)
		proctrace(up, SLock, 0);
	unlock(&q->use);
	sched();
	lockstat(getcallerpc(&q), t0);
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
	q->locked = 1;
	q->pc = getcallerpc(&q);
	unlock(&q->use);

	return 1;
}

void
qunlock(QLock *q)
{
	Proc *p;
	uint64_t t0;

	if(!canlock(&q->use)){
		cycles(&t0);
		lock(&q->use);
		slockstat(getcallerpc(&q), t0);
	}
	if (q->locked == 0)
		print("qunlock called with qlock not held, from %#p\n",
			getcallerpc(&q));
	p = q->head;
	if(p){
		q->head = p->qnext;
		if(q->head == 0)
			q->tail = 0;
		unlock(&q->use);
		q->pc = p->qpc;
		ready(p);
		return;
	}
	q->locked = 0;
	q->pc = 0;
	unlock(&q->use);
}

void
rlock(RWlock *q)
{
	Proc *up = externup();
	Proc *p;
	uint64_t t0;

	cycles(&t0);
	if(!canlock(&q->use)){
		lock(&q->use);
		slockstat(getcallerpc(&q), t0);
	}
	qlockstats.rlock++;
	if(q->writer == 0 && q->head == nil){
		/* no writer, go for it */
		q->readers++;
		unlock(&q->use);
		return;
	}

	qlockstats.rlockq++;
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
	if(up->trace)
		proctrace(up, SLock, 0);
	unlock(&q->use);
	sched();
	lockstat(getcallerpc(&q), t0);
}

void
runlock(RWlock *q)
{
	Proc *p;
	uint64_t t0;

	if(!canlock(&q->use)){
		cycles(&t0);
		lock(&q->use);
		slockstat(getcallerpc(&q), t0);
	}
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
	Proc *up = externup();
	Proc *p;
	uint64_t t0;

	cycles(&t0);
	if(!canlock(&q->use)){
		lock(&q->use);
		slockstat(getcallerpc(&q), t0);
	}
	qlockstats.wlock++;
	if(q->readers == 0 && q->writer == 0){
		/* noone waiting, go for it */
		q->wpc = getcallerpc(&q);
		q->wproc = up;
		q->writer = 1;
		unlock(&q->use);
		return;
	}

	/* wait */
	qlockstats.wlockq++;
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
	if(up->trace)
		proctrace(up, SLock, 0);
	unlock(&q->use);
	sched();
	lockstat(getcallerpc(&q), t0);
}

void
wunlock(RWlock *q)
{
	Proc *p;
	uint64_t t0;

	if(!canlock(&q->use)){
		cycles(&t0);
		lock(&q->use);
		slockstat(getcallerpc(&q), t0);
	}
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
	uint64_t t0;

	if(!canlock(&q->use)){
		cycles(&t0);
		lock(&q->use);
		slockstat(getcallerpc(&q), t0);
	}
	qlockstats.rlock++;
	if(q->writer == 0 && q->head == nil){
		/* no writer, go for it */
		q->readers++;
		unlock(&q->use);
		return 1;
	}
	unlock(&q->use);
	return 0;
}
