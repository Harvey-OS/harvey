#include <u.h>
#include <libc.h>
#include <thread.h>
#include "threadimpl.h"

Pqueue _threadpq;

static int
nextID(void)
{
	static Lock l;
	static int id;
	int i;

	lock(&l);
	i = ++id;
	unlock(&l);
	return i;
}
	
/*
 * Create and initialize a new Thread structure attached to a given proc.
 */
static int
newthread(Proc *p, void (*f)(void *arg), void *arg, uint stacksize, char *name, int grp)
{
	int id;
	Thread *t;

	if(stacksize < 32)
		sysfatal("bad stacksize %d", stacksize);
	t = _threadmalloc(sizeof(Thread), 1);
	t->stksize = stacksize;
	t->stk = _threadmalloc(stacksize, 0);
	memset(t->stk, 0xFE, stacksize);
	_threadinitstack(t, f, arg);
	t->grp = grp;
	if(name)
		t->cmdname = strdup(name);
	t->id = nextID();
	id = t->id;
	t->next = (Thread*)~0;
	t->proc = p;
	_threaddebug(DBGSCHED, "create thread %d.%d name %s", p->pid, t->id, name);
	lock(&p->lock);
	p->nthreads++;
	if(p->threads.head == nil)
		p->threads.head = t;
	else
		*p->threads.tail = t;
	p->threads.tail = &t->nextt;
	t->nextt = nil;
	t->state = Ready;
	_threadready(t);
	unlock(&p->lock);
	return id;
}

/* 
 * Create a new thread and schedule it to run.
 * The thread grp is inherited from the currently running thread.
 */
int
threadcreate(void (*f)(void *arg), void *arg, uint stacksize)
{
	return newthread(_threadgetproc(), f, arg, stacksize, nil, threadgetgrp());
}

/*
 * Create and initialize a new Proc structure with a single Thread
 * running inside it.  Add the Proc to the global process list.
 */
Proc*
_newproc(void (*f)(void *arg), void *arg, uint stacksize, char *name, int grp, int rforkflag)
{
	Proc *p;

	p = _threadmalloc(sizeof *p, 1);
	p->pid = -1;
	p->rforkflag = rforkflag;
	newthread(p, f, arg, stacksize, name, grp);

	lock(&_threadpq.lock);
	if(_threadpq.head == nil)
		_threadpq.head = p;
	else
		*_threadpq.tail = p;
	_threadpq.tail = &p->next;
	unlock(&_threadpq.lock);
	return p;
}

int
procrfork(void (*f)(void *), void *arg, uint stacksize, int rforkflag)
{
	Proc *p;
	int id;

	p = _threadgetproc();
	assert(p->newproc == nil);
	p->newproc = _newproc(f, arg, stacksize, nil, p->thread->grp, rforkflag);
	id = p->newproc->threads.head->id;
	_sched();
	return id;
}

int
proccreate(void (*f)(void*), void *arg, uint stacksize)
{
	return procrfork(f, arg, stacksize, 0);
}

void
_freeproc(Proc *p)
{
	Thread *t, *nextt;

	for(t = p->threads.head; t; t = nextt){
		if(t->cmdname)
			free(t->cmdname);
		assert(t->stk != nil);
		free(t->stk);
		nextt = t->nextt;
		free(t);
	}
	free(p);
}

void
_freethread(Thread *t)
{
	Proc *p;
	Thread **l;

	p = t->proc;
	lock(&p->lock);
	for(l=&p->threads.head; *l; l=&(*l)->nextt){
		if(*l == t){
			*l = t->nextt;
			if(*l == nil)
				p->threads.tail = l;
			break;
		}
	}
	unlock(&p->lock);
	if (t->cmdname)
		free(t->cmdname);
	assert(t->stk != nil);
	free(t->stk);
	free(t);
}

