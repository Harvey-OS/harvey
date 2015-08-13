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

typedef struct Mainarg Mainarg;
struct Mainarg
{
	int	argc;
	char	**argv;
};

int	mainstacksize;
int	_threadnotefd;
int	_threadpasserpid;
static jmp_buf _mainjmp;
static void mainlauncher(void*);
extern void (*_sysfatal)(char*, ...);
extern void (*__assert)(char*);
extern int (*_dial)(char*, char*, char*, int*);

extern int _threaddial(char*, char*, char*, int*);

static Proc **mainp;

void
main(int argc, char **argv)
{
	Mainarg *a;
	Proc *p;

	rfork(RFREND);
	mainp = &p;
	if(setjmp(_mainjmp))
		_schedinit(p);

//_threaddebuglevel = (DBGSCHED|DBGCHAN|DBGREND)^~0;
	_systhreadinit();
	_qlockinit(_threadrendezvous);
	_sysfatal = _threadsysfatal;
	_dial = _threaddial;
	__assert = _threadassert;
	notify(_threadnote);
	if(mainstacksize == 0)
		mainstacksize = 8*1024;

	a = _threadmalloc(sizeof *a, 1);
	a->argc = argc;
	a->argv = argv;

	p = _newproc(mainlauncher, a, mainstacksize, "threadmain", 0, 0);
	_schedinit(p);
	abort();	/* not reached */
}

static void
mainlauncher(void *arg)
{
	Mainarg *a;

	a = arg;
	threadmain(a->argc, a->argv);
	threadexits("threadmain");
}

static void
efork(Execargs *e)
{
	char buf[ERRMAX];

	_threaddebug(DBGEXEC, "_schedexec %s", e->prog);
	close(e->fd[0]);
	exec(e->prog, e->args);
	_threaddebug(DBGEXEC, "_schedexec failed: %r");
	rerrstr(buf, sizeof buf);
	if(buf[0]=='\0')
		strcpy(buf, "exec failed");
	write(e->fd[1], buf, strlen(buf));
	close(e->fd[1]);
	_exits(buf);
}

int
_schedexec(Execargs *e)
{
	int pid;

	switch(pid = rfork(RFREND|RFNOTEG|RFFDG|RFMEM|RFPROC)){
	case 0:
		efork(e);
	default:
		return pid;
	}
}

int
_schedfork(Proc *p)
{
	int pid;

	switch(pid = rfork(RFPROC|RFMEM|RFNOWAIT|p->rforkflag)){
	case 0:
		*mainp = p;	/* write to stack, so local to proc */
		longjmp(_mainjmp, 1);
	default:
		return pid;
	}
}

void
_schedexit(Proc *p)
{
	char ex[ERRMAX];
	Proc **l;

	lock(&_threadpq.lock);
	for(l=&_threadpq.head; *l; l=&(*l)->next){
		if(*l == p){
			*l = p->next;
			if(*l == nil)
				_threadpq.tail = l;
			break;
		}
	}
	unlock(&_threadpq.lock);

	utfecpy(ex, ex+sizeof ex, p->exitstr);
	free(p);
	_exits(ex);
}

void
_schedexecwait(void)
{
	int pid;
	Channel *c;
	Proc *p;
	Thread *t;
	Waitmsg *w;

	p = _threadgetproc();
	t = p->thread;
	pid = t->ret;
	_threaddebug(DBGEXEC, "_schedexecwait %d", t->ret);

	rfork(RFCFDG);
	for(;;){
		w = wait();
		if(w == nil)
			break;
		if(w->pid == pid)
			break;
		free(w);
	}
	if(w != nil){
		if((c = _threadwaitchan) != nil)
			sendp(c, w);
		else
			free(w);
	}
	threadexits("procexec");
}

static Proc **procp;

void
_systhreadinit(void)
{
	procp = (Proc **)privalloc();
}

Proc*
_threadgetproc(void)
{
	return *procp;
}

void
_threadsetproc(Proc *p)
{
	*procp = p;
}
