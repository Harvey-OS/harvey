#include <u.h>
#include <libc.h>
#include <thread.h>
#include "threadimpl.h"

char *_threadexitsallstatus;
Channel *_threadwaitchan;

void
threadexits(char *exitstr)
{
	Proc *p;
	Thread *t;

	p = _threadgetproc();
	t = p->thread;
	t->moribund = 1;
	if(exitstr==nil)
		exitstr="";
	utfecpy(p->exitstr, p->exitstr+ERRMAX, exitstr);
	_sched();
}

void
threadexitsall(char *exitstr)
{
	Proc *p;
	int *pid;
	int i, npid, mypid;

	if(exitstr == nil)
		exitstr = "";
	_threadexitsallstatus = exitstr;
	_threaddebug(DBGSCHED, "_threadexitsallstatus set to %p", _threadexitsallstatus);
	mypid = getpid();

	/*
	 * signal others.
	 * copying all the pids first avoids other threads
	 * teardown procedures getting in the way.
	 */
	lock(&_threadpq.lock);
	npid = 0;
	for(p=_threadpq.head; p; p=p->next)
		npid++;
	pid = _threadmalloc(npid*sizeof(pid[0]), 0);
	npid = 0;
	for(p = _threadpq.head; p; p=p->next)
		pid[npid++] = p->pid;
	unlock(&_threadpq.lock);
	for(i=0; i<npid; i++)
		if(pid[i] != mypid)
			postnote(PNPROC, pid[i], "threadint");

	/* leave */
	exits(exitstr);
}

Channel*
threadwaitchan(void)
{
	if(_threadwaitchan==nil)
		_threadwaitchan = chancreate(sizeof(Waitmsg*), 16);
	return _threadwaitchan;
}
