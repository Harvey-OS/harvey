#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

Ref	pidalloc;
Ref	noteidalloc;

struct
{
	Lock;
	Proc	*arena;
	Proc	*free;
}procalloc;

struct
{
	Lock;
	Waitq	*free;
}waitqalloc;

typedef struct
{
	Lock;
	Proc	*head;
	Proc	*tail;
}Schedq;

Schedq	runhiq, runloq;
int	nrdy;

char *statename[] =
{			/* BUG: generate automatically */
	"Dead",
	"Moribund",
	"Ready",
	"Scheding",
	"Running",
	"Queueing",
	"Wakeme",
	"Broken",
	"Stopped",
	"Rendez",
};

/*
 * Always splhi()'ed.
 */
void
schedinit(void)		/* never returns */
{
	Proc *p;

	setlabel(&m->sched);
	if(u){
		m->proc = 0;
		p = u->p;
		invalidateu();	/* safety first */
		u = 0;
		if(p->state == Running)
			ready(p);
		else
		if(p->state == Moribund) {
			p->pid = 0;
			p->state = Dead;
			/* 
			 * Holding locks from pexit:
			 * 	procalloc, debug, palloc
			 */
			mmurelease(p);
			simpleputpage(p->upage);
			p->upage = 0;

			p->qnext = procalloc.free;
			procalloc.free = p;
		
			unlock(&palloc);
			qunlock(&p->debug);
			unlock(&procalloc);
		}
		p->mach = 0;
	}
	sched();
}

void
sched(void)
{
	Proc *p;

	if(u){
		splhi();
		m->cs++;
		procsave(u->p);
		if(setlabel(&u->p->sched)){	/* woke up */
			p = u->p;
			p->state = Running;
			p->mach = m;
			m->proc = p;
			procrestore(p);
			spllo();
			return;
		}
		gotolabel(&m->sched);
	}
	p = runproc();
	mapstack(p);
	gotolabel(&p->sched);
}

int
anyready(void)
{
	return runloq.head != 0 || runhiq.head != 0;
}

void
ready(Proc *p)
{
	Schedq *rq;
	int s;

	s = splhi();

	if(p->state == Running)
		rq = &runloq;
	else
		rq = &runhiq;

	lock(&runhiq);
	p->rnext = 0;
	if(rq->tail)
		rq->tail->rnext = p;
	else
		rq->head = p;
	rq->tail = p;

	nrdy++;
	p->state = Ready;
	unlock(&runhiq);
	splx(s);
}

/*
 * Always called splhi
 */
Proc*
runproc(void)
{
	Schedq *rq;
	Proc *p;

loop:
	spllo();
	while(runhiq.head==0 && runloq.head==0)
		;
	splhi();

	lock(&runhiq);
	if(runhiq.head)
		rq = &runhiq;
	else
		rq = &runloq;

	p = rq->head;
	if(p==0 || p->mach){	/* p->mach==0 only when process state is saved */
		unlock(&runhiq);
		goto loop;
	}
	if(p->rnext == 0)
		rq->tail = 0;
	rq->head = p->rnext;
	nrdy--;
	if(p->state != Ready)
		print("runproc %s %d %s\n", p->text, p->pid, statename[p->state]);
	unlock(&runhiq);
	p->state = Scheding;
	return p;
}

int
canpage(Proc *p)
{
	int ok = 0;

	splhi();
	lock(&runhiq);
	/* Only reliable way to see if we are Running */
	if(p->mach == 0) {
		p->newtlb = 1;
		ok = 1;
	}
	unlock(&runhiq);
	spllo();

	return ok;
}

Proc*
newproc(void)
{
	Proc *p;

	for(;;) {
		lock(&procalloc);
		if(p = procalloc.free){		/* assign = */
			procalloc.free = p->qnext;
			p->state = Scheding;
			p->psstate = "New";
			unlock(&procalloc);
			p->mach = 0;
			p->qnext = 0;
			p->nchild = 0;
			p->nwait = 0;
			p->waitq = 0;
			p->pgrp = 0;
			p->egrp = 0;
			p->fgrp = 0;
			p->pdbg = 0;
			p->fpstate = FPinit;
			p->kp = 0;
			p->procctl = 0;
			p->notepending = 0;
			memset(p->seg, 0, sizeof p->seg);
			p->pid = incref(&pidalloc);
			p->noteid = incref(&noteidalloc);
			if(p->pid==0 || p->noteid==0)
				panic("pidalloc");
			return p;
		}
		unlock(&procalloc);
		resrcwait("no procs");
	}
	return 0;		/* not reached */
}

void
procinit0(void)		/* bad planning - clashes with devproc.c */
{
	Proc *p;
	int i;

	procalloc.free = xalloc(conf.nproc*sizeof(Proc));
	procalloc.arena = procalloc.free;

	p = procalloc.free;
	for(i=0; i<conf.nproc-1; i++,p++)
		p->qnext = p+1;
	p->qnext = 0;
}

void
sleep1(Rendez *r, int (*f)(void*), void *arg)
{
	Proc *p;
	int s;

	/*
	 * spl is to allow lock to be called
	 * at interrupt time. lock is mutual exclusion
	 */
	s = splhi();
	p = u->p;
	p->r = r;	/* early so postnote knows */
	lock(r);

	/*
	 * if condition happened, never mind
	 */
	if((*f)(arg)){
		p->r = 0;
		unlock(r);
		splx(s);
		return;
	}

	/*
	 * now we are committed to
	 * change state and call scheduler
	 */
	if(r->p){
		print("double sleep %d %d\n", r->p->pid, p->pid);
		dumpstack();
	}
	p->state = Wakeme;
	r->p = p;
	unlock(r);
}

void
sleep(Rendez *r, int (*f)(void*), void *arg)
{
	Proc *p;
	int s;

	p = u->p;
	sleep1(r, f, arg);
	if(p->notepending == 0)
		sched();	/* notepending may go true while asleep */
	if(p->notepending){
		p->notepending = 0;
		s = splhi();
		lock(r);
		if(r->p == p)
			r->p = 0;
		unlock(r);
		splx(s);
		error(Eintr);
	}
}

int
tfn(void *arg)
{
	Proc *p;

	p = u->p;
	return MACHP(0)->ticks >= p->twhen || (*p->tfn)(arg);
}

void
tsleep(Rendez *r, int (*fn)(void*), void *arg, int ms)
{
	ulong when;
	Proc *p, *f, **l;

	p = u->p;
	when = MS2TK(ms)+MACHP(0)->ticks;

	lock(&talarm);
	/* take out of list if checkalarm didn't */
	if(p->trend) {
		l = &talarm.list;
		for(f = *l; f; f = f->tlink) {
			if(f == p) {
				*l = p->tlink;
				break;
			}
			l = &f->tlink;
		}
	}
	/* insert in increasing time order */
	l = &talarm.list;
	for(f = *l; f; f = f->tlink) {
		if(f->twhen >= when)
			break;
		l = &f->tlink;
	}
	p->trend = r;
	p->twhen = when;
	p->tfn = fn;
	p->tlink = *l;
	*l = p;
	unlock(&talarm);

	sleep(r, tfn, arg);
	p->twhen = 0;
}

/*
 * Expects that only one process can call wakeup for any given Rendez
 */
void
wakeup(Rendez *r)
{
	Proc *p;
	int s;

	s = splhi();
	lock(r);
	p = r->p;
	if(p){
		r->p = 0;
		if(p->state != Wakeme) 
			panic("wakeup: state");
		p->r = 0;
		ready(p);
	}
	unlock(r);
	splx(s);
}

int
postnote(Proc *p, int dolock, char *n, int flag)
{
	User *up;
	KMap *k;
	int s, ret;
	Rendez *r;
	Proc *d, **l;

	if(dolock)
		qlock(&p->debug);

	if(p->upage == 0){
		if(dolock)
			qunlock(&p->debug);
		return 0;
	}

	SET(k);
	if(u == 0 || p != u->p){
		k = kmap(p->upage);
		up = (User*)VA(k);
	}else
		up = u;
	USED(k);

	if(flag!=NUser && (up->notify==0 || up->notified))
		up->nnote = 0;	/* force user's hand */

	ret = 0;
	if(up->nnote < NNOTE){
		strcpy(up->note[up->nnote].msg, n);
		up->note[up->nnote++].flag = flag;
		ret = 1;
	}
	p->notepending = 1;
	if(up != u)
		kunmap(k);
	if(dolock)
		qunlock(&p->debug);

	if(r = p->r){		/* assign = */
		/* wake up; can't call wakeup itself because we're racing with it */
		for(;;) {
			s = splhi();
			if(canlock(r))
				break;
			splx(s);
		}
		if(p->r==r && r->p==p && p->state==Wakeme){	/* check we won the race */
			r->p = 0;
			p->r = 0;
			ready(p);
		}
		unlock(r);
		splx(s);
	}

	if(p->state != Rendezvous)
		return ret;

	/* Try and pull out of a rendezvous */
	lock(p->pgrp);
	if(p->state == Rendezvous) {
		p->rendval = ~0;
		l = &REND(p->pgrp, p->rendtag);
		for(d = *l; d; d = d->rendhash) {
			if(d == p) {
				*l = p->rendhash;
				break;
			}
			l = &d->rendhash;
		}
		ready(p);
	}
	unlock(p->pgrp);
	return ret;
}

/*
 * weird thing: keep at most NBROKEN around
 */
#define	NBROKEN 4
struct
{
	QLock;
	int	n;
	Proc	*p[NBROKEN];
}broken;

void
addbroken(Proc *p)
{
	qlock(&broken);
	if(broken.n == NBROKEN) {
		ready(broken.p[0]);
		memmove(&broken.p[0], &broken.p[1], sizeof(Proc*)*(NBROKEN-1));
		--broken.n;
	}
	broken.p[broken.n++] = p;
	qunlock(&broken);

	p->state = Broken;
	p->psstate = 0;
	sched();
}

void
unbreak(Proc *p)
{
	int b;

	qlock(&broken);
	for(b=0; b < broken.n; b++)
		if(broken.p[b] == p) {
			broken.n--;
			memmove(&broken.p[b], &broken.p[b+1],
					sizeof(Proc*)*(NBROKEN-(b+1)));
			ready(p);
			break;
		}
	qunlock(&broken);
}

int
freebroken(void)
{
	int i, n;

	qlock(&broken);
	n = broken.n;
	for(i=0; i<n; i++) {
		ready(broken.p[i]);
		broken.p[i] = 0;
	}
	broken.n = 0;
	qunlock(&broken);
	return n;
}

void
pexit(char *exitstr, int freemem)
{
	int n;
	long utime, stime;
	Proc *p, *c;
	Segment **s, **es;
	Waitq *wq, *f, *next;

	c = u->p;
	c->alarm = 0;

	if(c->fgrp)
		closefgrp(c->fgrp);
	closepgrp(c->pgrp);
	close(u->dot);
	if(c->egrp)
		closeegrp(c->egrp);

	/*
	 * if not a kernel process and have a parent,
	 * do some housekeeping.
	 */
	if(c->kp == 0) {
		p = c->parent;
		if(p == 0) {
			if(exitstr == 0)
				exitstr = "unknown";
			panic("boot process died: %s", exitstr);
		}

		while(waserror())
			;	
		wq = smalloc(sizeof(Waitq));
		poperror();

		readnum(0, wq->w.pid, NUMSIZE, c->pid, NUMSIZE);
		utime = c->time[TUser] + c->time[TCUser];
		stime = c->time[TSys] + c->time[TCSys];
		readnum(0, &wq->w.time[TUser*12], NUMSIZE,
			TK2MS(utime), NUMSIZE);
		readnum(0, &wq->w.time[TSys*12], NUMSIZE,
			TK2MS(stime), NUMSIZE);
		readnum(0, &wq->w.time[TReal*12], NUMSIZE,
			TK2MS(MACHP(0)->ticks - c->time[TReal]), NUMSIZE);
		if(exitstr && exitstr[0]){
			n = sprint(wq->w.msg, "%s %d:", c->text, c->pid);
			strncpy(wq->w.msg+n, exitstr, ERRLEN-n);
		}
		else
			wq->w.msg[0] = '\0';

		lock(&p->exl);
		/* My parent still alive, processes are limited to 128 Zombies to
		 * prevent a badly written daemon lots of wait records
		 */
		if(p->pid == c->parentpid && p->state != Broken && p->nwait < 128) {	
			p->nchild--;
			p->time[TCUser] += utime;
			p->time[TCSys] += stime;
	
			wq->next = p->waitq;
			p->waitq = wq;
			p->nwait++;
			unlock(&p->exl);
	
			wakeup(&p->waitr);
		}
		else {
			unlock(&p->exl);
			free(wq);
		}
	}

	if(!freemem)
		addbroken(c);

	es = &c->seg[NSEG];
	for(s = c->seg; s < es; s++)
		if(*s)
			putseg(*s);

	lock(&c->exl);		/* Prevent my children from leaving waits */
	c->pid = 0;
	unlock(&c->exl);

	for(f = c->waitq; f; f = next) {
		next = f->next;
		free(f);
	}

	/*
	 * sched() cannot wait on these locks
	 */
	qlock(&c->debug);
	/* release debuggers */
	if(c->pdbg) {
		wakeup(&c->pdbg->sleep);
		c->pdbg = 0;
	}

	lock(&procalloc);
	lock(&palloc);

	c->state = Moribund;
	sched();
	panic("pexit");
}

int
haswaitq(void *x)
{
	Proc *p;

	p = (Proc *)x;
	return p->waitq != 0;
}

ulong
pwait(Waitmsg *w)
{
	Proc *p;
	ulong cpid;
	Waitq *wq;

	p = u->p;

	lock(&p->exl);
	if(p->nchild == 0 && p->waitq == 0) {
		unlock(&p->exl);
		error(Enochild);
	}
	unlock(&p->exl);

	sleep(&p->waitr, haswaitq, u->p);

	lock(&p->exl);
	wq = p->waitq;
	p->waitq = wq->next;
	p->nwait--;
	unlock(&p->exl);

	if(w)
		memmove(w, &wq->w, sizeof(Waitmsg));
	cpid = atoi(wq->w.pid);
	free(wq);
	return cpid;
}

Proc*
proctab(int i)
{
	return &procalloc.arena[i];
}

void
procdump(void)
{
	int i;
	char *s;
	Proc *p;
	ulong bss;

	for(i=0; i<conf.nproc; i++){
		p = procalloc.arena+i;
		if(p->state != Dead){
			bss = 0;
			if(p->seg[BSEG])
				bss = p->seg[BSEG]->top;

			s = p->psstate;
			if(s == 0)
				s = "kproc";
			print("%3d:%10s %10s pc %8lux %8s (%s) ut %ld st %ld r %lux qpc %lux bss %lux\n",
				p->pid, p->text, p->user, p->pc, 
				s, statename[p->state], p->time[0],
				p->time[1], p->r, p->qlockpc, bss);
		}
	}
}

void
kproc(char *name, void (*func)(void *), void *arg)
{
	Proc *p;
	int n;
	ulong upa;
	User *up;
	KMap *k;
	static Pgrp *kpgrp;
	char *user;
	int lastvar;	/* used to compute stack address */

	/*
	 * Kernel stack
	 */
	p = newproc();
	p->psstate = 0;
	p->procmode = 0644;
	p->kp = 1;
	p->upage = newpage(1, 0, USERADDR|(p->pid&0xFFFF));
	k = kmap(p->upage);
	upa = VA(k);
	up = (User*)upa;

	/*
	 * Save time: only copy u-> data and useful stack
	 */
	memmove(up, u, sizeof(User));
	n = USERADDR+BY2PG - (ulong)&lastvar;
	n = (n+32) & ~(BY2WD-1);	/* be safe & word align */
	memmove((void*)(upa+BY2PG-n), (void*)(USERADDR+BY2PG-n), n);
	up->p = p;

	/*
	 * Refs
	 */
	incref(up->dot);
	kunmap(k);

	/*
	 * Sched
	 */
	if(setlabel(&p->sched)){
		p->state = Running;
		p->mach = m;
		m->proc = p;
		spllo();
		(*func)(arg);
		pexit(0, 1);
	}

	user = eve;
	strcpy(p->user, user);
	if(kpgrp == 0){
		kpgrp = newpgrp();
	}
	p->pgrp = kpgrp;
	incref(kpgrp);

	strcpy(p->text, name);

	p->nchild = 0;
	p->parent = 0;
	memset(p->time, 0, sizeof(p->time));
	p->time[TReal] = MACHP(0)->ticks;
	ready(p);
	/*
	 *  since the bss/data segments are now shareable,
	 *  any mmu info about this process is now stale
	 *  and has to be discarded.
	 */
	flushmmu();
}

/*
 *  called splhi() by notify().  See comment in notify for the
 *  reasoning.
 */
void
procctl(Proc *p)
{
	char *state;

	switch(p->procctl) {
	case Proc_exitme:
		spllo();	/* pexit has locks in it */
		pexit("Killed", 1);

	case Proc_traceme:
		if(u->nnote == 0)
			return;
		/* No break */

	case Proc_stopme:
		p->procctl = 0;
		state = p->psstate;
		p->psstate = "Stopped";
		/* free a waiting debugger */
		qlock(&p->debug);
		if(p->pdbg) {
			wakeup(&p->pdbg->sleep);
			p->pdbg = 0;
		}
		qunlock(&p->debug);
		p->state = Stopped;
		sched();		/* sched returns spllo() */
		splhi();
		p->psstate = state;
		return;
	}
}

#include "errstr.h"

void
error(char *err)
{
	strncpy(u->error, err, ERRLEN);
	nexterror();
}

void
nexterror(void)
{
	gotolabel(&u->errlab[--u->nerrlab]);
}

void
exhausted(char *resource)
{
	char buf[ERRLEN];

	sprint(buf, "no free %s", resource);
	error(buf);
}
