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
#include <thread.h>
#include "threadimpl.h"
#include <tos.h>

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
	int pid[64];
	int i, npid, mypid;

	if(exitstr == nil)
		exitstr = "";
	_threadexitsallstatus = exitstr;
	_threaddebug(DBGSCHED, "_threadexitsallstatus set to %p", _threadexitsallstatus);
	mypid = _tos->pid; //getpid();

	/*
	 * signal others.
	 * copying all the pids first avoids other threads
	 * teardown procedures getting in the way.
	 *
	 * avoid mallocs since malloc can post a note which can
	 * call threadexitsall...
	 */
	for(;;){
		lock(&_threadpq.lock);
		npid = 0;
		for(p = _threadpq.head; p && npid < nelem(pid); p=p->next){
			if(p->threadint == 0 && p->pid != mypid){
				pid[npid++] = p->pid;
				p->threadint = 1;
			}
		}
		unlock(&_threadpq.lock);
		if(npid == 0)
			break;
		for(i=0; i<npid; i++)
			postnote(PNPROC, pid[i], "threadint");
	}

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
