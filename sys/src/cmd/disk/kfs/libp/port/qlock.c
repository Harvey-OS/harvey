#include <u.h>
#include <libc.h>
#include <libp.h>

typedef struct Rendez	Rendez;

struct Rendez
{
	Rendez	*next;
};

static Rendez	*qrfree;
static Lock	qrlock;

void
qlock(QLock *q)
{
	Rendez *r, *rr;
	int i;

   spin:
	lock(q);
	if(!q->locked){
		q->locked = 1;
		unlock(q);
		return;
	}
	lock(&qrlock);
	r = qrfree;
	if(!r){
		rr = malloc(100 * sizeof *rr);
		if(rr == 0){
			unlock(&qrlock);
			unlock(q);
			sleep(1);
			goto spin;
		}
		for(i = 0; i < 100; i++){
			rr[i].next = r;
			r = &rr[i];
		}
	}
	qrfree = r->next;
	unlock(&qrlock);
	rr = q->tail;
	if(rr)
		rr->next = r;
	else
		q->head = r;
	q->tail = r;
	r->next = 0;
	unlock(q);
	while(rendezvous((ulong)r, 1) == ~0)
		;
}

int
canqlock(QLock *q)
{
	lock(q);
	if(q->locked){
		unlock(q);
		return 0;
	}
	q->locked = 1;
	unlock(q);
	return 1;
}

void
qunlock(QLock *q)
{
	Rendez *r;

	lock(q);
	r = q->head;
	if(r){
		q->head = r->next;
		if(!q->head)
			q->tail = 0;
		unlock(q);
		while(rendezvous((ulong)r, 1) == ~0)
			;
		lock(&qrlock);
		r->next = qrfree;
		qrfree = r;
		unlock(&qrlock);
	}else{
		q->locked = 0;
		unlock(q);
	}
}
