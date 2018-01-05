/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>

static struct {
	QLp	*p;
	QLp	x[1024];
} ql = {
	ql.x
};

enum
{
	Queuing,
	QueuingR,
	QueuingW,
	Sleeping,
};

static void*	(*_rendezvousp)(void*, void*) = rendezvous;

/* this gets called by the thread library ONLY to get us to use its rendezvous */
void
_qlockinit(void* (*r)(void*, void*))
{
	_rendezvousp = r;
}

/* find a free shared memory location to queue ourselves in */
static QLp*
getqlp(void)
{
	QLp *p, *op;

	op = ql.p;
	for(p = op+1; ; p++){
		if(p == &ql.x[nelem(ql.x)])
			p = ql.x;
		if(p == op)
			abort();
		if(_tas(&(p->inuse)) == 0){
			ql.p = p;
			p->next = nil;
			break;
		}
	}
	return p;
}

void
qlock(QLock *q)
{
	QLp *p, *mp;

	lock(&q->lock);
	if(!q->locked){
		q->locked = 1;
		unlock(&q->lock);
		return;
	}


	/* chain into waiting list */
	mp = getqlp();
	p = q->tail;
	if(p == nil)
		q->head = mp;
	else
		p->next = mp;
	q->tail = mp;
	mp->state = Queuing;
	unlock(&q->lock);

	/* wait */
	while((*_rendezvousp)(mp, (void*)1) == (void*)~0)
		;
	mp->inuse = 0;
}

void
qunlock(QLock *q)
{
	QLp *p;

	lock(&q->lock);
	if (q->locked == 0)
		fprint(2, "qunlock called with qlock not held, from %#p\n",
			getcallerpc());
	p = q->head;
	if(p != nil){
		/* wakeup head waiting process */
		q->head = p->next;
		if(q->head == nil)
			q->tail = nil;
		unlock(&q->lock);
		while((*_rendezvousp)(p, (void*)0x12345) == (void*)~0)
			;
		return;
	}
	q->locked = 0;
	unlock(&q->lock);
}

int
canqlock(QLock *q)
{
	if(!canlock(&q->lock))
		return 0;
	if(!q->locked){
		q->locked = 1;
		unlock(&q->lock);
		return 1;
	}
	unlock(&q->lock);
	return 0;
}

void
rlock(RWLock *q)
{
	QLp *p, *mp;

	lock(&q->lock);
	if(q->writer == 0 && q->head == nil){
		/* no writer, go for it */
		q->_readers++;
		unlock(&q->lock);
		return;
	}

	mp = getqlp();
	p = q->tail;
	if(p == 0)
		q->head = mp;
	else
		p->next = mp;
	q->tail = mp;
	mp->next = nil;
	mp->state = QueuingR;
	unlock(&q->lock);

	/* wait in kernel */
	while((*_rendezvousp)(mp, (void*)1) == (void*)~0)
		;
	mp->inuse = 0;
}

int
canrlock(RWLock *q)
{
	lock(&q->lock);
	if (q->writer == 0 && q->head == nil) {
		/* no writer; go for it */
		q->_readers++;
		unlock(&q->lock);
		return 1;
	}
	unlock(&q->lock);
	return 0;
}

void
runlock(RWLock *q)
{
	QLp *p;

	lock(&q->lock);
	if(q->_readers <= 0)
		abort();
	p = q->head;
	if(--(q->_readers) > 0 || p == nil){
		unlock(&q->lock);
		return;
	}

	/* start waiting writer */
	if(p->state != QueuingW)
		abort();
	q->head = p->next;
	if(q->head == 0)
		q->tail = 0;
	q->writer = 1;
	unlock(&q->lock);

	/* wakeup waiter */
	while((*_rendezvousp)(p, 0) == (void*)~0)
		;
}

void
wlock(RWLock *q)
{
	QLp *p, *mp;

	lock(&q->lock);
	if(q->_readers == 0 && q->writer == 0){
		/* noone waiting, go for it */
		q->writer = 1;
		unlock(&q->lock);
		return;
	}

	/* chain into waiting list */
	p = q->tail;
	mp = getqlp();
	if(p == nil)
		q->head = mp;
	else
		p->next = mp;
	q->tail = mp;
	mp->next = nil;
	mp->state = QueuingW;
	unlock(&q->lock);

	/* wait in kernel */
	while((*_rendezvousp)(mp, (void*)1) == (void*)~0)
		;

	mp->inuse = 0;
}

int
canwlock(RWLock *q)
{
	lock(&q->lock);
	if (q->_readers == 0 && q->writer == 0) {
		/* no one waiting; go for it */
		q->writer = 1;
		unlock(&q->lock);
		return 1;
	}
	unlock(&q->lock);
	return 0;
}

void
wunlock(RWLock *q)
{
	QLp *p;

	lock(&q->lock);
	if(q->writer == 0)
		abort();
	p = q->head;
	if(p == nil){
		q->writer = 0;
		unlock(&q->lock);
		return;
	}
	if(p->state == QueuingW){
		/* start waiting writer */
		q->head = p->next;
		if(q->head == nil)
			q->tail = nil;
		unlock(&q->lock);
		while((*_rendezvousp)(p, 0) == (void*)~0)
			;
		return;
	}

	if(p->state != QueuingR)
		abort();

	/* wake waiting readers */
	while(q->head != nil && q->head->state == QueuingR){
		p = q->head;
		q->head = p->next;
		q->_readers++;
		while((*_rendezvousp)(p, 0) == (void*)~0)
			;
	}
	if(q->head == nil)
		q->tail = nil;
	q->writer = 0;
	unlock(&q->lock);
}

void
rsleep(Rendez *r)
{
	QLp *t, *me;

	if(!r->l)
		abort();
	lock(&r->l->lock);
	/* we should hold the qlock */
	if(!r->l->locked)
		abort();

	/* add ourselves to the wait list */
	me = getqlp();
	me->state = Sleeping;
	if(r->head == nil)
		r->head = me;
	else
		r->tail->next = me;
	me->next = nil;
	r->tail = me;

	/* pass the qlock to the next guy */
	t = r->l->head;
	if(t){
		r->l->head = t->next;
		if(r->l->head == nil)
			r->l->tail = nil;
		unlock(&r->l->lock);
		while((*_rendezvousp)(t, (void*)0x12345) == (void*)~0)
			;
	}else{
		r->l->locked = 0;
		unlock(&r->l->lock);
	}

	/* wait for a wakeup */
	while((*_rendezvousp)(me, (void*)1) == (void*)~0)
		;
	me->inuse = 0;
}

int
rwakeup(Rendez *r)
{
	QLp *t;

	/*
	 * take off wait and put on front of queue
	 * put on front so guys that have been waiting will not get starved
	 */

	if(!r->l)
		abort();
	lock(&r->l->lock);
	if(!r->l->locked)
		abort();

	t = r->head;
	if(t == nil){
		unlock(&r->l->lock);
		return 0;
	}

	r->head = t->next;
	if(r->head == nil)
		r->tail = nil;

	t->next = r->l->head;
	r->l->head = t;
	if(r->l->tail == nil)
		r->l->tail = t;

	t->state = Queuing;
	unlock(&r->l->lock);
	return 1;
}

int
rwakeupall(Rendez *r)
{
	int i;

	for(i=0; rwakeup(r); i++)
		;
	return i;
}
