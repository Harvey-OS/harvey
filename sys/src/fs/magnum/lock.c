#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"mem.h"

void
lock(Lock *l)
{
	int i;

	/*
	 * Try the fast grab first
	 */
    	if (tas(l) == 0)
		return;
	for (i = 0; i < 1000000; i++) {
    		if (tas(l) == 0)
			return;
	}
	l->sbsem = 0;

	panic("lock loop 0x%lux held by pc 0x%lux\n", l, l->pc);
}

int
canlock(Lock *l)
{
	if (tas(l) == 0)
		return 1;
	return 0;
}

void
unlock(Lock *l)
{
	l->pc = 0;
	l->sbsem = 0;
}

void
qlock(QLock *q)
{
	User *p;
	int i;

	lock(q);
	if(!q->locked){
		q->locked = 1;
		unlock(q);
		goto out;
	}
	p = q->tail;
	if(p == 0)
		q->head = u;
	else
		p->qnext = u;
	q->tail = u;
	u->qnext = 0;
	u->state = Queueing;
	u->has.want = q;
	unlock(q);
	sched();
	u->has.want = 0;

out:
	if(1 && u) {
		for(i=0; i<NHAS; i++)
			if(u->has.q[i] == 0) {
				u->has.q[i] = q;
				return;
			}
		print("NHAS(%d) too small\n", NHAS);
	}
}

int
canqlock(QLock *q)
{
	int i;

	lock(q);
	if(q->locked){
		unlock(q);
		return 0;
	}
	q->locked = 1;
	unlock(q);

	if(1 && u) {
		for(i=0; i<NHAS; i++)
			if(u->has.q[i] == 0) {
				u->has.q[i] = q;
				return 1;
			}
		print("NHAS(%d) too small\n", NHAS);
	}
	return 1;
}

void
qunlock(QLock *q)
{
	User *p;
	int i;

	lock(q);
	p = q->head;
	if(p) {
		q->head = p->qnext;
		if(q->head == 0)
			q->tail = 0;
		unlock(q);
		ready(p);
	} else {
		q->locked = 0;
		unlock(q);
	}

	if(1 && u) {
		for(i=0; i<NHAS; i++)
			if(u->has.q[i] == q) {
				u->has.q[i] = 0;
				return;
			}
		print("qunlock: not there %lux\n", q);
	}
}

/*
 * readers/writers lock
 * allows 1 writer or many readers
 */
void
rlock(RWlock *l)
{
	QLock *q;

	qlock(&l->wr);			/* wait here for writers and exclusion */

	q = &l->rd;			/* first reader in, qlock(&l->rd) */
	lock(q);
	q->locked = 1;
	l->nread++;
	unlock(q);

	qunlock(&l->wr);
}

void
runlock(RWlock *l)
{
	QLock *q;
	User *p;
	int n;

	q = &l->rd;
	lock(q);
	n = l->nread - 1;
	l->nread = n;
	if(n == 0) {			/* last reader out, qunlock(&l->rd) */
		p = q->head;
		if(p) {
			q->head = p->qnext;
			if(q->head == 0)
				q->tail = 0;
			unlock(q);
			ready(p);
			return;
		} 
		q->locked = 0;
	}
	unlock(q);
}

void
wlock(RWlock *l)
{
	qlock(&l->wr);			/* wait here for writers and exclusion */
	qlock(&l->rd);			/* wait here for last reader */
}

void
wunlock(RWlock *l)
{
	qunlock(&l->rd);
	qunlock(&l->wr);
}
