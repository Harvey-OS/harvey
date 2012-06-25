#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

static int timernotefd;
static void timerproc(void*);

static int
pidhash(int pid)
{
	return (pid - 1) % MAXPROC;
}

Uproc*
getproc(int tid)
{
	Uproc *p;

	if(tid > 0){
		p = &proctab.proc[pidhash(tid)];
		if(p->tid == tid)
			return p;
	}
	return nil;
}

Uproc*
getprocn(int n)
{
	Uproc *p;

	p = &proctab.proc[n];
	if(p->tid > 0)
		return p;
	return nil;
}

static Uproc*
allocproc(void)
{
	Uproc *p;
	int tid, i;

	for(i=0; i<MAXPROC; i++){
		tid = proctab.nextpid++;
		p = &proctab.proc[pidhash(tid)];
		if(p->tid <= 0){
			proctab.alloc++;

			p->tid = tid;
			p->pid = tid;
			p->pgid = tid;
			p->psid = tid;
			return p;
		}
	}

	trace("allocproc(): out of processes");
	return nil;
}

static void
freeproc(Uproc *p)
{
	Uwait *w;

	while(w = p->freewait){
		p->freewait = w->next;
		free(w);
	}
	exittrace(p);
	free(p->comm);
	free(p->root);
	free(p->cwd);
	free(p->kcwd);
	memset(p, 0, sizeof(*p));
	proctab.alloc--;
}

void initproc(void)
{
	Uproc *p;
	char buf[1024];
	int pid;

	proctab.nextpid = 10;

	p = allocproc();
	p->kpid = getpid();
	snprint(buf, sizeof(buf), "/proc/%d/note", p->kpid);
	p->notefd = open(buf, OWRITE);
	snprint(buf, sizeof(buf), "/proc/%d/args", p->kpid);
	p->argsfd = open(buf, ORDWR);

	current = p;

	inittrace();
	inittime();
	initsignal();
	initmem();
	inittls();
	initfile();

	if((pid = procfork(timerproc, nil, 0)) < 0)
		panic("initproc: unable to fork timerproc: %r");

	snprint(buf, sizeof(buf), "/proc/%d/note", pid);
	timernotefd = open(buf, OWRITE);

	current->root = nil;
	current->cwd = kstrdup(getwd(buf, sizeof(buf)));
	current->kcwd = kstrdup(current->cwd);
	current->linkloop = 0;
	current->starttime = nsec();

	inittrap();
}

void
setprocname(char *s)
{
	if(current == nil){
		char buf[32];
		int fd;

		snprint(buf, sizeof(buf), "/proc/%d/args", getpid());
		if((fd = open(buf, OWRITE)) >= 0){
			write(fd, s, strlen(s));
			close(fd);
		}
	} else {
		write(current->argsfd, s, strlen(s));
	}
}

static void
intrnote(void *, char *msg)
{
	if(strncmp(msg, "interrupt", 9) == 0)
		noted(NCONT);
	noted(NDFLT);
}

struct kprocforkargs
{
	int	flags;
	void	(*func)(void *aux);
	void	*aux;
};

static int
kprocfork(void *arg)
{
	struct kprocforkargs args;
	int pid;

	memmove(&args, arg, sizeof(args));

	if((pid = rfork(RFPROC|RFMEM|args.flags)) != 0)
		return pid;

	notify(intrnote);

	unmapuserspace();
	current = nil;

	profme();
	args.func(args.aux);
	longjmp(exitjmp, 1);
	return -1;
}

/*
 * procfork starts a kernel process running on kstack.
 * that process will have linux memory segments (stack, private,
 * shared) unmapped but plan9 segments (text, bss, stack) shared.
 * here is no Uproc associated with it! current will be set to nil so
 * you cant call sys_????() functions in here.
 * procfork returns the plan9 pid. (usefull for posting notes)
 */
int procfork(void (*func)(void *aux), void *aux, int flags)
{
	struct kprocforkargs args;

	args.flags = flags;
	args.func = func;
	args.aux = aux;

	return onstack(kstack, kprocfork, &args);
}

static void *Intr = (void*)~0;

static char Notifyme[] = "notifyme";
static char Wakeme[] = "wakeme";
static char Xchange[] = "xchange";

static char Wakeup[] = "wakeup";
static char Abort[] = "abort";

int notifyme(int on)
{
	Uproc *p;

	p = current;
	qlock(p);
	if(on){
		if(p->notified || signalspending(p)){
			qunlock(p);
			return 1;
		}
		if(p->state == nil)
			p->state = Notifyme;
	} else {
		p->state = nil;
	}
	qunlock(p);
	return 0;
}

void wakeme(int on)
{
	Uproc *p;

	p = current;
	qlock(p);
	if(on){
		if(p->state == nil)
			p->state = Wakeme;
	} else {
		p->state = nil;
	}
	qunlock(p);
}

int sleepproc(QLock *l, int flags)
{
	Uproc *p;
	void *ret;
	char *x;

	p = current;
	qlock(p);
	x = p->state;
	if(x == nil || x == Wakeme){
		p->xstate = x;
		p->state = Xchange;
		if(l != nil)
			qunlock(l);
		qunlock(p);
		if(flags && signalspending(p)){
			ret = Intr;
		} else {
			ret = rendezvous(p, Xchange);
		}
		if(ret == Intr){
			qlock(p);
			if(p->state != Xchange){
				while((ret = rendezvous(p, Xchange)) == Intr)
					;
			} else {
				p->state = x;
			}
			qunlock(p);
		}
		if(l != nil)
			qlock(l);
	} else {
		p->state = Wakeme;
		ret = x;
		qunlock(p);
	}
	return (ret == Wakeup) ? 0 : -ERESTART;
}

static int
wakeup(Uproc *proc, char *m, int force)
{
	char *x;

	if(proc != nil){
		qlock(proc);
		x = proc->state;

		if(x == Wakeme){
			proc->state = m;
			qunlock(proc);
			return 1;
		}
		if(x == Xchange){
			proc->state = proc->xstate;
			proc->xstate = nil;
			qunlock(proc);
			while(rendezvous(proc, m) == Intr)
				;
			return 1;
		}
		if((m != Wakeup) && (proc->notified == 0)){
			if(x == Notifyme)
				proc->state = nil;
			if(x == Notifyme || force){
				proc->notified = 1;
				qunlock(proc);
				write(proc->notefd, "interrupt", 9);
				return 1;
			}
		}
		qunlock(proc);
	}
	return 0;
}

Uwait* addwaitq(Uwaitq *q)
{
	Uproc *p;
	Uwait *w;

	p = current;
	if(w = p->freewait){
		p->freewait = w->next;
	} else {
		w = kmalloc(sizeof(*w));
	}

	w->next = nil;

	w->proc = p;
	w->file = nil;

	w->q = q;
	qlock(q);
	w->nextq = q->w;
	q->w = w;
	qunlock(q);

	return w;
}

void delwaitq(Uwait *w)
{
	Uwaitq *q;
	Uwait **x;

	q = w->q;
	qlock(q);
	for(x = &q->w; *x; x=&((*x)->nextq)){
		if(*x == w){
			*x = w->nextq;
			break;
		}
	}
	qunlock(q);

	w->q = nil;
	w->nextq = nil;

	w->proc = nil;
	putfile(w->file);
	w->file = nil;

	w->next = current->freewait;
	current->freewait = w;
}

int requeue(Uwaitq *q1, Uwaitq *q2, int nrequeue)
{
	int n;
	Uwait *w;

	n = 1000;
	for(;;){
		qlock(q1);
		if(canqlock(q2))
			break;
		qunlock(q1);	
		if(--n <= 0)
			return 0;
		sleep(0);
	}
	n = 0;
	while((w = q1->w) && (n < nrequeue)){
		q1->w = w->nextq;
		w->q = q2;
		w->nextq = q2->w;
		q2->w = w;
		n++;
	}
	qunlock(q2);
	qunlock(q1);
	return n;
}

int wakeq(Uwaitq *q, int nwake)
{
	int n;
	Uwait *w;

	n = 0;
	if(q != nil){
		qlock(q);
		for(w = q->w; w && n < nwake; w=w->nextq)
			n += wakeup(w->proc, Wakeup, 0);
		qunlock(q);
	}
	return n;
}

int sleepq(Uwaitq *q, QLock *l, int flags)
{
	Uwait *w;
	int ret;

	w = addwaitq(q);
	ret = sleepproc(l, flags);
	delwaitq(w);

	return ret;
}

static Uproc *alarmq;

int
procsetalarm(Uproc *proc, vlong t)
{
	Uproc **pp;
	int ret;

	if(proc->alarm && t >= proc->alarm)
		return 0;
	ret = (alarmq == nil) || (t < alarmq->alarm);
	for(pp = &alarmq; *pp; pp = &((*pp)->alarmq)){
		if(*pp == proc){
			*pp = proc->alarmq;
			break;
		}
	}
	for(pp = &alarmq; *pp; pp = &((*pp)->alarmq))
		if((*pp)->alarm > t)
			break;
	proc->alarm = t;
	proc->alarmq = *pp;
	*pp = proc;
	return ret;
}

void
setalarm(vlong t)
{
	qlock(&proctab);
	if(procsetalarm(current, t))
		write(timernotefd, "interrupt", 9);
	qunlock(&proctab);
}

/* signal.c */
extern void alarmtimer(Uproc *proc, vlong now);

static void
timerproc(void*)
{
	Uproc *h;
	vlong now;
	long m;

	setprocname("timerproc()");

	while(proctab.alloc > 0){
		qlock(&proctab);
		m = 2000;
		now = nsec();
		while(h = alarmq){
			if(now < h->alarm){
				m = (h->alarm - now) / 1000000;
				break;
			}
			alarmq = h->alarmq;
			h->alarm = 0;
			h->alarmq = nil;
			if(h->timeout){
				 if(now >= h->timeout){
					h->timeout = 0;
					wakeup(h, Wakeup, 0);
				} else
					procsetalarm(h, h->timeout);
			}
			alarmtimer(h, now);
		}
		qunlock(&proctab);
		sleep((m + (1000/HZ-1))/(1000/HZ));
	}
}

/*
static void
timerproc(void *)
{
	Uproc *p;
	vlong expire, now, wake, dead;
	int err, i, alive;
	char c;

	setprocname("timerproc()");
	dead = 0;
	for(;;){
		qlock(&proctab);
		now = nsec();
		wake = now + 60000000000LL;
		alive = 0;
		for(i=0; i<MAXPROC; i++){
			if((p = getprocn(i)) == nil)
				continue;
			if(p->wstate & WEXITED)
				continue;
			if(p->kpid <= 0)
				continue;

			if(now >= dead){
				if(read(p->argsfd, &c, 1) < 0){
					err = mkerror();
					if(err != -EINTR && err != -ERESTART){
						p->kpid = 0;
						qunlock(&proctab);
						exitproc(p, SIGKILL, 1);
						qlock(&proctab);
						continue;	
					}	
				}
			}
			alive++;
			expire = p->timeout;
			if(expire > 0){
				if(now >= expire){
					p->timeout = 0;
					wakeup(p, Wakeup, 0);
				} else {
					if(expire < wake)
						wake = expire;
				}
			}
			expire = alarmtimer(p, now, wake);
			if(expire < wake)
				wake = expire;
		}
		qunlock(&proctab);

		if(now >= dead)
			dead = now + 5000000000LL;
		if(dead < wake)
			wake = dead;
		if(alive == 0)
			break;
		wake -= now;

		sleep(wake/1000000LL);
	}
}
*/

int sys_waitpid(int pid, int *pexit, int opt)
{
	int i, n, m, status;
	Uproc *p;

	trace("sys_waitpid(%d, %p, %d)", pid, pexit, opt);

	m = WEXITED;
	if(opt & WUNTRACED)
		m |= WSTOPPED;
	if(opt & WCONTINUED)
		m |= WCONTINUED;

	qlock(&proctab);
	for(;;){
		n = 0;
		for(i=0; i<MAXPROC; i++){
			if((p = getprocn(i)) == nil)
				continue;
			if(p == current)
				continue;
			if((p->exitsignal != SIGCHLD) && (opt & (WALL|WCLONE))==0)
				continue;
			if(p->ppid != current->pid)
				continue;
			if(pid > 0){
				if(p->pid != pid)
					continue;
			} else if(pid == 0){
				if(p->pgid != current->pgid)
					continue;
			} else if(pid < -1){
				if(p->pgid != -pid)
					continue;
			}
			n++;
			trace("sys_waitpid(): child %d wstate %x", p->pid, p->wstate);
			if(p->wevent & m)
				goto found;
		}
		if(n == 0){
			qunlock(&proctab);
			trace("sys_waitpid(): no children we can wait for");
			return -ECHILD;
		}
		if(opt & WNOHANG){
			qunlock(&proctab);
			trace("sys_waitpid(): no exited/stoped/cont children");
			return 0;
		}
		if((i = sleepproc(&proctab, 1)) < 0){
			qunlock(&proctab);
			return i;
		}
	}

found:
	pid = p->pid;
	status = p->exitcode;
	p->wevent &= ~(p->wevent & m);
	if(p->wstate & WEXITED){
		trace("sys_waitpid(): found zombie %d exitcode %d", pid, status);
		freeproc(p);
	}
	qunlock(&proctab);
	if(pexit)
		*pexit = status;
	return pid;
}

struct linux_rusage { 
    struct linux_timeval ru_utime; /* user time used */ 
    struct linux_timeval ru_stime; /* system time used */ 
    long   ru_maxrss;        /* maximum resident set size */ 
    long   ru_ixrss;         /* integral shared memory size */ 
    long   ru_idrss;         /* integral unshared data size */ 
    long   ru_isrss;         /* integral unshared stack size */ 
    long   ru_minflt;        /* page reclaims */ 
    long   ru_majflt;        /* page faults */ 
    long   ru_nswap;         /* swaps */ 
    long   ru_inblock;       /* block input operations */ 
    long   ru_oublock;       /* block output operations */ 
    long   ru_msgsnd;        /* messages sent */ 
    long   ru_msgrcv;        /* messages received */ 
    long   ru_nsignals;      /* signals received */ 
    long   ru_nvcsw;         /* voluntary context switches */ 
    long   ru_nivcsw;        /* involuntary context switches */ 
}; 

int sys_wait4(int pid, int *pexit, int opt, void *prusage)
{
	int ret;
	struct linux_rusage *ru = prusage;

	trace("sys_wait4(%d, %p, %d, %p)", pid, pexit, opt, prusage);

	ret = sys_waitpid(pid, pexit, opt);
	if(ru != nil)
		memset(ru, 0, sizeof(*ru));

	return ret;
}

int
threadcount(int pid)
{
	Uproc *p;
	int i, n;

	n = 0;
	for(i = 0; i<MAXPROC; i++){
		p = getprocn(i);
		if(p != nil && p->pid == pid)
			n++;
	}
	return n;
}

int
killproc(Uproc *p, Usiginfo *info, int group)
{
	int i, n;
	Uproc *w;
	int sig, err;

	if((err = sendsignal(p, info, group)) <= 0)
		return err;
	w = p;
	sig = info->signo;
	if(group && !wantssignal(w, sig)){
		for(i=1, n = p->tid + 1; i<MAXPROC; i++, n++){
			if((p = getprocn(pidhash(n))) == nil)
				continue;
			if(p->pid != w->pid)
				continue;
			if(!wantssignal(p, info->signo))
				continue;
			w = p;
			break;
		}
	}
	wakeup(w, Abort, (sig == SIGKILL || sig == SIGSTOP || sig == SIGALRM));
	return 0;
}

enum
{
	CLD_EXITED		= 1,
	CLD_KILLED,
	CLD_DUMPED,
	CLD_TRAPPED,
	CLD_STOPPED,
	CLD_CONTINUED,
};

/*
 * queue the exit signal into the parent process. this
 * doesnt do the wakeup like killproc(). 
  */
static int
sendexitsignal(Uproc *parent, Uproc *proc, int sig, int code)
{
	Usiginfo si;

	memset(&si, 0, sizeof(si));
	switch(si.signo = sig){
	case SIGCHLD:
		switch(code & 0xFF){
		case 0:
			si.code = CLD_EXITED;
			break;
		case SIGSTOP:
			si.code = CLD_STOPPED;
			break;
		case SIGCONT:
			si.code = CLD_CONTINUED;
			break;
		case SIGKILL:
			si.code = CLD_KILLED;
			break;
		default:
			si.code = CLD_DUMPED;
			break;
		}
		si.chld.pid = proc->pid;
		si.chld.uid = proc->uid;
		si.chld.status = code;
	}
	return sendsignal(parent, &si, 1);
}

/*
 * wakeup all threads who are in the same thread group
 * as p including p. must be called with proctab locked.
 */
static void
wakeupall(Uproc *p, char *m, int force)
{
	int pid, i, n;

	pid = p->pid;
	for(i=0, n = p->tid; i<MAXPROC; i++, n++)
		if(p = getprocn(pidhash(n)))
			if(p->pid == pid)
				wakeup(p, m, force);
}

static void
zap(void *)
{
	exitproc(current, 0, 0);
}

void
zapthreads(void)
{
	Uproc *p;
	int i, n, z;

	for(;;){
		z = 0;
		for(i=1, n = current->tid+1; i<MAXPROC; i++, n++){
			if((p = getprocn(pidhash(n))) == nil)
				continue;
			if(p->pid != current->pid || p == current)
				continue;
			if(p->kpid <= 0)
				continue;

			trace("zapthreads() zapping thread %p", p);
			p->tracearg = current;
			p->traceproc = zap;
			wakeup(p, Abort, 1);
			z++;
		}
		if(z == 0)
			break;
		sleepproc(&proctab, 0);
	}
}

struct kexitprocargs
{
	Uproc	*proc;
	int		code;
	int		group;
};

#pragma profile off

static int
kexitproc(void *arg)
{
	struct kexitprocargs *args;
	Uproc *proc;
	int code, group;
	Uproc *parent, *child, **pp;
	int i;

	args = arg;
	proc = args->proc;
	code = args->code;
	group = args->group;

	if(proc == current){
		trace("kexitproc: cleartidptr = %p", proc->cleartidptr);
		if(okaddr(proc->cleartidptr, sizeof(*proc->cleartidptr), 1))
			*proc->cleartidptr = 0;
		sys_futex((ulong*)proc->cleartidptr, 1, MAXPROC, nil, nil, 0);

		qlock(&proctab);
		exitsignal();
		qunlock(&proctab);

		exitmem();
	}

	exitfile(proc);

	close(proc->notefd); proc->notefd = -1;
	close(proc->argsfd); proc->argsfd = -1;

	qlock(&proctab);

	for(pp = &alarmq; *pp; pp = &((*pp)->alarmq)){
		if(*pp == proc){
			*pp = proc->alarmq;
			proc->alarmq = nil;
			break;
		}
	}

	/* reparent children, and reap when zombies */
	for(i=0; i<MAXPROC; i++){
		if((child = getprocn(i)) == nil)
			continue;
		if(child->ppid != proc->pid)
			continue;
		child->ppid = 0;
		if(child->wstate & WEXITED)
			freeproc(child);
	}

	/* if we got zapped, just free the proc and wakeup zapper */
	if((proc == current) && (proc->traceproc == zap) && (parent = proc->tracearg)){
		freeproc(proc);
		wakeup(parent, Wakeup, 0);
		goto zapped;
	}

	if(group && proc == current)
		zapthreads();

	parent = getproc(proc->ppid);
	if((threadcount(proc->pid)==1) && parent && 
		(proc->exitsignal == SIGCHLD) && !ignoressignal(parent, SIGCHLD)){

		/* we are zombie */
		proc->exitcode = code;
		proc->wstate = WEXITED;
		proc->wevent = proc->wstate;
		if(proc == current){
			current->kpid = 0;
			sendexitsignal(parent, proc, proc->exitsignal, code);
			wakeupall(parent, Abort, 0);
			qunlock(&proctab);
			longjmp(exitjmp, 1);
		} else {
			sendexitsignal(parent, proc, proc->exitsignal, code);
		}
	} else {
		/* we are clone */
		if(parent && proc->exitsignal > 0)
			sendexitsignal(parent, proc, proc->exitsignal, code);
		freeproc(proc);
	}
	if(parent)
		wakeupall(parent, Abort, 0);

zapped:
	qunlock(&proctab);

	if(proc == current)
		longjmp(exitjmp, 1);

	return 0;
}

void exitproc(Uproc *proc, int code, int group)
{
	struct kexitprocargs args;

	trace("exitproc(%p, %d, %d)", proc, code, group);

	args.proc = proc;
	args.code = code;
	args.group = group;

	if(proc == current){
		onstack(kstack, kexitproc, &args);
	} else {
		kexitproc(&args);
	}
}

struct kstoparg
{
	Uproc	*stopper;
	int		code;
};

static void
stop(void *aux)
{
	struct kstoparg *arg = aux;

	stopproc(current, arg->code, 0);
}

void stopproc(Uproc *proc, int code, int group)
{
	struct kstoparg *arg;
	Uproc *p, *parent;
	int i, n, z;

	trace("stopproc(%p, %d, %d)", proc, code, group);

	qlock(&proctab);
	proc->exitcode = code;
	proc->wstate = WSTOPPED;
	proc->wevent = proc->wstate;

	if((proc == current) && (proc->traceproc == stop) && (arg = proc->tracearg)){
		proc->traceproc = nil;
		proc->tracearg = nil;
		wakeup(arg->stopper, Wakeup, 0);
		qunlock(&proctab);
		return;
	}

	/* put all threads in the stopped state */
	arg = nil;
	while(group){
		if(arg == nil){
			arg = kmalloc(sizeof(*arg));
			arg->stopper = current;
			arg->code = code;
		}
		z = 0;
		for(i=1, n = proc->tid+1; i<MAXPROC; i++, n++){
			if((p = getprocn(pidhash(n))) == nil)
				continue;
			if(p->pid != proc->pid || p == proc)
				continue;
			if(p->kpid <= 0)
				continue;
			if(p->wstate & (WSTOPPED | WEXITED))
				continue;

			trace("stopproc() stopping thread %p", p);
			p->tracearg = arg;
			p->traceproc = stop;
			wakeup(p, Abort, 1);
			z++;
		}
		if(z == 0)
			break;
		sleepproc(&proctab, 0);
	}
	free(arg);

	if(parent = getproc(proc->ppid)){
		if(group && !ignoressignal(parent, SIGCHLD))
			sendexitsignal(parent, proc, SIGCHLD, code);
		wakeupall(parent, Abort, 0);
	}
	qunlock(&proctab);
}

void contproc(Uproc *proc, int code, int group)
{
	Uproc *p, *parent;
	int i, n;

	trace("contproc(%p, %d, %d)", proc, code, group);

	qlock(&proctab);
	proc->exitcode = code;
	proc->wstate = WCONTINUED;
	proc->wevent = proc->wstate;
	if(group){
		for(i=1, n = proc->tid+1; i<MAXPROC; i++, n++){
			if((p = getprocn(pidhash(n))) == nil)
				continue;
			if(p->pid != proc->pid || p == proc)
				continue;
			if(p->kpid <= 0)
				continue;
			if((p->wstate & WSTOPPED) == 0)
				continue;
			if(p->wstate & (WCONTINUED | WEXITED))
				continue;

			trace("contproc() waking thread %p", p);
			p->exitcode = code;
			p->wstate = WCONTINUED;
			p->wevent = p->wstate;
			wakeup(p, Wakeup, 0);
		}
	}
	if(parent = getproc(proc->ppid)){
		if(group && !ignoressignal(parent, SIGCHLD))
			sendexitsignal(parent, proc, SIGCHLD, code);
		wakeupall(parent, Abort, 0);
	}
	qunlock(&proctab);
}

int sys_exit(int code)
{
	trace("sys_exit(%d)", code);

	exitproc(current, (code & 0xFF)<<8, 0);
	return -1;
}

int sys_exit_group(int code)
{
	trace("sys_exit_group(%d)", code);

	exitproc(current, (code & 0xFF)<<8, 1);
	return -1;
}

struct kcloneprocargs
{
	int	flags;
	void	*newstack;
	int	*parenttidptr;
	void	*tlsdescr;
	int	*childtidptr;
};

static int
kcloneproc(void *arg)
{
	struct kcloneprocargs args;
	struct linux_user_desc tls;
	Ureg ureg;
	int rflags, pid, tid;
	char buf[80];
	Uproc *new;

	memmove(&args, arg, sizeof(args));
	memmove(&ureg, current->ureg, sizeof(ureg));
	if(args.flags & CLONE_SETTLS){
		if(!okaddr(args.tlsdescr, sizeof(tls), 0))
			return -EFAULT;
		memmove(&tls, args.tlsdescr, sizeof(tls));
	}

	qlock(&proctab);
	if((new = allocproc()) == nil){
		qunlock(&proctab);
		return -EAGAIN;
	}
	tid = new->tid;

	if(args.flags & CLONE_PARENT_SETTID){
		if(!okaddr(args.parenttidptr, sizeof(*args.parenttidptr), 1)){
			freeproc(new);
			qunlock(&proctab);
			return -EFAULT;
		}
		*args.parenttidptr = tid;
	}

	rflags = RFPROC;
	if(args.flags & CLONE_VM)
		rflags |= RFMEM;

	qlock(current);
	if((pid = rfork(rflags)) < 0){
		freeproc(new);
		qunlock(current);
		qunlock(&proctab);

		trace("kcloneproc(): rfork failed: %r");
		return mkerror();
	}

	if(pid){
		/* parent */
		new->kpid = pid;
		new->exitsignal = args.flags & 0xFF;
		new->innote = 0;
		new->ureg = &ureg;
		new->syscall = current->syscall;
		new->sysret = current->sysret;
		new->comm = nil;
		new->ncomm = 0;
		new->linkloop = 0;
		new->root = current->root ? kstrdup(current->root) : nil;
		new->cwd = kstrdup(current->cwd);
		new->kcwd = kstrdup(current->kcwd);
		new->starttime = nsec();

		snprint(buf, sizeof(buf), "/proc/%d/note", pid);
		new->notefd = open(buf, OWRITE);
		snprint(buf, sizeof(buf), "/proc/%d/args", pid);
		new->argsfd = open(buf, ORDWR);

		if(args.flags & (CLONE_THREAD | CLONE_PARENT)){
			new->ppid = current->ppid;
		} else {
			new->ppid = current->pid;
		}

		if(args.flags & CLONE_THREAD)
			new->pid = current->pid;

		new->cleartidptr = nil;
		if(args.flags & CLONE_CHILD_CLEARTID)
			new->cleartidptr = args.childtidptr;

		new->pgid = current->pgid;
		new->psid = current->psid;
		new->uid = current->uid;
		new->gid = current->gid;

		clonetrace(new, !(args.flags & CLONE_THREAD));
		clonesignal(new, !(args.flags & CLONE_SIGHAND), !(args.flags & CLONE_THREAD));
		clonemem(new, !(args.flags & CLONE_VM));
		clonefile(new, !(args.flags & CLONE_FILES));
		clonetls(new);
		qunlock(&proctab);

		while(rendezvous(new, 0) == (void*)~0)
			;

		qunlock(current);

		return tid;
	} 

	/* child */
	current = new;
	profme();

	/* wait for parent to copy our resources */
	while(rendezvous(new, 0) == (void*)~0)
		;

	trace("kcloneproc(): hello world");

	if(args.flags & CLONE_SETTLS)
		sys_set_thread_area(&tls);

	if(args.flags & CLONE_CHILD_SETTID)
		if(okaddr(args.childtidptr, sizeof(*args.childtidptr), 1))
			*args.childtidptr = tid;

	if(args.newstack != nil)
		current->ureg->sp = (ulong)args.newstack;
	current->sysret(0);
	retuser();

	return -1;
}

#pragma profile on

int sys_linux_clone(int flags, void *newstack, int *parenttidptr, int *tlsdescr, void *childtidptr)
{
	struct kcloneprocargs a;

	trace("sys_linux_clone(%x, %p, %p, %p, %p)", flags, newstack, parenttidptr, childtidptr, tlsdescr);

	a.flags = flags;
	a.newstack = newstack;
	a.parenttidptr = parenttidptr;
	a.childtidptr = childtidptr;
	a.tlsdescr = tlsdescr;

	return onstack(kstack, kcloneproc, &a);
}

int sys_fork(void)
{
	trace("sys_fork()");

	return sys_linux_clone(SIGCHLD, nil, nil, nil, nil);
}

int sys_vfork(void)
{
	trace("sys_vfork()");

	return sys_fork();
}

int sys_getpid(void)
{
	trace("sys_getpid()");

	return current->pid;
}

int sys_getppid(void)
{
	trace("sys_getppid()");

	return current->ppid;
}

int sys_gettid(void)
{
	trace("sys_gettid()");

	return current->tid;
}

int sys_setpgid(int pid, int pgid)
{
	int i, n;

	trace("sys_setpgid(%d, %d)", pid, pgid);

	if(pgid == 0)
		pgid = current->pgid;
	if(pid == 0)
		pid = current->pid;

	n = 0;
	qlock(&proctab);
	for(i=0; i<MAXPROC; i++){
		Uproc *p;

		if((p = getprocn(i)) == nil)
			continue;
		if(p->pid != pid)
			continue;

		p->pgid = pgid;
		n++;
	}
	qunlock(&proctab);

	return n ? 0 : -ESRCH;
}

int sys_getpgid(int pid)
{
	int i;
	int pgid;

	trace("sys_getpgid(%d)", pid);

	pgid = -ESRCH;
	if(pid == 0)
		return current->pgid;
	qlock(&proctab);
	for(i=0; i<MAXPROC; i++){
		Uproc *p;

		if((p = getprocn(i)) == nil)
			continue;
		if(p->pid != pid)
			continue;

		pgid = p->pgid;
		break;
	}
	qunlock(&proctab);

	return pgid;
}

int sys_getpgrp(void)
{
	trace("sys_getpgrp()");

	return sys_getpgid(0);
}

int sys_getuid(void)
{
	trace("sys_getuid()");

	return current->uid;
}

int sys_getgid(void)
{
	trace("sys_getgid()");

	return current->gid;
}

int sys_setuid(int uid)
{
	trace("sys_setuid(%d)", uid);

	current->uid = uid;
	return 0;
}

int sys_setgid(int gid)
{
	trace("sys_setgid(%d)", gid);

	current->gid = gid;
	return 0;
}

int sys_setresuid(int ruid, int euid, int suid)
{
	trace("sys_setresuid(%d, %d, %d)", ruid, euid, suid);

	return 0;
}

int sys_setresgid(int rgid, int egid, int sgid)
{
	trace("sys_setresgid(%d, %d, %d)", rgid, egid, sgid);

	return 0;
}
int sys_setreuid(int ruid, int euid)
{
	trace("sys_setreuid(%d, %d)", ruid, euid);

	return 0;
}

int sys_setregid(int rgid, int egid)
{
	trace("sys_setregid(%d, %d)", rgid, egid);

	return 0;
}

int sys_getresuid(int *ruid, int *euid, int *suid)
{
	trace("sys_getresuid(%p, %p, %p)", ruid, euid, suid);

	if(ruid == nil)
		return -EINVAL;
	if(euid == nil)
		return -EINVAL;
	if(suid == nil)
		return -EINVAL;

	*ruid = current->uid;
	*euid = current->uid;
	*suid = current->uid;

	return 0;
}

int sys_getresgid(int *rgid, int *egid, int *sgid)
{
	trace("sys_getresgid(%p, %p, %p)", rgid, egid, sgid);

	if(rgid == nil)
		return -EINVAL;
	if(egid == nil)
		return -EINVAL;
	if(sgid == nil)
		return -EINVAL;

	*rgid = current->gid;
	*egid = current->gid;
	*sgid = current->gid;

	return 0;
}

int sys_setsid(void)
{
	int i;

	trace("sys_setsid()");

	if(current->pid == current->pgid)
		return -EPERM;

	qlock(&proctab);
	for(i=0; i<MAXPROC; i++){
		Uproc *p;

		if((p = getprocn(i)) == nil)
			continue;
		if(p->pid != current->pid)
			continue;
		p->pgid = current->pid;
		p->psid = current->pid;
	}
	qunlock(&proctab);

	settty(nil);

	return current->pgid;
}

int sys_getsid(int pid)
{
	int i, pgid;

	trace("sys_getsid(%d)", pid);

	pgid = -ESRCH;
	if(pid == 0)
		pid = current->pid;
	qlock(&proctab);
	for(i=0; i<MAXPROC; i++){
		Uproc *p;

		if((p = getprocn(i)) == nil)
			continue;
		if(p->pid != pid)
			continue;
		if(p->pid != p->psid)
			continue;
		pgid = p->pgid;
		break;
	}
	qunlock(&proctab);

	return pgid;
}

int sys_getgroups(int size, int *groups)
{
	trace("sys_getgroups(%d, %p)", size, groups);
	if(size < 0)
		return -EINVAL;
	return 0;
}

int sys_setgroups(int size, int *groups)
{
	trace("sys_setgroups(%d, %p)", size, groups);
	return 0;
}

struct linux_utsname
{
	char sysname[65];
	char nodename[65];
	char release[65];
	char version[65];
	char machine[65];
	char domainname[65];
};

int sys_uname(void *a)
{
	struct linux_utsname *p = a;

	trace("sys_uname(%p)", a);

	strncpy(p->sysname, "Linux", 65);
	strncpy(p->nodename, sysname(), 65);
	strncpy(p->release, "2.6.11", 65);
	strncpy(p->version, "linuxemu", 65);
	strncpy(p->machine, "i386", 65);
	strncpy(p->domainname, sysname(), 65);

	return 0;
}

int sys_personality(ulong p)
{
	trace("sys_personality(%lux)", p);

	if(p != 0 && p != 0xffffffff)
		return -EINVAL;
	return 0;
}

int sys_tkill(int tid, int sig)
{
	int err;

	trace("sys_tkill(%d, %S)", tid, sig);

	err = -EINVAL;
	if(tid > 0){
		Uproc *p;

		err = -ESRCH;
		qlock(&proctab);
		if(p = getproc(tid)){
			Usiginfo si;

			memset(&si, 0, sizeof(si));
			si.signo = sig;
			si.code = SI_TKILL;
			si.kill.pid = current->tid;
			si.kill.uid = current->uid;
			err = killproc(p, &si, 0);
		}
		qunlock(&proctab);
	}
	return err;
}

int sys_tgkill(int pid, int tid, int sig)
{
	int err;

	trace("sys_tgkill(%d, %d, %S)", pid, tid, sig);

	err = -EINVAL;
	if(tid > 0){
		Uproc *p;

		err = -ESRCH;
		qlock(&proctab);
		if((p = getproc(tid)) && (p->pid == pid)){
			Usiginfo si;

			memset(&si, 0, sizeof(si));
			si.signo = sig;
			si.code = SI_TKILL;
			si.kill.pid = current->tid;
			si.kill.uid = current->uid;
			err = killproc(p, &si, 0);
		}
		qunlock(&proctab);
	}
	return err;
}

int sys_rt_sigqueueinfo(int pid, int sig, void *info)
{
	int err;
	Uproc *p;
	Usiginfo si;

	trace("sys_rt_sigqueueinfo(%d, %S, %p)", pid, sig, info);

	err = -ESRCH;
	qlock(&proctab);
	if(p = getproc(pid)){
		memset(&si, 0, sizeof(si));
		linux2siginfo(info, &si);
		si.signo = sig;
		si.code = SI_QUEUE;
		err = killproc(p, &si, 1);
	}
	qunlock(&proctab);
	return err;
}

enum {
	PIDMAPBITS1	= 8*sizeof(ulong),
};

int sys_kill(int pid, int sig)
{
	int i, j, n;
	Uproc *p;
	Usiginfo si;
	ulong pidmap[(MAXPROC + PIDMAPBITS1-1) / PIDMAPBITS1];
	ulong m;

	trace("sys_kill(%d, %S)", pid, sig);

	n = 0;
	memset(pidmap, 0, sizeof(pidmap));
	qlock(&proctab);
	for(i=0; i<MAXPROC; i++){
		if((p = getprocn(i)) == nil)
			continue;
		if(p->wstate & WEXITED)
			continue;
		if(p->kpid <= 0)
			continue;

		if(pid == 0){
			if(p->pgid != current->pgid)
				continue;
		} else if(pid == -1){
			if(p->pid <= 1)
				continue;
			if(p->tid == current->tid)
				continue;
		} else if(pid < -1) {
			if(p->pgid != -pid)
				continue;
		} else {
			if(p->pid != pid)
				continue;
		}

		/* make sure we send only one signal per pid */
		j = pidhash(p->pid);
		m = 1 << (j % PIDMAPBITS1);
		j /= PIDMAPBITS1;
		if(pidmap[j] & m)
			continue;
		pidmap[j] |= m;

		if(sig > 0){
			memset(&si, 0, sizeof(si));
			si.signo = sig;
			si.code = SI_USER;
			si.kill.pid = current->tid;
			si.kill.uid = current->uid;
			killproc(p, &si, 1);
		}
		n++;
	}
	qunlock(&proctab);
	if(n == 0)
		return -ESRCH;
	return 0;
}

int sys_set_tid_address(int *tidptr)
{
	trace("sys_set_tid_address(%p)", tidptr);

	current->cleartidptr = tidptr;
	return current->pid;
}

struct linux_sched_param
{
	int sched_priority;
};

int sys_sched_setscheduler(int pid, int policy, void *param)
{
	trace("sys_sched_setscheduler(%d, %d, %p)", pid, policy, param);

	if(getproc(pid) == nil)
		return -ESRCH;
	return 0;
}

int sys_sched_getscheduler(int pid)
{
	trace("sys_sched_getscheduler(%d)", pid);

	if(getproc(pid) == nil)
		return -ESRCH;
	return 0;
}

int sys_sched_setparam(int pid, void *param)
{
	trace("sys_sched_setparam(%d, %p)", pid, param);

	if(getproc(pid) == nil)
		return -ESRCH;
	return 0;
}

int sys_sched_getparam(int pid, void *param)
{
	struct linux_sched_param *p = param;

	trace("sys_sched_getparam(%d, %p)", pid, param);

	if(getproc(pid) == nil)
		return -ESRCH;
	if(p == nil)
		return -EINVAL;
	p->sched_priority = 0;

	return 0;
}

int sys_sched_yield(void)
{
	trace("sys_sched_yield()");

	sleep(0);
	return 0;
}

enum {
	RLIMIT_CPU,
	RLIMIT_FSIZE,
	RLIMIT_DATA,
	RLIMIT_STACK,
	RLIMIT_CORE,
	RLIMIT_RSS,
	RLIMIT_NPROC,
	RLIMIT_NOFILE,
	RLIMIT_MEMLOCK,
	RLIMIT_AS,
	RLIMIT_LOCKS,
	RLIMIT_SIGPENDING,
	RLIMIT_MSGQUEUE,

	RLIM_NLIMITS,

	RLIM_INFINITY		= ~0UL,
};

struct linux_rlimit
{
	ulong	rlim_cur;
	ulong	rlim_max;
};

int sys_getrlimit(long resource, void *rlim)
{
	struct linux_rlimit *r = rlim;

	trace("sys_getrlimit(%ld, %p)", resource, rlim);

	if(resource >= RLIM_NLIMITS)
		return -EINVAL;
	if(rlim == nil)
		return -EFAULT;

	r->rlim_cur = RLIM_INFINITY;
	r->rlim_max = RLIM_INFINITY;

	switch(resource){
	case RLIMIT_STACK:
		r->rlim_cur = USTACK;
		r->rlim_max = USTACK;
		break;
	case RLIMIT_CORE:
		r->rlim_cur = 0;
		break;
	case RLIMIT_NPROC:
		r->rlim_cur = MAXPROC;
		r->rlim_max = MAXPROC;
		break;
	case RLIMIT_NOFILE:
		r->rlim_cur = MAXFD;
		r->rlim_max = MAXFD;
		break;
	}
	return 0;
}

int sys_setrlimit(long resource, void *rlim)
{
	trace("sys_setrlimit(%ld, %p)", resource, rlim);

	if(resource >= RLIM_NLIMITS)
		return -EINVAL;
	if(rlim == nil)
		return -EFAULT;

	return -EPERM;
}

