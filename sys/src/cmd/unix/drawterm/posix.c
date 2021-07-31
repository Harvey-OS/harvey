/*
 * Posix generic OS implementation for drawterm.
 */

#include	<pthread.h>
#include	<time.h>
#include	<signal.h>
#include 	<pwd.h>
#include	<errno.h>
#include	<sys/time.h>

#include "lib9.h"
#include "sys.h"
#include "error.h"
typedef struct Ttbl	Ttbl;
typedef struct Thread	Thread;

static pthread_key_t prdakey;

static Thread* tcurthread(void);
#define CT tcurthread()

enum {
	Nthread = 100,
	Nerrlbl = 30,
	KSTACK = 32*1024
};

struct Thread {
	int	tss;
	int	tindex;

	Thread	*qnext;		/* for qlock */

	int	nerrlbl;
	Jump	errlbl[Nerrlbl];

	char	error[ERRLEN];

	void	(*f)(void*);
	void	*a;
	void	*stk;

	int sema[2];
	
	/* for sleep/wakeup/intr */
	Lock	rlock;
	Rendez	*r;
	int	intr;

	pthread_t pthread;
};

struct Ttbl {
	Lock	lk;
	int	nthread;
	int	nalloc;
	Thread	*t[Nthread];
	Thread	*free;
};


static ulong gettss(void);
static Ttbl	tt;

static Thread	*threadalloc(void);
static void	threadfree(Thread *t);
static void* tramp(void *p);
static void	threadsleep(void);
static void	threadwakeup(Thread *t);

/*
 * Not using semaphores, because the Linux kernel is broken.
 * This is easier and fast enough.  See comment near threadsleep.
 */
static void
setsema(Thread *t)
{
	char c;
	if(pipe(t->sema) < 0)
		fatal("pipe failed: %r");

	c = 'L';
}

void
handler(int s)
{
	_exits("sigkill");
}

void
loophandler(int s)
{
fprintf(stderr, "signal %d to %d; looping\n", s, getpid());

	for(;;)
		sched_yield();
}

void
threadinit(void)
{
	Thread *t;

	setpgid(getpid(), getpgrp());
	signal(SIGQUIT, handler);
	signal(SIGTERM, handler);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGSEGV, loophandler);

	if(pthread_key_create(&prdakey, 0))
		fatal("keycreate failed");

	t = threadalloc();
	assert(t != 0);
	if(pthread_setspecific(prdakey, t))
		fatal("cannot set CT");
	setsema(t);
}

int
thread(char *name, void (*f)(void *), void *a)
{
	Thread *t;
	
	if((t = threadalloc()) == 0)
		return -1;

	t->f = f;
	t->a = a;

	if(pthread_create(&t->pthread, nil, tramp, t)){
		oserror();
		fatal("thr_create failed: %d %d %r", errno, thread);
	}
	sched_yield();
	return t->tindex;
}

void
threadexit(void)
{
	if(tt.nthread == 1)
		exits("normal");

	threadfree(CT);

	pthread_setspecific(prdakey, nil);
	pthread_exit(0);
}

static void*
tramp(void *p)
{
	Thread *t;
	
	t = (Thread*)p;
	if(pthread_setspecific(prdakey, t))
		fatal("cannot set CT in tramp");
	setsema(t);
 	(*t->f)(t->a);
	threadexit();
}

int
errstr(char *s)
{
	char tmp[ERRLEN];
	Thread *t;

	t = CT;
	if(t == nil){
		strcpy(s, "unknown error");
		return 0;
	}
	strncpy(tmp, s, ERRLEN);
	strncpy(s, t->error, ERRLEN);
	strncpy(t->error, tmp, ERRLEN);

	return 0;
}

char*
threaderr(void)
{
	return CT->error;
}

/*
 * Threadsleep, threadwakeup depend on the fact
 * that there is one threadwakeup for each threadsleep.
 * It is allowed to come before, but there must be
 * only one.
 *
 * I hope this is okay.  -rsc
 */
/*
 * I was using semaphores before, but it seems to tickle
 * a semaphore leak in the Linux kernel (2.2.5) and eventually you have
 * to reboot because you're out of them, even once all
 * the drawterm processes have been killed off. 
 *
 * So I use pipes now, which might be a bit slower, but don't leak.
 */
static void
threadsleep(void)
{
	char c;
	Thread *t;
	
	t = CT;
	if(read(t->sema[0], &c, 1) != 1){
		oserror();
		fatal("threadsleep fails: %r");
	}
}

static void
threadwakeup(Thread *t)
{
	char c;
	c = 'L';
	if(write(t->sema[1], &c, 1) != 1){
		oserror();
		fatal("threadwakeup fails: %r");
	}
}

static Thread *
threadalloc(void)
{
	Thread *t;

	lock(&tt.lk);
	tt.nthread++;
	if((t = tt.free) != 0) {
		tt.free = t->qnext;
		unlock(&tt.lk);
		return t;
	}

	if(tt.nalloc == Nthread) {
		unlock(&tt.lk);
		return 0;
	}
	t = tt.t[tt.nalloc] = mallocz(sizeof(Thread));
	t->tindex = tt.nalloc;
	t->stk = mallocz(KSTACK);
	tt.nalloc++;
	unlock(&tt.lk);
	return t;
}

static void
threadfree(Thread *t)
{
	lock(&tt.lk);
	tt.nthread--;
	t->qnext = tt.free;
	tt.free = t;
	unlock(&tt.lk);
}

/*
 * Despite having a man page for nanosleep, the linker
 * can't find it, so we'll use select instead. 
 */
static void
msleep(int milli)
{
	fd_set foo;
	struct timeval tv;

	FD_ZERO(&foo);
	tv.tv_sec = milli / 1000;
	tv.tv_usec = (milli % 1000) * 1000; /* micro */
	if(select(0,NULL,NULL,NULL,&tv) < 0)
		perror("select");
}


long
p9sleep(long milli)
{
/*	struct timespec time; */

	if(milli == 0){
		sched_yield();	/* because nanosleep just returns */
		return 0;
	}

	msleep(milli);
/*
	time.tv_sec = milli/1000;
	time.tv_nsec = (milli%1000)*1000000;
	nanosleep(&time, nil);
*/
	return 0;
}

void
_exits(char *s)
{
	if(s && *s)
		fprintf(stderr, "exits: %s\n", s);

	exit(0);
}

void
nexterror(void)
{
	int n;
	Thread *t;

	t = CT;
	
	n = --t->nerrlbl;
	if(n < 0)
		fatal("error: %r");
	longjmp(t->errlbl[n].buf, 1);
}

void
error(char *fmt, ...)
{
	char buf[ERRLEN];
	va_list va;

	if(fmt) {
		va_start(va, fmt);
		doprint(buf, buf+sizeof(buf), (char*)fmt, va);
		va_end(va);
		errstr(buf);
	}
	nexterror();
}

void
poperror(void)
{
	Thread *t;

	t = CT;
	if(t->nerrlbl <= 0)
		fatal("error stack underflow");
	t->nerrlbl--;
}

Jump*
pm_waserror(void)
{
	Thread *t;
	int n;

	t = CT;
	n = t->nerrlbl++;
	if(n >= Nerrlbl)
		fatal("error stack underflow");
	return &t->errlbl[n];
}

void
oserror(void)
{
	char *p;
	char buf[ERRLEN];
	
	if((p = strerror(errno)))
		strncpy(buf, p, ERRLEN);
	else
		sprint(buf, "unix error %d", errno);
	errstr(buf);
	setbuf(stderr, 0);
	fprintf(stderr, "error: %s\n", threaderr());
}

static void
queue(Thread **first, Thread **last)
{
	Thread *t;
	Thread *ct;

	ct = CT;

	t = *last;
	if(t == 0)
		*first = ct;
	else
		t->qnext = ct;
	*last = ct;
	ct->qnext = 0;
}

static Thread *
dequeue(Thread **first, Thread **last)
{
	Thread *t;

	t = *first;
	if(t == 0)
		return 0;
	*first = t->qnext;
	if(*first == 0)
		*last = 0;
	return t;
}

void
qlock(Qlock *q)
{
	lock(&q->lk);

	if(q->hold == 0) {
		q->hold = CT;
		unlock(&q->lk);
		return;
	}

	/*
	 * Can't assert this because of RWLock
	assert(q->hold != CT);
	 */		

	queue((Thread**)&q->first, (Thread**)&q->last);
	unlock(&q->lk);
	threadsleep();
}

int
canqlock(Qlock *q)
{
	lock(&q->lk);
	if(q->hold == 0) {
		q->hold = CT;
		unlock(&q->lk);
		return 1;
	}
	unlock(&q->lk);
	return 0;
}

void
qunlock(Qlock *q)
{
	Thread *t;

	lock(&q->lk);
	/* 
	 * Can't assert this because of RWlock
	assert(q->hold == CT);
	 */
	t = dequeue((Thread**)&q->first, (Thread**)&q->last);
	if(t) {
		q->hold = t;
		unlock(&q->lk);
		threadwakeup(t);
	} else {
		q->hold = 0;
		unlock(&q->lk);
	}
}

int
holdqlock(Qlock *q)
{
	return q->hold == CT;
}

void
rendsleep(Rendez *r, int (*f)(void*), void *arg)
{
	lock(&CT->rlock);
	CT->r = r;
	unlock(&CT->rlock);

	lock(&r->l);

	/*
	 * if condition happened, never mind
	 */
	if(CT->intr || f(arg)){
		unlock(&r->l);
		goto Done;
	}

	/*
	 * now we are committed to
	 * change state and call scheduler
	 */
	if(r->t)
		fatal("double sleep");
	r->t = CT;
	unlock(&r->l);

	threadsleep();

Done:
	lock(&CT->rlock);
	CT->r = 0;
	if(CT->intr) {
		CT->intr = 0;
		unlock(&CT->rlock);
		error(Eintr);
	}
	unlock(&CT->rlock);
}

void
rendwakeup(Rendez *r)
{
	Thread *t;

	lock(&r->l);
	t = r->t;
	if(t) {
		r->t = 0;
		threadwakeup(t);
	}
	unlock(&r->l);
}

void
intr(void *v)
{
	Thread *t;

	t = v;
	lock(&t->rlock);
	t->intr = 1;
	if(t->r)
		rendwakeup(t->r);
	unlock(&t->rlock);
}

void
clearintr(void)
{
	lock(&CT->rlock);
	CT->intr = 0;
	unlock(&CT->rlock);
}

int
ticks(void)
{
	static long sec0 = 0, usec0;
	struct timeval t;

	if(gettimeofday(&t, nil) < 0)
		return 0;
	if(sec0 == 0){
		sec0 = t.tv_sec;
		usec0 = t.tv_usec;
	}
	return (t.tv_sec-sec0)*1000+(t.tv_usec-usec0+500)/1000;
}

void
abort(void)
{
fprintf(stderr, "abort %d\n", getpid());
for(;;) sleep(1);
	*((ulong*)0)=0;
}

Thread*
tcurthread(void)
{
	void *v;

	if((v = pthread_getspecific(prdakey)) == nil)
		fatal("cannot getspecific");
	return v;
}

void*
curthread(void)
{
	return tcurthread();
}

void
osfillproc(Proc *up)
{
	up->uid = getuid();
	up->gid = getgid();
}

extern int tas(void *);
