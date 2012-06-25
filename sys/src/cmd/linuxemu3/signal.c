#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

typedef struct Signal Signal;
typedef struct Action Action;
typedef struct Queue Queue;
typedef struct Timers Timers;
typedef struct Handlers Handlers;
typedef struct Private Private;

struct Signal
{
	Usiginfo;
	Signal	*next;
};

struct Action
{
	void	*handler;
	int		flags;
	uvlong	block;
};

struct Queue
{
	Ref;
	QLock;

	Signal	*head;
	Signal	**tailp;
	Signal	*free;
	Signal	a[64];

	Ufile		*tty;
};

struct Timers
{
	Ref;
	struct {
		vlong	interval;
		vlong	expire;
	}		itimer[2];
};

struct Handlers
{
	Ref;
	QLock;
	Action	a[SIGMAX-1];
};

struct Private
{
	Handlers   	*h;
	Queue		*q;
	Timers		*t;

	struct {
		ulong	sp;
		ulong	size;
	}			altstack;
	
	uvlong		block;

	Urestart		*freerestart;
};

enum
{
	SIG_ERR		= -1,
	SIG_DFL		= 0,
	SIG_IGN		= 1,
	SIG_HOLD	= 2,
};

enum
{
	SA_NOCLDSTOP	= 1,
	SA_NOCLDWAIT	= 2,
	SA_SIGINFO		= 4,
	SA_ONSTACK		= 0x08000000,
	SA_RESTART		= 0x10000000,
	SA_NODEFER		= 0x40000000,
	SA_RESETHAND	= 0x80000000,
};

enum
{
	SS_ONSTACK		= 1,
	SS_DISABLE		= 2,
};

#define MASK(sig)	(1LL << ((sig)-1))

static void
nextsignal(uvlong rblock, int wait);

static int
getsignal(Private *p, Usiginfo *pinfo, int wait);

static void
initrestart(Uproc *proc)
{
	Urestart *r;

	r = &proc->restart0;
	r->syscall = nil;
	r->link = nil;
	proc->restart = r;
}

static void
poprestart(Private *p)
{
	Urestart *r;

	for(;;){
		r = current->restart;
		if(r->link==nil || r->syscall)
			break;
		current->restart = r->link;

		r->link = p->freerestart;
		p->freerestart = r;
	}
	if(r->syscall)
		current->syscall = r->syscall;
}

static Queue*
mkqueue(void)
{
	Queue *q;
	int i;

	q = kmallocz(sizeof(Queue), 1);
	q->ref = 1;
	q->head = nil;
	q->tailp = &q->head;
	for(i=0; i<nelem(q->a); i++)
		q->a[i].next = (i+1 == nelem(q->a)) ? nil : &q->a[i+1];
	q->free = q->a;

	return q;
}

static Handlers*
mkhandlers(void)
{
	Handlers *h;
	int i;

	h = kmallocz(sizeof(Handlers), 1);
	h->ref = 1;
	for(i=1; i<SIGMAX; i++)
		h->a[i-1].handler = (void*)SIG_DFL;
	return h;
}

static Timers*
mktimers(void)
{
	Timers *t;

	t = kmallocz(sizeof(Timers), 1);
	t->ref = 1;
	return t;
}

/* bits.s */
extern int get_ds(void);
extern int get_cs(void);
static ulong user_cs, user_ds;

void initsignal(void)
{
	Private *p;

	if(user_ds==0 && user_cs==0){
		user_ds = get_ds();
		user_cs = get_cs();
	}

	p = kmallocz(sizeof(*p), 1);
	p->block = 0;

	p->q = mkqueue();
	p->h = mkhandlers();
	p->t = mktimers();

	current->signal = p;
	initrestart(current);
}

void exitsignal(void)
{
	Private *p;
	Queue *q;
	Timers *t;
	Signal **i;
	Handlers *h;
	Urestart *r;

	if((p = current->signal) == nil)
		return;
	current->signal = nil;
	q = p->q;
	qlock(q);
again:
	for(i=&q->head; *i; i=&((*i)->next)){
		Signal *r;
		r = *i;
		if(!r->group && (r->topid == current->tid)){
			if((*i = r->next) == nil)
				q->tailp = i;
			r->next = q->free;
			q->free = r;
			goto again;
		}
	}
	qunlock(q);
	if(!decref(q)){
		putfile(q->tty);
		q->tty = nil;
		free(q);
	}
	h =  p->h;
	if(!decref(h))
		free(h);
	t = p->t;
	if(!decref(t))
		free(t);
	while(r = current->restart){
		if(r->link == nil)
			break;
		current->restart = r->link;
		r->link = p->freerestart;
		p->freerestart = r;
	}
	current->restart = nil;
	while(r = p->freerestart){
		p->freerestart = r->link;
		free(r);
	}
	free(p);
}

void clonesignal(Uproc *new, int copyhand, int newproc)
{
	Private *p, *n;

	if((p = current->signal) == nil)
		return;

	n = kmallocz(sizeof(*n), 1);
	if(copyhand){
		n->h = mkhandlers();

		qlock(p->h);
		memmove(n->h->a, p->h->a, sizeof(n->h->a));
		qunlock(p->h);
	} else {
		incref(p->h);
		n->h = p->h;
	}

	qlock(p->q);
	if(newproc){
		n->q = mkqueue();
		n->q->tty = getfile(p->q->tty);
		n->t = mktimers();
		n->altstack = p->altstack;
	} else {
		incref(p->q);
		n->q = p->q;
		incref(p->t);
		n->t = p->t;
	}
	qunlock(p->q);

	n->block = p->block;
	new->signal = n;

	initrestart(new);
}

void
settty(Ufile *tty)
{
	Private *p;
	Ufile *old;

	if((p = current->signal) == nil)
		return;
	tty = getfile(tty);
	qlock(p->q);
	old = p->q->tty;
	p->q->tty = tty;
	qunlock(p->q);
	putfile(old);
}

Ufile*
gettty(void)
{
	Private *p;
	Ufile *tty;

	if((p = current->signal) == nil)
		return nil;
	qlock(p->q);
	tty = getfile(p->q->tty);
	qunlock(p->q);
	return tty;
}

int ignoressignal(Uproc *proc, int sig)
{
	Private *p;
	int a, f;

	if((p = proc->signal) == nil)
		return 1;
	qlock(p->h);
	a = (int)p->h->a[sig-1].handler;
	f = p->h->a[sig-1].flags;
	qunlock(p->h);
	switch(sig){
	case SIGKILL:
	case SIGSTOP:
		return 0;
	case SIGCHLD:
		if(f & SA_NOCLDWAIT)
			return 1;
		break;
	case SIGWINCH:
	case SIGURG:
		if(a == SIG_DFL)
			return 1;
	}
	return (a == SIG_IGN);
}

int wantssignal(Uproc *proc, int sig)
{
	Private *p;

	p = proc->signal;
	if(p == nil || p->block & MASK(sig))
		return 0;
	return !ignoressignal(proc, sig);
}

int sendsignal(Uproc *proc, Usiginfo *info, int group)
{
	Private *p;
	Signal *s;

	trace("sendsignal(%S) to %d from %d",
		info->signo, proc->tid, (current != nil) ? current->tid : 0);

	if(ignoressignal(proc, info->signo)){
		trace("sendsignal(): ignored signal %S", info->signo);
		return  0;
	}

	p = proc->signal;
	qlock(p->q);
	if(info->signo < SIGRT1){
		for(s=p->q->head; s; s=s->next){
			if(!s->group && (s->topid != proc->tid))
				continue;
			if(s->signo == info->signo){
				qunlock(p->q);
				trace("sendsignal(): droping follow up signal %S", info->signo);
				return 0;
			}
		}
	}
	if((s = p->q->free) == nil){
		qunlock(p->q);
		trace("sendsignal(): out of signal buffers");
		return -EAGAIN;
	}
	p->q->free = s->next;
	s->next = nil;
	memmove(s, info, sizeof(*info));
	s->group = group;
	s->topid = group ? proc->pid : proc->tid;
	*p->q->tailp = s;
	p->q->tailp = &s->next;
	qunlock(p->q);
	return 1;
}

int
signalspending(Uproc *proc)
{
	Private *p;
	Signal *s;
	int ret;

	p = proc->signal;
	if(p == nil || p->q->head == nil)
		return 0;

	ret = 0;
	qlock(p->q);
	for(s=p->q->head; s; s=s->next){
		if(!s->group && (s->topid != current->tid))
			continue;
		if(MASK(s->signo) & p->block)
			continue;
		ret = 1;
		break;
	}
	qunlock(p->q);

	return ret;
}

static int
getsignal(Private *p, Usiginfo *pinfo, int wait)
{
	Signal *r;
	Signal **i;
	int sig;

	if(!wait && p->q->head == nil)
		return 0;

	sig = 0;
	qlock(p->q);
	for(;;){
		for(i=&p->q->head; *i; i=&((*i)->next)){
			r = *i;

			if(!r->group && (r->topid != current->tid))
				continue;

			if(p->block & MASK(r->signo)){
				if(sig == 0)
					sig = -r->signo;
				continue;
			}
			sig = r->signo;

			/* dequeue nonblocked signal */
			memmove(pinfo, r, sizeof(*pinfo));
			if((*i = r->next) == nil)
				p->q->tailp = i;
			r->next = p->q->free;
			p->q->free = r;
			break;
		}
		if(wait && sig <= 0){
			if(sleepproc(p->q, 0) == 0)
				continue;
		}
		break;
	}
	qunlock(p->q);

	return sig;
}

static uvlong
sigset2uvlong(uchar *set, int setsize)
{
	uvlong r;
	int i;

	r = 0;
	if(setsize > sizeof(uvlong))
		setsize = sizeof(uvlong);
	for(i=0; i<setsize; i++)
		r |= (uvlong)set[i] << (i * 8);
	return r;
}

static void
uvlong2sigset(uchar *set, int setsize, uvlong mask)
{
	int i;

	for(i=0; i<setsize; i++){
		if(i < sizeof(uvlong)){
			set[i] = ((mask >> (i*8)) & 0xff);
		} else {
			set[i] = 0;
		}
	}
}

struct linux_siginfo {
	int	signo;
	int	errno;
	int	code;

	union {
		int _pad[29];

		/* kill() */
		struct {
			int	pid;			/* sender's pid */
			int	uid;			/* sender's uid */
		} kill;

		/* POSIX.1b timers */
		struct {
			int	tid;			/* timer id */
			int	overrun;		/* overrun count */
			int	val;			/* same as below */
		} timer;

		/* POSIX.1b signals */
		struct {
			int	pid;			/* sender's pid */
			int	uid;			/* sender's uid */
			int	val;
		} rt;

		/* SIGCHLD */
		struct {
			int	pid;			/* which child */
			int	uid;			/* sender's uid */
			int	status;			/* exit code */
			long	utime;
			long	stime;
		} chld;

		/* SIGILL, SIGFPE, SIGSEGV, SIGBUS */
		struct {
			void	*addr;	/* faulting insn/memory ref. */
			int		trapno;	/* TRAP # which caused the signal */
		} fault;

		/* SIGPOLL */
		struct {
			long	band;	/* POLL_IN, POLL_OUT, POLL_MSG */
			int	fd;
		} poll;
	};
};

void
siginfo2linux(Usiginfo *info, void *p)
{
	struct linux_siginfo *li = p;
	int sig;

	sig = info->signo;

	li->signo = sig;
	li->errno = info->errno;
	li->code = info->code;

	switch(sig){
	case SIGALRM:
		li->timer.tid = info->timer.tid;
		li->timer.overrun = info->timer.overrun;
		li->timer.val = info->timer.val;
		break;
	case SIGCHLD:
		li->chld.pid = info->chld.pid;
		li->chld.uid = info->chld.uid;
		li->chld.status = info->chld.status;
		li->chld.utime = info->chld.utime;
		li->chld.stime = info->chld.stime;
		break;
	case SIGILL:
	case SIGBUS:
	case SIGFPE:
	case SIGSEGV:
		li->fault.addr = info->fault.addr;
		li->fault.trapno = info->fault.trapno;
		break;
	case SIGPOLL:
		li->poll.fd = info->poll.fd;
		li->poll.band = info->poll.band;
		break;
	case SIGRT1:
	case SIGRT2:
	case SIGRT3:
	case SIGRT4:
	case SIGRT5:
	case SIGRT6:
	case SIGRT7:
	case SIGRT8:
		li->rt.pid = info->rt.pid;
		li->rt.uid = info->rt.uid;
		li->rt.val = info->rt.val;
		break;
	default:
		li->kill.pid = info->kill.pid;
		li->kill.uid = info->kill.uid;
	}
}

void
linux2siginfo(void *p, Usiginfo *info)
{
	struct linux_siginfo *li = p;
	int sig;

	sig = li->signo;

	info->signo = sig;
	info->errno = li->errno;
	info->code = li->code;

	switch(sig){
	case SIGALRM:
		info->timer.tid = li->timer.tid;
		info->timer.overrun = li->timer.overrun;
		info->timer.val = li->timer.val;
		break;
	case SIGCHLD:
		info->chld.pid = li->chld.pid;
		info->chld.uid = li->chld.uid;
		info->chld.status = li->chld.status;
		info->chld.utime = li->chld.utime;
		info->chld.stime = li->chld.stime;
		break;
	case SIGILL:
	case SIGBUS:
	case SIGFPE:
	case SIGSEGV:
		info->fault.addr = li->fault.addr;
		info->fault.trapno = li->fault.trapno;
		break;
	case SIGPOLL:
		info->poll.fd = li->poll.fd;
		info->poll.band = li->poll.band;
		break;
	case SIGRT1:
	case SIGRT2:
	case SIGRT3:
	case SIGRT4:
	case SIGRT5:
	case SIGRT6:
	case SIGRT7:
	case SIGRT8:
		info->rt.pid = li->rt.pid;
		info->rt.uid = li->rt.uid;
		info->rt.val = li->rt.val;
		break;
	default:
		info->kill.pid = li->kill.pid;
		info->kill.uid = li->kill.uid;
	}
}

struct linux_sigcontext {
	ulong	gs;
	ulong	fs;
	ulong	es;
	ulong	ds;
	ulong	di;
	ulong	si;
	ulong	bp;
	ulong	sp;
	ulong	bx;
	ulong	dx;
	ulong	cx;
	ulong	ax;
	ulong	trapno;
	ulong	err;
	ulong	ip;
	ulong	cs;
	ulong	flags;
	ulong	sp_at_signal;
	ulong	ss;
	void*	fpstate;
	ulong	oldmask;
	ulong	cr2;
};

static void
ureg2linuxsigcontext(Ureg *u, struct linux_sigcontext *sc)
{
	sc->gs = u->gs;
	sc->fs = u->fs;
	sc->es = u->es;
	sc->ds = u->ds;
	sc->di = u->di;
	sc->si = u->si;
	sc->bp = u->bp;
	sc->sp = u->sp;
	sc->bx = u->bx;
	sc->dx = u->dx;
	sc->cx = u->cx;
	sc->ax = u->ax;
	sc->trapno = u->trap;
	sc->err = u->ecode;
	sc->ip = u->pc;
	sc->cs = u->cs;
	sc->flags = u->flags;
	sc->sp_at_signal = u->sp;
	sc->ss = u->ss;
	sc->cr2 = 0;
}

struct linux_sigset {
	ulong	sig[2];
};

struct linux_signalstack {
	ulong	sp;
	int		flags;
	ulong	size;
};

struct linux_ucontext {
	ulong	flags;
	struct linux_ucontext	*link;
	struct linux_signalstack	stack;
	struct linux_sigcontext	context;
	struct linux_sigset	sigmask;
};

static void
linuxsigcontext2ureg(struct linux_sigcontext *sc, Ureg *u)
{
	u->pc = sc->ip;
	u->sp = sc->sp;
	u->ax = sc->ax;
	u->bx = sc->bx;
	u->cx = sc->cx;
	u->dx = sc->dx;
	u->di = sc->di;
	u->si = sc->si;
	u->bp = sc->bp;

	u->cs = sc->cs;
	u->ss = sc->ss;
	u->ds = sc->ds;
	u->es = sc->es;
	u->fs = sc->fs;
	u->gs = sc->gs;
}

struct linux_sigframe {
	void	*ret;
	int		sig;

	union {
		struct linux_sigcontext		sc;

		struct {
			struct linux_siginfo	*pinfo;
			struct linux_ucontext	*puc;

			struct linux_siginfo	info;
			struct linux_ucontext	uc;
		} rt;
	};
};

#pragma profile off

static int
linuxstackflags(Private *p, ulong sp)
{
	if(p->altstack.size == 0 || p->altstack.sp == 0)
		return SS_DISABLE;
	if(sp - p->altstack.sp < p->altstack.size)
		return SS_ONSTACK;
	return 0;
}

static void
linuxsignal(Private *p, Action *a, Usiginfo *i, uvlong rblock)
{
	struct linux_sigframe _frame;
	struct linux_sigframe *f;
	Ureg *u;
	int stackflags;

	u = current->ureg;

	stackflags = linuxstackflags(p, u->sp);
	if((a->flags & SA_ONSTACK) && (stackflags == 0)){
		trace("linuxsignal: altstack %lux %lux", p->altstack.sp, p->altstack.size);
		f = (struct linux_sigframe*)(p->altstack.sp + p->altstack.size);
		f--;
	} else {
		f = &_frame;
	}

	trace("linuxsignal(): frame %p", f);
	memset(f, 0, sizeof(*f));

	f->sig = i->signo;

	if(a->flags & SA_SIGINFO){
		f->ret = linux_rtsigreturn;
		siginfo2linux(i, &f->rt.info);
		f->rt.pinfo = &f->rt.info;

		f->rt.uc.stack.sp = p->altstack.sp;
		f->rt.uc.stack.size = p->altstack.size;
		f->rt.uc.stack.flags = stackflags;

		ureg2linuxsigcontext(u, &f->rt.uc.context);
		f->rt.uc.context.oldmask = rblock & 0xFFFFFFFF;
		f->rt.uc.sigmask.sig[0] = rblock & 0xFFFFFFFF;
		f->rt.uc.sigmask.sig[1] = (rblock >> 32) & 0xFFFFFFFF;
		f->rt.puc = &f->rt.uc;
		u->cx = (ulong)f->rt.puc;
		u->dx = (ulong)f->rt.pinfo;
	} else {
		f->ret = linux_sigreturn;
		ureg2linuxsigcontext(u, &f->sc);
		f->sc.oldmask = rblock & 0xFFFFFFFF;
		u->cx = 0;
		u->dx = 0;
	}

	u->di = 0;
	u->si = 0;
	u->bp = 0;
	u->bx = 0;

	u->ax = (ulong)i->signo;

	u->sp = (ulong)f;
	u->pc = (ulong)a->handler;

	u->cs = user_cs;
	u->ss = user_ds;
	u->ds = user_ds;
	u->es = user_ds;

	p->block |= a->block;

	trace("linuxsignal(): retuser pc=%lux sp=%lux", u->pc, u->sp);
	retuser();
}

int
sys_sigreturn(void)
{
	struct linux_sigframe *f;
	Private *p;
	Ureg *u;

	trace("sys_sigreturn()");

	p = current->signal;
	u = current->ureg;

	f = (struct linux_sigframe*)(u->sp - 4);

	trace("sys_sigreturn(): frame %p", f);

	linuxsigcontext2ureg(&f->sc, u);
	p->block &= ~0xFFFFFFFF;
	p->block |= f->sc.oldmask;
	nextsignal(p->block, 0);
	poprestart(p);

	trace("sys_sigreturn(): retuser pc=%lux sp=%lux", u->pc, u->sp);
	retuser();

	return -1;
}

int
sys_rt_sigreturn(void)
{
	struct linux_sigframe *f;
	Private *p;
	Ureg *u;

	trace("sys_rt_sigreturn()");

	p = current->signal;
	u = current->ureg;

	f = (struct linux_sigframe*)(u->sp - 4);
	trace("sys_rt_sigreturn(): frame %p", f);

	linuxsigcontext2ureg(&f->rt.uc.context, u);
	p->block = (uvlong)f->rt.uc.sigmask.sig[0] | (uvlong)f->rt.uc.sigmask.sig[1]<<32;
	nextsignal(p->block, 0);
	poprestart(p);

	trace("sys_rt_sigreturn(): pc=%lux sp=%lux", u->pc, u->sp);
	retuser();

	return -1;
}

/*
 * nextsignal transfers execution to the next pending
 * signal or just returns. after the signal got executed,
 * the block mask is restored to rblock. if heres no
 * pending signal and wait is non zero the current
 * process is suspended until here is a signal available.
 */

static void
nextsignal(uvlong rblock, int wait)
{
	Private *p;
	int sig;
	Usiginfo info;
	Action a;
	Urestart *r;
	
	for(;;){
		if((p = current->signal) == nil)
			return;

		if(current->wstate & WSTOPPED){
			p->block = ~(MASK(SIGCONT) | MASK(SIGKILL));
			sig = getsignal(p, &info, 1);
			p->block = rblock;
			if(sig <= 0)
				return;
			if(sig == SIGCONT){
				contproc(current, sig, info.group);
				continue;
			}
		} else {
			if((sig = getsignal(p, &info, wait)) <= 0)
				return;
			if(sig == SIGCONT)
				continue;
			if(sig == SIGSTOP){
				stopproc(current, sig, info.group);
				continue;
			}
		}
		break;
	}

	trace("nextsignal(): signal %S", sig);

	qlock(p->h);
	a = p->h->a[sig-1];
	if(a.flags & SA_RESETHAND)
		p->h->a[sig-1].handler = (void*)SIG_DFL;
	if(a.flags & SA_NODEFER == 0)
		a.block |= MASK(sig);
	qunlock(p->h);

	switch((int)a.handler){
	case SIG_DFL:
		switch(sig){
		case SIGCHLD:
		case SIGWINCH:
		case SIGURG:
			goto Ignored;
		}
		/* no break */
	case SIG_ERR:
		trace("nextsignal(): signal %S causes exit", sig);
		exitproc(current, sig, 1);
Ignored:
	case SIG_IGN:
	case SIG_HOLD:
		trace("nextsignal(): signal %S ignored", sig);
		return;
	}

	if(current->restart->syscall){
		if(a.flags & SA_RESTART){
			if(r = p->freerestart)
				p->freerestart = r->link;
			if(r == nil)
				r = kmalloc(sizeof(*r));
			r->syscall = nil;
			r->link = current->restart;
			current->restart = r;
		} else {
			trace("nextsignal(): interrupting syscall %s", current->syscall);
			current->sysret(-EINTR);
		}
	}

	linuxsignal(p, &a, &info, rblock);
}

void handlesignals(void)
{
	Private *p;

	if(p = current->signal)
		nextsignal(p->block, 0);
}

int
sys_rt_sigsuspend(uchar *set, int setsize)
{
	Private *p;
	uvlong b, rblock;

	trace("sys_rt_sigsuspend(%p, %d)", set, setsize);

	p = current->signal;
	b = sigset2uvlong(set, setsize);
	b &= ~(MASK(SIGKILL) | MASK(SIGSTOP));

	rblock = p->block;
	p->block = b;

	/*
	 * if a signal got handled, it will pop out after the the
	 * sigsuspend syscall with return value set to -EINTR
	 */
	current->sysret(-EINTR);

	for(;;)
		nextsignal(rblock, 1);
}

#pragma profile on

struct linux_altstack
{
	ulong	sp;
	int		flags;
	ulong	size;
};

int sys_sigaltstack(void *stk, void *ostk)
{
	Private *p;
	struct linux_altstack *a = stk, *oa = ostk;
	int flags;
	ulong sp, size;

	trace("sys_sigaltstack(%lux, %lux)", (ulong)stk, (ulong)ostk);

	p = current->signal;
	sp = p->altstack.sp;
	size = p->altstack.size;
	flags = linuxstackflags(p, current->ureg->sp);

	if(a){
		if(flags == SS_ONSTACK)
			return -EPERM;

		if(a->flags == SS_DISABLE){
			p->altstack.sp = 0;
			p->altstack.size = 0;
		} else {
			p->altstack.sp = a->sp;
			p->altstack.size = a->size;
		}

		trace("sys_signalstack(): new altstack %lux-%lux",
			p->altstack.sp, p->altstack.sp + p->altstack.size);
	}
	if(oa){
		oa->sp = sp;
		oa->size = size;
		oa->flags = flags;
	}

	return 0;
}

struct linux_sigaction
{
	void *handler;
	ulong flags;
	void *restorer;
	uchar mask[];
};

int sys_rt_sigaction(int sig, void *pact, void *poact, int setsize)
{
	Private *p;
	Action *a;
	struct linux_sigaction *act;
	struct linux_sigaction *oact;
	void *handler;
	int flags;
	uvlong block;

	trace("sys_rt_sigaction(%S, %p, %p, %d)", sig, pact, poact, setsize);

	p = current->signal;
	act = (struct linux_sigaction*)pact;
	oact = (struct linux_sigaction*)poact;

	if((sig < 1) || (sig >= SIGMAX))
		return -EINVAL;

	qlock(p->h);
	a = &p->h->a[sig-1];
	handler = a->handler;
	flags = a->flags;
	block = a->block;
	if(act){
		trace("flags = %x", a->flags);
		a->handler = act->handler;
		a->flags = act->flags;
		a->block = sigset2uvlong(act->mask, setsize);
	}
	if(oact){
		oact->handler = handler;
		oact->flags = flags;
		oact->restorer = 0;
		uvlong2sigset(oact->mask, setsize, block);
	}
	qunlock(p->h);

	return 0;
}

int sys_rt_sigpending(uchar *set, int setsize)
{
	Private *p;
	Signal *s;
	uvlong m;

	trace("sys_rt_sigpending(%p, %d)", set, setsize);

	p = current->signal;
	m = 0LL;
	qlock(p->q);
	for(s=p->q->head; s; s=s->next){
		if(!s->group && (s->topid != current->tid))
			continue;
		m |= MASK(s->signo);
	}
	qunlock(p->q);

	uvlong2sigset(set, setsize, m);
	return 0;
}

enum
{
	SIG_BLOCK	= 0,
	SIG_UNBLOCK	= 1,
	SIG_SETMASK	= 2,
};

int sys_rt_sigprocmask(int how, uchar *act, uchar *oact, int setsize)
{
	Private *p;
	uvlong m, block;

	trace("sys_rt_sigprocmask(%d, %p, %p, %d)", how, act, oact, setsize);

	p = current->signal;
	block = p->block;
	if(act){
		m = sigset2uvlong(act, setsize);
		m &= ~(MASK(SIGKILL) | MASK(SIGSTOP));
		switch(how){
		default:
			return -EINVAL;
		case SIG_BLOCK:
			p->block |= m;
			break;
		case SIG_UNBLOCK:
			p->block &= ~m;
			break;
		case SIG_SETMASK:
			p->block = m;
			break;
		}
	}
	if(oact)
		uvlong2sigset(oact, setsize, block);
	return 0;
}

struct linux_itimer
{
	struct linux_timeval	it_interval;
	struct linux_timeval	it_value;
};

static vlong
hzround(vlong t)
{
	vlong q = 1000000000LL/HZ;
	return (t + q-1) / q;
}

int sys_setitimer(int which, void *value, void *ovalue)
{
	Private *p;
	Timers *t;
	vlong now, rem, delta;
	struct linux_itimer *nv = value, *ov = ovalue;

	trace("sys_setitimer(%d, %p, %p)", which, value, ovalue);

	p = current->signal;
	t = p->t;

	if(which < 0 || which >= nelem(t->itimer))
		return -EINVAL;

	now = nsec();
	delta = t->itimer[which].interval;
	rem = t->itimer[which].expire - now;
	if(rem < 0)
		rem = 0;
	if(nv != nil){
		trace("nv->{interval->{%ld, %ld}, value->{%ld, %ld}}",
			nv->it_interval.tv_sec, nv->it_interval.tv_usec,
			nv->it_value.tv_sec, nv->it_value.tv_usec);
		t->itimer[which].interval = hzround(nv->it_interval.tv_sec*1000000000LL +
			nv->it_interval.tv_usec*1000);
		t->itimer[which].expire = (now + nv->it_value.tv_sec*1000000000LL +
			nv->it_value.tv_usec*1000);
		setalarm(t->itimer[which].expire);
	}

	if(ov != nil){
		ov->it_interval.tv_sec =  delta / 1000000000LL;
		ov->it_interval.tv_usec = (delta % 1000000000LL)/1000;
		ov->it_value.tv_sec = rem / 1000000000LL;
		ov->it_value.tv_usec = (rem % 1000000000LL)/1000;
		trace("ov->{interval->{%ld, %ld}, value->{%ld, %ld}}",
			ov->it_interval.tv_sec, ov->it_interval.tv_usec,
			ov->it_value.tv_sec, ov->it_value.tv_usec);
	}

	return 0;
}

int sys_getitimer(int which, void *value)
{
	Private *p;
	Timers *t;
	vlong rem, delta;
	struct linux_itimer *v = value;

	trace("sys_getitimer(%d, %p)", which, value);

	p = current->signal;
	t = p->t;

	if(value == nil)
		return -EINVAL;
	if(which < 0 || which >= nelem(t->itimer))
		return -EINVAL;

	delta =t->itimer[which].interval;
	rem = t->itimer[which].expire - nsec();

	if(rem < 0)
		rem = 0;
	v->it_interval.tv_sec = delta / 1000000000LL;
	v->it_interval.tv_usec = (delta % 1000000000LL)/1000;
	v->it_value.tv_sec = rem / 1000000000LL;
	v->it_value.tv_usec = (rem % 1000000000LL)/1000;

	return 0;
}

int sys_alarm(long seconds)
{
	Private *p;
	Timers *t;
	vlong old, now;

	trace("sys_alarm(%ld)", seconds);
	p = current->signal;
	t = p->t;
	now = nsec();
	old = t->itimer[0].expire - now;
	if(old < 0)
		old = 0;
	t->itimer[0].interval = 0;
	if(seconds > 0){
		t->itimer[0].expire = now + (vlong)seconds * 1000000000LL;
		setalarm(t->itimer[0].expire);
	} else {
		t->itimer[0].expire = 0;
	}
	return old / 1000000000LL;
}

int
Sfmt(Fmt *f)
{
	static char *t[] = {
	[SIGHUP]	= "SIGHUP",
	[SIGINT]	= "SIGINT",
	[SIGQUIT]	= "SIGQUIT",
	[SIGILL]	= "SIGILL",
	[SIGTRAP]	= "SIGTRAP",
	[SIGABRT]	= "SIGABRT",
	[SIGBUS]	= "SIGBUS",
	[SIGFPE]	= "SIGFPE",
	[SIGKILL]	= "SIGKILL",
	[SIGUSR1]	= "SIGUSR1",
	[SIGSEGV]	= "SIGSEGV",
	[SIGUSR2]	= "SIGUSR2",
	[SIGPIPE]	= "SIGPIPE",
	[SIGALRM]	= "SIGALRM",
	[SIGTERM]	= "SIGTERM",
	[SIGSTKFLT]	= "SIGSTKFLT",
	[SIGCHLD]	= "SIGCHLD",
	[SIGCONT]	= "SIGCONT",
	[SIGSTOP]	= "SIGSTOP",
	[SIGTSTP]	= "SIGTSTP",
	[SIGTTIN]	= "SIGTTIN",
	[SIGTTOU]	= "SIGTTOU",
	[SIGURG]	= "SIGURG",
	[SIGXCPU]	= "SIGXCPU",
	[SIGXFSZ]	= "SIGXFSZ",
	[SIGVTALRM]	= "SIGVTALRM",
	[SIGPROF]	= "SIGPROF",
	[SIGWINCH]	= "SIGWINCH",
	[SIGIO]		= "SIGIO",
	[SIGPWR]	= "SIGPWR",
	[SIGSYS]	= "SIGSYS",
	[SIGRT1]	= "SIGRT1",
	[SIGRT2]	= "SIGRT2",
	[SIGRT3]	= "SIGRT3",
	[SIGRT4]	= "SIGRT4",
	[SIGRT5]	= "SIGRT5",
	[SIGRT6]	= "SIGRT6",
	[SIGRT7]	= "SIGRT7",
	[SIGRT8]	= "SIGRT8",
	};

	int sig;

	sig = va_arg(f->args, int);
	if(sig < 1 || sig >= SIGMAX)
		return fmtprint(f, "%d", sig);
	return fmtprint(f, "%d [%s]", sig, t[sig]);
}

/* proc.c */
extern int procsetalarm(Uproc *proc, vlong t);

void
alarmtimer(Uproc *proc, vlong now)
{
	Private *p;
	Timers *t;
	vlong expire, delta;
	Usiginfo si;
	int i, overrun;

	if((p = proc->signal) == nil)
		return;
	t = p->t;
	for(i=0; i < nelem(t->itimer); i++){
		expire = t->itimer[i].expire;
		if(expire <= 0)
			continue;
		if(now < expire){
			procsetalarm(proc, expire);
			continue;
		}
		overrun = 0;
		delta = (t->itimer[i].interval);
		if(delta > 0){
			expire += delta;
			while(expire <= now){
				expire += delta;
				overrun++;
			}
			procsetalarm(proc, expire);
		} else {
			expire = 0;
		}
		t->itimer[i].expire = expire;

		memset(&si, 0, sizeof(si));
		si.signo = SIGALRM;
		si.code = SI_TIMER;
		si.timer.tid = i;
		si.timer.overrun = overrun;
		killproc(proc, &si, 1);
	}
}
