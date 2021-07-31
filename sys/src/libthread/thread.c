#include <u.h>
#include <libc.h>
#include "assert.h"
#include "threadimpl.h"

static void		initproc(void (*)(ulong, int, char*[]), int, char*[], uint);
static void		garbagethread(Thread *t);
static Thread*	getqbytag(Tqueue *, ulong);
static void		putq(Tqueue *, Thread *);
static Thread*	getq(Tqueue *);
static void		launcher(ulong, void (*)(void *), void *);
static void		mainlauncher(ulong, int argc, char *argv[]);
static void		garbageproc(Proc *);
static Proc*	prepproc(Newproc *);
static int		setnoteid(int);
static int		getnoteid(void);

static int		notefd;
static int		passerpid;
static long	totalmalloc;

static Thread*	dontkill;

static Tqueue	rendez;
static Lock	rendezlock;
int			mainstacksize;
Channel*		thewaitchan;

struct Pqueue	pq;
Proc			**procp;	/* Pointer to pointer to proc's Proc structure */

#define STACKOFF 2
#ifdef M386
	#define FIX1
	#define FIX2 *--tos = 0
#endif
#ifdef Mmips
	#define FIX1
	#define FIX2
#endif
#ifdef Mpower
	/* This could be wrong ... */
	#define FIX1
	#define FIX2 *--tos = 0
#endif
#ifdef Malpha
	#define FIX1 *--tos = 0
	#define FIX2 *--tos = 0
#endif
#ifdef Marm
	#define FIX1
	#define FIX2
#endif

static int
nextID(void)
{
	static Lock l;
	static id;
	int i;

	lock(&l);
	i = ++id;
	unlock(&l);
	return i;
}
	
static int
notehandler(void *, char *s) {
	Proc *p;
	Thread *t;
	int id;

	if (DBGNOTE & _threaddebuglevel)
		threadprint(2, "Got note %s\n", s);
	if (getpid() == passerpid) {
		if (DBGNOTE & _threaddebuglevel)
			threadprint(2, "Notepasser %d got note %s\n", passerpid, s);
		if (strcmp(s, "kilpasser") == 0)
			exits(nil);
		write(notefd, s, strlen(s));
		if (strncmp(s, "kilthr", 6) == 0 || strncmp(s, "kilgrp", 6) == 0)
			return 1;
	} else {
		if (strncmp(s, "kilthr", 6) == 0) {
			p = *procp;
			id = strtoul(s+6, nil, 10);
			if (DBGNOTE & _threaddebuglevel)
				threadprint(2, "Thread id %d\n", id);
			for (t = p->threads.head; t; t = t->nextt) {
				if (t->id == id) {
					break;
				}
			}
			if (t == nil) {
				if (DBGNOTE & _threaddebuglevel)
					threadprint(2, "Thread id %d not found\n", id);
				return 1;
			}
			if (t == p->curthread) {
				char err[ERRLEN] = "exiting";
				if (DBGNOTE & _threaddebuglevel)
					threadprint(2, "Killing current thread\n");
				errstr(err);
			}
			threadassert(t->state != Rendezvous);
			t->exiting = 1;
			return 1;
		}
		if (strncmp(s, "kilgrp", 6) == 0) {
			p = *procp;
			id = strtoul(s+6, nil, 10);
			if (DBGNOTE & _threaddebuglevel)
				threadprint(2, "Thread grp %d", id);
			for (t = p->threads.head; t; t = t->nextt) {
				if (t->grp == id) {
					if (t != p->curthread)
						t->exiting = 1;
				}
			}
			return 1;
		}
	}
	if (strncmp(s, "kilall", 6) == 0)
		exits(s+6);
	return 0;
}

void
threadexits(char *exitstr) {
	Thread *t, *new;
	Proc *p;
	Channel *c;

	_threaddebug(DBGTHRD|DBGKILL, "Exitthread()");
	p = *procp;
	t = p->curthread;
	threadassert(t->state == Running);
	if (p->nthreads > 1) {
		t->exiting = 1;
		p->nthreads--;
		lock(&rendezlock);
		while ((new = getq(&p->runnable)) == nil) {
			_threaddebug(DBGTHRD, "Nothing left to run");
			/* called with rendezlock held */
			p->blocked = 1;
			unlock(&rendezlock);
			if ((new = (Thread *)rendezvous((ulong)p, 0)) != (Thread *)~0) {
				threadassert(!p->blocked);
				threadassert(new->proc == p);
				p->curthread = new;
				new->state = Running;
				/* switch to new thread, pass it `t' for cleanup */
				new->garbage = t;
				longjmp(new->env, (int)t);
				/* No return */
			}
			_threaddebug(DBGNOTE|DBGTHRD, "interrupted");
			lock(&rendezlock);
		}
		unlock(&rendezlock);
		_threaddebug(DBGTHRD, "Yield atexit to %d.%d", p->pid, new->id);
		p->curthread = new;
		new->state = Running;
		if (new->exiting)
			threadexits(nil);
		/* switch to new thread, pass it `t' for cleanup */
		new->garbage = t;
		longjmp(new->env, (int)t);
		/* no return */
	}
	/*
	 * thewaitchan confounds exiting the entire program, so handle it
	 * carefully; store exposed global in local variable c:
	 */
	if ((c = thewaitchan) != nil) {
		Waitmsg w;
		long t[4];

		snprint(w.pid, sizeof(w.pid), "%d", p->pid);
		times(t);
		snprint(w.time + 0*12, 12, "%10.2fu", t[0]/1000.0);
		snprint(w.time + 1*12, 12, "%10.2fu", t[1]/1000.0);
		snprint(w.time + 2*12, 12, "%10.2fu",
			(t[0]+t[1]+t[2]+t[3])/1000.0);	/* Cheating just a bit */
		if (exitstr)
			strncpy(w.msg, exitstr, sizeof(w.msg));
		else
			w.msg[0] = '\0';
		_threaddebug(DBGCHLD, "In thread %s: sending exit status %s for %d\n",
			p->curthread->cmdname, exitstr, p->pid);
		send(c, &w);
	}
	_threaddebug(DBGPROC, "Exiting\n");
	t->exiting = 1;
	if(exitstr == nil)
		p->str[0] = '\0';
	else
		strncpy(p->str, exitstr, sizeof(p->str));
	p->nthreads--;
	/* Clean up and exit */
	longjmp(p->oldenv, DOEXIT);
}

static Thread *
threadof(int id) {
	Proc *pp;
	Thread *t;

	lock(&pq.lock);
	for (pp = pq.head; pp->next; pp = pp->next)
		for (t = pp->threads.head; t; t = t->nextt)
			if (t->id == id) {
				unlock(&pq.lock);
				return t;
			}
	unlock(&pq.lock);
	return nil;
}

int
threadpid(int id) {
	Proc *pp;
	Thread *t;

	if (id < 0)
		return id;
	if (id == 0)
		return (*procp)->pid;
	lock(&pq.lock);
	for (pp = pq.head; pp->next; pp = pp->next)
		for (t = pp->threads.head; t; t = t->nextt)
			if (t->id == id) {
				unlock(&pq.lock);
				return pp->pid;
			}
	unlock(&pq.lock);
	return -1;
}

void
threadkillgrp(int grp) {
	int fd;
	char buf[128];

	sprint(buf, "/proc/%d/notepg", getpid());
	fd = open(buf, OWRITE|OCEXEC);
	sprint(buf, "kilgrp%d", grp);
	write(fd, buf, strlen(buf));
	close(fd);
}

void
threadkill(int id) {
	char buf[16];
	Thread *t;
	int pid;
	Proc *p;

	if ((t = threadof(id)) == nil) {
		if (_threaddebuglevel & DBGNOTE)
			threadprint(2, "Can't find thread to kill\n");
		return;
	}
	sprint(buf, "kilthr%d", id);
	if ((pid = t->proc->pid) == 0) {
		if (_threaddebuglevel & DBGNOTE)
			threadprint(2, "Thread has zero pid\n");
		if (postnote(PNGROUP, getpid(), buf) < 0)
			threadprint(2, "postnote failed: %r\n");
		return;
	}
	p = *procp;
	if (pid == p->pid) {
		if (_threaddebuglevel & DBGNOTE)
			threadprint(2, "Thread in same proc\n");
		assert(pid == getpid());
		if (t == p->curthread) {
			threadprint(2, "Threadkill:  killing self\n");
			threadexits("threadicide");
		}
		lock(&rendezlock);
		if (t->state == Rendezvous) {
			if (_threaddebuglevel & DBGNOTE)
				threadprint(2, "Thread was in rendezvous\n");
			assert(getqbytag(&rendez, t->tag) == t);
		}
		t->exiting = 1;
		unlock(&rendezlock);
		garbagethread(t);
		return;
	}
	if (postnote(PNPROC, pid, buf) < 0)
		threadprint(2, "postnote failed: %r\n");
}

void
threadexitsall(char *status) {
	int fd;
	Channel *c;
	char buf[128];

	if ((c = thewaitchan) != nil) {
		thewaitchan = nil;
		while (nbrecv(c, nil))
			;	/* Unblock threads previously exited,
				 * sending on thewaitchan, not yet waited for
				 */
	}
	sprint(buf, "/proc/%d/note", passerpid);
	if ((fd = open(buf, OWRITE)) < 0) {
		/* passer proc died, send note to notepg directly */
		sprint(buf, "/proc/%d/notepg", getpid());
		if ((fd = open(buf, OWRITE)) < 0) {
			threadprint(2, "threadexitsall: can't open %s: %r\n", buf);
			exits("panic");
		}
		snprint(buf, sizeof buf, "kilall%s", status?status:"");
		if (DBGNOTE & _threaddebuglevel)
			fprint(2, "Sending note %s to process group of %d\n", buf, getpid());
	} else {
		snprint(buf, sizeof buf, "kilall%s", status?status:"");
		if (DBGNOTE & _threaddebuglevel)
			fprint(2, "Sending note %s to passer proc %d\n", buf, passerpid);
	}
	write(fd, buf, strlen(buf));
	exits(status);
}

void
yield(void) {
	Thread *new, *t;
	Proc *p;
	ulong thr;

	p = *procp;
	t = p->curthread;

	if (t->exiting) {
		_threaddebug(DBGTHRD|DBGKILL, "Exiting in yield()");
		threadexits(nil);	/* no return */
	}
	if ((new = getq(&p->runnable)) == nil) {
		_threaddebug(DBGTHRD, "Nothing to yield for");
		return;	/* Nothing to yield for */
	}
	if ((thr = setjmp(p->curthread->env))) {
		if (thr != ~0)
			garbagethread((Thread *)thr);
		if ((*procp)->curthread->exiting)
			threadexits(nil);
		return;	/* Return from yielding */
	}
	putq(&p->runnable, p->curthread);
	p->curthread->state = Runnable;
	_threaddebug(DBGTHRD, "Yielding to %d.%d", p->pid, new->id);
	p->curthread = new;
	new->state = Running;
	longjmp(new->env, ~0);
	/* no return */
}

ulong
_threadrendezvous(ulong tag, ulong value) {
	Proc *p;
	Thread *this, *that, *new;
	ulong v, t;

	p = *procp;
	this = p->curthread;

	lock(&rendezlock);
	_threaddebug(DBGREND, "rendezvous tag %lux", tag);
	/* find a thread waiting in a rendezvous on tag */
	that = getqbytag(&rendez, tag);
	/* if a thread in same or another proc waiting, rendezvous */
	if (that) {
		_threaddebug(DBGREND, "waiting proc is %lud.%d", that->proc->pid, that->id);
		threadassert(that->state == Rendezvous);
		/* exchange values */
		v = that->value;
		that->value = value;
		/* remove from rendez-vous queue */
		that->state = Runnable;
		if (that->proc->blocked) {
			threadassert(that->proc != p);
			that->proc->blocked = 0;
			_threaddebug(DBGREND, "unblocking rendezvous, tag = %lux", (ulong)that->proc);
			unlock(&rendezlock);
			/* `Non-blocking' rendez-vous */
			while (rendezvous((ulong)that->proc, (ulong)that) == ~0) {
				_threaddebug(DBGNOTE|DBGTHRD, "interrupted");
				if (this->exiting) {
					_threaddebug(DBGNOTE|DBGTHRD, "and committing suicide");
					threadexits(nil);
				}
			}
		} else {
			putq(&that->proc->runnable, that);
			unlock(&rendezlock);
		}
		yield();
		return v;
	}
	_threaddebug(DBGREND, "blocking");
	/* Mark this thread waiting */
	this->value = value;
	this->state = Rendezvous;
	this->tag = tag;
	putq(&rendez, this);

	/* Look for runnable threads */
	new = getq(&p->runnable);
	if (new == nil) {
		/* No other thread runnable, rendezvous */
		p->blocked = 1;
		_threaddebug(DBGREND, "blocking rendezvous, tag = %lux", (ulong)p);
		unlock(&rendezlock);
		while ((new = (Thread *)rendezvous((ulong)p, 0)) == (Thread *)~0) {
			_threaddebug(DBGNOTE|DBGTHRD, "interrupted");
			if (this->exiting) {
				_threaddebug(DBGNOTE|DBGTHRD, "and committing suicide");
				threadexits(nil);
			}
		}
		threadassert(!p->blocked);
		threadassert(new->proc == p);
		if (new == this) {
			this->state = Running;
			if (this->exiting)
				threadexits(nil);
			return this->value;
		}
		_threaddebug(DBGREND, "after rendezvous, new thread is %lux, id %d", new, new->id);
	} else {
		unlock(&rendezlock);
	}
	if ((t = setjmp(p->curthread->env))) {
		if (t != ~0)
			garbagethread((Thread *)t);
		_threaddebug(DBGREND, "unblocking");
		threadassert(p->curthread->next == (Thread *)~0);
		this->state = Running;
		if (p->curthread->exiting) {
			_threaddebug(DBGKILL, "Exiting after rendezvous");
			threadexits(nil);
		}
		return this->value;
	}
	_threaddebug(DBGREND|DBGTHRD, "Scheduling %lud.%d", new->proc->pid, new->id);
	p->curthread = new;
	new->state = Running;
	longjmp(new->env, ~0);
	/* no return */
	return ~0;	/* Not called */
}

void
main(int argc, char *argv[])
{
	char buf[40], err[ERRLEN];
	int fd, id;

	_qlockinit(_threadrendezvous);
	id = getnoteid();
	threadassert(id > 0);
	rfork(RFNOTEG|RFREND);
	atnotify(notehandler, 1);
	switch(rfork(RFPROC|RFMEM)){
	case -1:
		threadassert(0);
	case 0:
		/* handle notes */
		rfork(RFCFDG);
		if (_threaddebuglevel) {
			fd = open("/dev/cons", OWRITE);
			dup(fd, 2);
		}
		passerpid = getpid();
		/*
		 * The noteid is attached to the fd at open.
		 * Notefd will correspond to our current (new) noteid
		 * even after we set our noteid back to the old one.
		 *
		 * We could open main's notepg file and not depend
		 * on this behavior, but then we would depend on the fact that
		 * when main exits its open notepg file is still writable.
		 * We would also depend on opening the file before main
		 * exits, which is a race I'd rather not run.
		 */
		sprint(buf, "/proc/%d/notepg", getpid());
		notefd = open(buf, OWRITE|OCEXEC);
		if (_threaddebuglevel & DBGNOTE)
			threadprint(2, "Passer is %d\n", passerpid);
		if(setnoteid(id) < 0){
			passerpid = -1;
			exits(0);
		}
		for(;;){
			if(sleep(120*1000) < 0){
				errstr(err);
				if (strcmp(err, "interrupted") != 0)
					break;
			}
		}
		if (_threaddebuglevel & DBGNOTE)
			threadprint(2, "Passer %d exits\n", passerpid);
		exits("interrupted");

	default:
		if(mainstacksize == 0)
			mainstacksize = 512*1024;
		initproc(mainlauncher, argc, argv, mainstacksize);
		/* never returns */
	}
}

void
threadsetname(char *name)
{
	Thread *t = (*procp)->curthread;

	if (t->cmdname)
		free(t->cmdname);
	t->cmdname = strdup(name);
}

char *threadgetname(void) {
	return (*procp)->curthread->cmdname;
}

ulong*
procdata(void) {
	return &(*procp)->udata;
}

ulong*
threaddata(void) {
	return &(*procp)->curthread->udata;
}

int
procrfork(void (*f)(void *), void *arg, uint stacksize, int rforkflag) {
	Newproc *np;
	Proc *p;
	int id;
	ulong *tos;

	p = *procp;
	/* Save old stack position */
	if ((id = setjmp(p->curthread->env))) {
		_threaddebug(DBGPROC, "newproc, return\n");
		return id;	/* Return with pid of new proc */
	}
	np = _threadmalloc(sizeof(Newproc));
	threadassert(np != nil);

	_threaddebug(DBGPROC, "newproc, creating stack\n");
	/* Create a stack */
	np->stack = _threadmalloc(stacksize);
	threadassert(np->stack != nil);
	memset(np->stack, 0xFE, stacksize);
	tos = (ulong *)(&np->stack[stacksize&(~7)]);
	FIX1;
	*--tos = (ulong)arg;
	*--tos = (ulong)f;
	FIX2;
	np->stacksize = stacksize;
	np->stackptr = tos;
	np->grp = p->curthread->grp;
	np->launcher = (long)launcher;
	np->rforkflag = rforkflag;

	_threaddebug(DBGPROC, "newproc, switch stacks\n");
	/* Goto unshared stack and fire up new process */
	p->arg = np;
	longjmp(p->oldenv, DOPROC);
	/* no return */
	return -1;
}

int
proccreate(void (*f)(void*), void *arg, uint stacksize) {
	return procrfork(f, arg, stacksize, 0);
}

int
threadcreate(void (*f)(void *arg), void *arg, uint stacksize) {
	Thread *child;
	ulong *tos;
	Proc *p;

	p = *procp;
	if (stacksize < 32) {
		werrstr("%s", "stacksize");
		return -1;
	}
	if ((child = _threadmalloc(sizeof(Thread))) == nil ||
		(child->stk = _threadmalloc(child->stksize = stacksize)) == nil) {
			if (child) free(child);
			werrstr("%s", "_threadmalloc");
			return -1;
	}
	memset(child->stk, 0xFE, stacksize);
	p->nthreads++;
	child->cmdname = nil;
	child->id = nextID();
	child->proc = p;
	tos = (ulong *)(&child->stk[stacksize&~7]);
	FIX1;
	*--tos = (ulong)arg;
	*--tos = (ulong)f;
	FIX2;	/* Insert a dummy argument on 386 */
	child->env[JMPBUFPC] = ((ulong)launcher+JMPBUFDPC);
	/* -STACKOFF leaves room for old pc and new pc in frame */
	child->env[JMPBUFSP] = (ulong)(tos - STACKOFF);
	child->state = Runnable;
	child->exiting = 0;
	child->nextt = nil;
	if (p->threads.head == nil) {
		p->threads.head = p->threads.tail = child;
	} else {
		p->threads.tail->nextt = child;
		p->threads.tail = child;
	}
	child->next = (Thread *)~0;
	child->garbage = nil;
	putq(&p->runnable, child);
	return child->id;
}

void
procexecl(Channel *pidc, char *f, ...) {

	procexec(pidc, f, &f+1);
}

void
procexec(Channel *pidc, char *f, char *args[]) {
	Proc *p;
	Dir d;
	int n, pid;
	Execproc *ep;
	Channel *c;
	char *q;
	int s, l;

	/* make sure exec is likely to succeed before tossing thread state */
	if(dirstat(f, &d) < 0 || (d.mode & CHDIR) || access(f, AEXEC) < 0) {
bad:	if (pidc) sendul(pidc, ~0);
		return;
	}
	p = *procp;
	if(p->threads.head != p->curthread || p->threads.head->nextt != nil)
		goto bad;

	n = 0;
	while (args[n++])
		;
	ep = (Execproc*)procp;
	q = ep->data;
	s = sizeof(ep->data);
	if ((l = n*sizeof(char *)) < s) {
		_threaddebug(DBGPROC, "args on stack, %d pointers\n", n);
		ep->arg = (char **)q;
		q += l;
		s -= l;
	} else
		ep->arg = _threadmalloc(l);	/* will be leaked */
	if ((l = strlen(f) + 1) < s) {
		ep->file = q;
		strcpy(q, f);
		_threaddebug(DBGPROC, "file on stack, %s\n", q);
		q += l;
		s -= l;
	} else
		ep->file = strdup(f);	/* will be leaked */
	ep->arg[--n] = nil;
	while (--n >= 0)
		if ((l = strlen(args[n]) + 1) < s) {
			ep->arg[n] = q;
			strcpy(q, args[n]);
			_threaddebug(DBGPROC, "arg on stack, %s\n", q);
			q += l;
			s -= l;
		} else
			ep->arg[n] = strdup(args[n]);	/* will be leaked */
	/* committed to exec-ing */
	if ((pid = setjmp(p->curthread->env))) {
		int wpid, i;
		Waitmsg w;

		rfork(RFFDG);
		close(0);
		close(1);
		for (i = 3; i < 100; i++)
			close(i);
		if(pidc != nil)
			send(pidc, &pid);
		/* wait for exec-ed child */
		while ((wpid = wait(&w)) != pid)
			threadassert(wpid != -1);
		if ((c = thewaitchan) != nil) {	/* global is exposed */
			_threaddebug(DBGCHLD, "Sending exit status for exec: real pid %d, fake pid %d, status %s\n", pid, getpid(), w.msg);
			send(c, &w);
		}
		_threaddebug(DBGPROC, "Exiting (exec)\n");
		threadexits("libthread procexec");
	}
	p->arg = ep;
	longjmp(p->oldenv, DOEXEC);
	/* No return; */
}

Channel *
threadwaitchan(void) {
	static void *arg[1];

	thewaitchan = chancreate(sizeof(Waitmsg), 0);
	return thewaitchan;
}

int
threadgetgrp(void) {
	return (*procp)->curthread->grp;
}

int
threadsetgrp(int ng) {
	int og;

	og = (*procp)->curthread->grp;
	(*procp)->curthread->grp = ng;
	return og;
}

void *
_threadmalloc(long size) {
	void *m;

	m = malloc(size);
	if (m == nil) {
		fprint(2, "Malloc of size %ld failed: %r\n", size);
		abort();
	}
	setmalloctag(m, getcallerpc(&size));
	totalmalloc += size;
	if (size > 1000000) {
		fprint(2, "Malloc of size %ld, total %ld\n", size, totalmalloc);
		abort();
	}
	return m;
}

void
threadnonotes(void)
{
	char buf[30];
	int fd;

	sprint(buf, "/proc/%d/note", passerpid);
	if((fd = open(buf, OWRITE)) >= 0)
		write(fd, "kilpasser", 9);
	close(fd);
}


static void
runproc(Proc *p)
{
	int action, pid, rforkflag;
	Proc *pp;
	long r;
	int id;
	char str[ERRLEN];

	r = ~0;
runp:
	/* Leave a proc manager */
	while ((action = setjmp(p->oldenv))) {
		Newproc *np;
		Execproc *ne;

		p = *procp;
		switch(action) {
		case DOEXEC:
			ne = (Execproc *)p->arg;
			if ((pid = rfork(RFPROC|RFREND|RFNOTEG|RFFDG)) < 0) {
				exits("doexecproc: fork: %r");
			}
			if (pid == 0) {
				exec(ne->file, ne->arg);
				if (_threaddebuglevel & DBGPROC) {
					char **a;
					fprint(2, "%s: ", ne->file);
					a = ne->arg;
					while(*a)
						fprint(2, "%s, ", *a++);
					fprint(2, "0\n");
				}
				exits("Can't exec");
				/* No return */
			}
			longjmp(p->curthread->env, pid);
			/* No return */
		case DOEXIT:
			_threaddebug(DBGPROC, "at doexit\n");
			lock(&pq.lock);
			if (pq.head == p) {
				pq.head = p->next;
				if (pq.tail == p) {
					pq.tail = nil;
					postnote(PNPROC, passerpid, "kilpasser");
				}
			} else {
				for (pp = pq.head; pp->next; pp = pp->next) {
					if(pp->next == p) {
						pp->next = p->next;
						if (pq.tail == p)
							pq.tail = pp;
						break;
					}
				}
			}
			unlock(&pq.lock);
			strncpy(str, p->str, sizeof(str));
			garbageproc(p);
			exits(str);
		case DOPROC:
			_threaddebug(DBGPROC, "at doproc\n");
			np = (Newproc *)p->arg;
			pp = prepproc(np);
			id = pp->curthread->id;	// get id before fork() to avoid race
			rforkflag = np->rforkflag;
			free(np);
			if ((pid = rfork(RFPROC|RFMEM|RFNOWAIT|rforkflag)) < 0) {
				exits("donewproc: fork: %r");
			}
			if (pid == 0) {
				/* Child is the new proc; touch old proc struct no more */
				p = pp;
				p->pid = getpid();
				goto runp;
				/* No return */
			}
			/* Parent, return to caller */
			r = id;			// better than returning pid
			_threaddebug(DBGPROC, "newproc, unswitch stacks\n");
			break;
		default:
			/* `Can't happen' */
			threadprint(2, "runproc, can't happen: %d\n", action);
			threadassert(0);
		}
	}
	if (_threaddebuglevel & DBGPROC)
		threadprint(2, "runproc, longjmp\n");
	/* Jump into proc */
	*procp = p;
	longjmp(p->curthread->env, r);
	/* No return */
}

static void
initproc(void (*f)(ulong, int argc, char *argv[]), int argc, char *argv[], uint stacksize) {
	Proc *p;
	Newproc *np;
	ulong *tos;
	ulong *av;
	int i;
	Execproc	ex;

	procp = (Proc **)&ex;	/* address of the execproc struct */

	if (_threaddebuglevel & DBGPROC)
		threadprint(2, "Initproc, f = 0x%p, argc = %d, argv = 0x%p\n",
			f, argc, argv);
	/* Create a stack and fill it */
	np = _threadmalloc(sizeof(Newproc));
	threadassert(np != nil);
	np->stack = _threadmalloc(stacksize);
	threadassert(np->stack != nil);
	memset(np->stack, 0xFE, stacksize);
	tos = (ulong *)(&np->stack[stacksize&(~7)]);
	np->stacksize = stacksize;
	np->rforkflag = 0;
	np->grp = 0;
	for (i = 0; i < argc; i++){
		char *nargv;
	
		nargv = (char *)tos - (strlen(argv[i]) + 1);
		strcpy(nargv, argv[i]);
		argv[i] = nargv;
		tos = (ulong *)nargv;
	}
	/* round down to address of char* */
	tos = (ulong *)((ulong)tos & ~0x3);
	tos -= argc + 1;
	/* round down to address of vlong (for the alpha): */
	tos = (ulong *)((ulong)tos & ~0x7);
	av = tos;
	memmove(av, argv, (argc+1)*sizeof(char *));
	FIX1;
	*--tos = (ulong)av;
	*--tos = (ulong)argc;
	FIX2;
	np->stackptr = tos;
	np->launcher = (long)f;
	p = prepproc(np);
	p->pid = getpid();
	free(np);
	if (_threaddebuglevel & DBGPROC)
		threadprint(2, "calling runproc\n");
	runproc(p);
	/* no return; */
}

static void
garbageproc(Proc *p) {
	Thread *t, *nextt;

	for (t = p->threads.head; t; t = nextt) {
		if (t->cmdname)
			free(t->cmdname);
		threadassert(t->stk != nil);
		free(t->stk);
		nextt = t->nextt;
		free(t);
	}
	free(p);
}

static void
garbagethread(Thread *t) {
	Proc *p;
	Thread *r, *pr;

	p = t->proc;
	threadassert(*procp == p);
	pr = nil;
	for (r = p->threads.head; r; r = r->nextt) {
		if (r == t)
			break;
		pr = r;
	}
	threadassert (r != nil);
	if (pr)
		pr->nextt = r->nextt;
	else
		p->threads.head = r->nextt;
	if (p->threads.tail == r)
		p->threads.tail = pr;
	if (t->cmdname)
		free(t->cmdname);
	threadassert(t->stk != nil);
	free(t->stk);
	free(t);
}

static void
putq(Tqueue *q, Thread *t) {
	lock(&q->lock);
	_threaddebug(DBGQUE, "Putq 0x%lux on 0x%lux, next == 0x%lux",
		(ulong)t, (ulong)q, (ulong)t->next);
	threadassert((ulong)(t->next) == (ulong)~0);
	t->next = nil;
	if (q->head == nil) {
		q->head = t;
		q->tail = t;
	} else {
		threadassert(q->tail->next == nil);
		q->tail->next = t;
		q->tail = t;
	}
	unlock(&q->lock);
}

static Thread *
getq(Tqueue *q) {
	Thread *t;

	lock(&q->lock);
	if ((t = q->head)) {
		q->head = q->head->next;
		t->next = (Thread *)~0;
	}
	unlock(&q->lock);
	_threaddebug(DBGQUE, "Getq 0x%lux from 0x%lux", (ulong)t, (ulong)q);
	return t;
}

static Thread *
getqbytag(Tqueue *q, ulong tag) {
	Thread *r, *pr, *w, *pw;

	w = pr = pw = nil;
	_threaddebug(DBGQUE, "Getqbytag 0x%lux", q);
	lock(&q->lock);
	for (r = q->head; r; r = r->next) {
		if (r->tag == tag) {
			w = r;
			pw = pr;
			if (r->proc == *procp) {
				/* Locals or blocked remotes are best */
				break;
			}
		}
		pr = r;
	}
	if (w) {
		if (pw)
			pw->next = w->next;
		else
			q->head = w->next;
		if (q->tail == w)
			q->tail = pw;
		w->next = (Thread *)~0;
	}
	unlock(&q->lock);
	_threaddebug(DBGQUE, "Getqbytag 0x%lux from 0x%lux", w, q);
	return w;
}

static void
launcher(ulong, void (*f)(void *arg), void *arg) {
	Proc *p;
	Thread *t;

	p = *procp;
	t = p->curthread;
	if (t->garbage) {
		garbagethread(t->garbage);
		t->garbage = nil;
	}

	(*f)(arg);
	threadexits(nil);
	/* no return */
}

static void
mainlauncher(ulong, int argc, char *argv[]) {
/*	ulong *p; */

/*	p = (ulong *)&argc; */
/*	fprint(2, "p[-2..2]: %lux %lux %lux %lux %lux\n", */
/*		p[-2], p[-1], p[0], p[1], p[2]); */

	threadmain(argc, argv);
	threadexits(nil);
	/* no return */
}

static Proc *
prepproc(Newproc *np) {
	Proc *p;
	Thread *t;

	/* Create proc and thread structs */
	p = _threadmalloc(sizeof(Proc));
	t = _threadmalloc(sizeof(Thread));
	if (p == nil || t == nil) {
		char err[ERRLEN] = "";
		errstr(err);
		write(2, err, strlen(err));
		write(2, "\n", 1);
		exits("procinit: _threadmalloc");
	}
	memset(p, 0, sizeof(Proc));
	memset(t, 0, sizeof(Thread));
	t->cmdname = strdup("threadmain");
	t->id = nextID();
	t->grp = np->grp;	/* Inherit grp id */
	t->proc = p;
	t->state = Running;
	t->nextt = nil;
	t->next = (Thread *)~0;
	t->garbage = nil;
	t->stk = np->stack;
	t->stksize = np->stacksize;
	t->env[JMPBUFPC] = (np->launcher+JMPBUFDPC);
	/* -STACKOFF leaves room for old pc and new pc in frame */
	t->env[JMPBUFSP] = (ulong)(np->stackptr - STACKOFF);
/*fprint(2, "SP = %lux\n", t->env[JMPBUFSP]); */
	p->curthread = t;
	p->threads.head = p->threads.tail = t;
	p->nthreads = 1;
	p->pid = 0;
	p->next = nil;

	lock(&pq.lock);
	if (pq.head == nil) {
		pq.head = pq.tail = p;
	} else {
		threadassert(pq.tail->next == nil);
		pq.tail->next = p;
		pq.tail = p;
	}
	unlock(&pq.lock);

	return p;
}

int
threadprint(int fd, char *fmt, ...)
{
	int n;
	va_list arg;
	char *buf;

	buf = _threadmalloc(1024);
	if(buf == nil)
		return -1;
	va_start(arg, fmt);
	doprint(buf, buf+1024, fmt, arg);
	va_end(arg);
	n = write(fd, buf, strlen(buf));
	free(buf);
	return n;
}

static int
setnoteid(int id)
{
	char buf[40];
	int n, fd;

	sprint(buf, "/proc/%d/noteid", getpid());
	if((fd = open(buf, OWRITE)) < 0)
		return -1;

	sprint(buf, "%d", id);
	n = write(fd, buf, strlen(buf));
	close(fd);
	if(n != strlen(buf))
		return -1;
	return 0;
}

static int
getnoteid(void)
{
	char buf[40];
	int n, fd;

	sprint(buf, "/proc/%d/noteid", getpid());
	if((fd = open(buf, OREAD)) < 0)
		return -1;

	n = read(fd, buf, 20);
	close(fd);
	if(n < 0)
		return -1;

	buf[n] = '\0';
	return atoi(buf);
}
