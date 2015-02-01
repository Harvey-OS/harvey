/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"

static void
queue(Proc **first, Proc **last)
{
	Proc *t;

	t = *last;
	if(t == 0)
		*first = up;
	else
		t->qnext = up;
	*last = up;
	up->qnext = 0;
}

static Proc*
dequeue(Proc **first, Proc **last)
{
	Proc *t;

	t = *first;
	if(t == 0)
		return 0;
	*first = t->qnext;
	if(*first == 0)
		*last = 0;
	return t;
}

void
qlock(QLock *q)
{
	lock(&q->lk);

	if(q->hold == 0) {
		q->hold = up;
		unlock(&q->lk);
		return;
	}

	/*
	 * Can't assert this because of RWLock
	assert(q->hold != up);
	 */		

	queue((Proc**)&q->first, (Proc**)&q->last);
	unlock(&q->lk);
	procsleep();
}

int
canqlock(QLock *q)
{
	lock(&q->lk);
	if(q->hold == 0) {
		q->hold = up;
		unlock(&q->lk);
		return 1;
	}
	unlock(&q->lk);
	return 0;
}

void
qunlock(QLock *q)
{
	Proc *p;

	lock(&q->lk);
	/* 
	 * Can't assert this because of RWlock
	assert(q->hold == CT);
	 */
	p = dequeue((Proc**)&q->first, (Proc**)&q->last);
	if(p) {
		q->hold = p;
		unlock(&q->lk);
		procwakeup(p);
	} else {
		q->hold = 0;
		unlock(&q->lk);
	}
}

int
holdqlock(QLock *q)
{
	return q->hold == up;
}

