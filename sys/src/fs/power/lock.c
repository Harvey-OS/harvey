#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"mem.h"

/*
 * The hardware semaphores are strange.  64 per page, replicated 16 times
 * per page, 1024 pages of them.  Only the low bit is meaningful.
 * Reading an unset semaphore sets the semaphore and returns the old value.
 * Writing a semaphore sets the value, so writing 0 resets (clears) the semaphore.
 */

#define	SEMPERPG	64		/* hardware semaphores per page */
#define	NONSEMPERPG	(WD2PG-64)	/* words of non-semaphore per page */

struct
{
	Lock	lock;			/* lock to allocate */
	ulong	*nextsem;		/* next one to allocate */
	int	nsem;			/* at SEMPERPG, jump to next page */
} semalloc;

void
lockinit(void)
{
	semalloc.lock.sbsem = SBSEM;
	semalloc.nextsem = SBSEM+1;
	semalloc.nsem = 1;
	unlock(&semalloc.lock);
}

/*
 * If l->sbsem is zero, allocate a hardware semaphore first.
 * There is no way to free a semaphore.
 */
void
lock(Lock *l)
{
	int i, j, k;
	ulong *sbsem, pcp, s;

	sbsem = l->sbsem;
	if(sbsem == 0) {
		lock(&semalloc.lock);
		if(semalloc.nsem == SEMPERPG) {
			semalloc.nsem = 0;
			semalloc.nextsem += NONSEMPERPG;
			if(semalloc.nextsem == SBSEMTOP)
				panic("sem");
		}
		l->sbsem = semalloc.nextsem;
		semalloc.nextsem++;
		semalloc.nsem++;
		unlock(&semalloc.lock);
		unlock(l);		/* put sem in known state */
		sbsem = l->sbsem;
	}
	/*
	 * Try the fast grab first
	 */
	pcp = getcallerpc();
	if((*sbsem&1) == 0) {
		l->pc = pcp;
		return;
	}

loop:
	for(i=0; i<100000; i++) {
		for(j=0; j<100; j++) {
			for(k=0; k<50; k++)
				;
    			if((*sbsem&1) == 0) {
				l->pc = pcp;
				return;
			}
		}

		s = getstatus();
		if(s & IEC)	/* were we spllo */
			sched();
	}

	dumpstack(u);
	print("lock loop %lux pc %lux held by pc %lux\n",
		l, pcp, l->pc);
	goto loop;
}

void
ilock(Lock *l)
{
	l->sr = splhi();
	lock(l);
}

void
iunlock(Lock *l)
{
	ulong sr;

	sr = l->sr;
	l->pc = 0;
	*l->sbsem = 0;
	splx(sr);
}

int
canlock(Lock *l)
{
	ulong *sbsem;

	sbsem = l->sbsem;
	if(sbsem == 0){
		lock(&semalloc.lock);
		if(semalloc.nsem == SEMPERPG){
			semalloc.nsem = 0;
			semalloc.nextsem += NONSEMPERPG;
			if(semalloc.nextsem == SBSEMTOP)
				panic("sem");
		}
		l->sbsem = semalloc.nextsem;
		semalloc.nextsem++;
		semalloc.nsem++;
		unlock(&semalloc.lock);
		unlock(l);		/* put sem in known state */
		sbsem = l->sbsem;
	}
	if(*sbsem & 1)
		return 0;
	l->pc = getcallerpc();
	return 1;
}

void
unlock(Lock *l)
{
	l->pc = 0;
	*l->sbsem = 0;
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
