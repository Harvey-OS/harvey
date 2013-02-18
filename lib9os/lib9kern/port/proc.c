#include	"dat.h"
#include	"fns.h"
#include	"error.h"
#include	"interp.h"

Proc*
newproc(void)
{
	Proc *p;

	p = malloc(sizeof(Proc));
	if(p == nil)
		return nil;

	p->type = Unknown;
	p->killed = 0;
	p->swipend = 0;
	p->env = &p->defenv;
	kstrdup(&p->env->user, "*nouser");
	p->env->errstr = p->env->errbuf0;
	p->env->syserrstr = p->env->errbuf1;
	// XXX: what is the difference between a proc and prog?
	addprog(p);

	return p;
}

void
Sleep(Rendez *r, int (*f)(void*), void *arg)
{
	lock(&up->rlock);
	lock(&r->l);

	/*
	 * if interrupted or condition happened, never mind
	 */
	if(up->killed || f(arg)) {
		unlock(&r->l);
	}else{
		if(r->p != nil)
			panic("double sleep pc=0x%lux %s[%lud] %s[%lud] r=0x%lux\n", getcallerpc(&r), r->p->text, r->p->pid, up->text, up->pid, r);

		r->p = up;
		unlock(&r->l);
		up->swipend = 0;
		up->r = r;	/* for swiproc */
		unlock(&up->rlock);

		osblock();

		lock(&up->rlock);
		up->r = nil;
	}

	if(up->killed || up->swipend) {
		up->killed = 0;
		up->swipend = 0;
		unlock(&up->rlock);
		p9error(Eintr);
	}
	unlock(&up->rlock);
}

int
Wakeup(Rendez *r)
{
	Proc *p;

	lock(&r->l);
	p = r->p;
	if(p != nil) {
		r->p = nil;
		osready(p);
	}
	unlock(&r->l);
	return p != nil;
}

void
swiproc(Proc *p, int interp)
{
	Rendez *r;
	
	if(p == nil)
		return;

	/*
	 * Pull out of emu Sleep
	 */
	lock(&p->rlock);
	if(!interp)
		p->killed = 1;
	r = p->r;
	if(r != nil) {
		lock(&r->l);
		if(r->p == p){
			p->swipend = 1;
			r->p = nil;
			osready(p);
		}
		unlock(&r->l);
		unlock(&p->rlock);
		return;
	}
	unlock(&p->rlock);

	/*
	 * Maybe pull out of Host OS
	 */
	lock(&p->sysio);
	if(p->syscall && p->intwait == 0) {
		p->swipend = 1;
		p->intwait = 1;
		unlock(&p->sysio);
		oshostintr(p);
		return;	
	}
	unlock(&p->sysio);
}

void
notkilled(void)
{
	lock(&up->rlock);
	up->killed = 0;
	unlock(&up->rlock);
}

void
osenter(void)
{
	up->syscall = 1;
}

void
osleave(void)
{
	int r;

	lock(&up->rlock);
	r = up->swipend;
	up->swipend = 0;
	unlock(&up->rlock);

	lock(&up->sysio);
	up->syscall = 0;
	unlock(&up->sysio);

	/* Cleared by the signal/note/exception handler */
	while(up->intwait)
		osyield();

	if(r != 0)
		p9error(Eintr);
}

void
rptwakeup(void *o, void *ar)
{
	Rept *r;

	r = ar;
	if(r == nil)
		return;
	lock(&r->l);
	r->o = o;
	unlock(&r->l);
	Wakeup(&r->r);
}

static int
rptactive(void *a)
{
	Rept *r = a;
	int i;
	lock(&r->l);
	i = r->active(r->o);
	unlock(&r->l);
	return i;
}

static void
rproc(void *a)
{
	long now, then;
	ulong t;
	int i;
	void *o;
	Rept *r;

	up->env->fgrp = newfgrp(nil);
	r = a;
	t = (ulong)r->t;

Wait:
	Sleep(&r->r, rptactive, r);
	lock(&r->l);
	o = r->o;
	unlock(&r->l);
	then = osmillisec();
	for(;;){
		osmillisleep(t);
		now = osmillisec();
		if(waserror())
			break;
		i = r->ck(o, now-then);
		poperror();
		if(i == -1)
			goto Wait;
		if(i == 0)
			continue;
		then = now;
		acquire();
		if(waserror()) {
			release();
			break;
		}
		r->f(o);
		poperror();
		release();
	}
	pexit("", 0);
}

void*
rptproc(char *s, int t, void *o, int (*active)(void*), int (*ck)(void*, int), void (*f)(void*))
{
	Rept *r;

	r = mallocz(sizeof(Rept), 1);
	if(r == nil)
		return nil;
	r->t = t;
	r->active = active;
	r->ck = ck;
	r->f = f;
	r->o = o;
	kproc(s, rproc, r, KPDUPPG);
	return r;
}
