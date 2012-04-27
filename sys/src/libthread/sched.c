#include <u.h>
#include <libc.h>
#include <thread.h>
#include "threadimpl.h"
#include <tos.h>

static Thread	*runthread(Proc*);

static char *_psstate[] = {
	"Moribund",
	"Dead",
	"Exec",
	"Fork",
	"Running",
	"Ready",
	"Rendezvous",
};

static char*
psstate(int s)
{
	if(s < 0 || s >= nelem(_psstate))
		return "unknown";
	return _psstate[s];
}

void
_schedinit(void *arg)
{
	Proc *p;
	Thread *t, **l;

	p = arg;
	_threadsetproc(p);
	p->pid = _tos->pid; //getpid();
	while(setjmp(p->sched))
		;
	_threaddebug(DBGSCHED, "top of schedinit, _threadexitsallstatus=%p", _threadexitsallstatus);
	if(_threadexitsallstatus)
		exits(_threadexitsallstatus);
	lock(&p->lock);
	if((t=p->thread) != nil){
		p->thread = nil;
		if(t->moribund){
			t->state = Dead;
			for(l=&p->threads.head; *l; l=&(*l)->nextt)
				if(*l == t){
					*l = t->nextt;
					if(*l==nil)
						p->threads.tail = l;
					p->nthreads--;
					break;
				}
			unlock(&p->lock);
			if(t->inrendez){
				_threadflagrendez(t);
				_threadbreakrendez();
			}
			free(t->stk);
			free(t->cmdname);
			free(t);	/* XXX how do we know there are no references? */
			t = nil;
			_sched();
		}
		if(p->needexec){
			t->ret = _schedexec(&p->exec);
			p->needexec = 0;
		}
		if(p->newproc){
			t->ret = _schedfork(p->newproc);
			p->newproc = nil;
		}
		t->state = t->nextstate;
		if(t->state == Ready)
			_threadready(t);
	}
	unlock(&p->lock);
	_sched();
}

void
needstack(int n)
{
	int x;
	Proc *p;
	Thread *t;
	
	p = _threadgetproc();
	t = p->thread;
	
	if((uchar*)&x - n < (uchar*)t->stk){
		fprint(2, "%s %lud: &x=%p n=%d t->stk=%p\n",
			argv0, _tos->pid, &x, n, t->stk);
		fprint(2, "%s %lud: stack overflow\n", argv0, _tos->pid);
		abort();
	}
}

void
_sched(void)
{
	Proc *p;
	Thread *t;

Resched:
	p = _threadgetproc();
	if((t = p->thread) != nil){
		needstack(128);
		_threaddebug(DBGSCHED, "pausing, state=%s", psstate(t->state));
		if(setjmp(t->sched)==0)
			longjmp(p->sched, 1);
		return;
	}else{
		t = runthread(p);
		if(t == nil){
			_threaddebug(DBGSCHED, "all threads gone; exiting");
			_schedexit(p);
		}
		_threaddebug(DBGSCHED, "running %d.%d", t->proc->pid, t->id);
		p->thread = t;
		if(t->moribund){
			_threaddebug(DBGSCHED, "%d.%d marked to die");
			goto Resched;
		}
		t->state = Running;
		t->nextstate = Ready;
		longjmp(t->sched, 1);
	}
}

static Thread*
runthread(Proc *p)
{
	Thread *t;
	Tqueue *q;

	if(p->nthreads==0)
		return nil;
	q = &p->ready;
	lock(&p->readylock);
	if(q->head == nil){
		q->asleep = 1;
		_threaddebug(DBGSCHED, "sleeping for more work");
		unlock(&p->readylock);
		while(rendezvous(q, 0) == (void*)~0){
			if(_threadexitsallstatus)
				exits(_threadexitsallstatus);
		}
		/* lock picked up from _threadready */
	}
	t = q->head;
	q->head = t->next;
	unlock(&p->readylock);
	return t;
}

void
_threadready(Thread *t)
{
	Tqueue *q;

	assert(t->state == Ready);
	_threaddebug(DBGSCHED, "readying %d.%d", t->proc->pid, t->id);
	q = &t->proc->ready;
	lock(&t->proc->readylock);
	t->next = nil;
	if(q->head==nil)
		q->head = t;
	else
		*q->tail = t;
	q->tail = &t->next;
	if(q->asleep){
		q->asleep = 0;
		/* lock passes to runthread */
		_threaddebug(DBGSCHED, "waking process %d", t->proc->pid);
		while(rendezvous(q, 0) == (void*)~0){
			if(_threadexitsallstatus)
				exits(_threadexitsallstatus);
		}
	}else
		unlock(&t->proc->readylock);
}

void
yield(void)
{
	_sched();
}

