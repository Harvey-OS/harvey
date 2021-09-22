#include <u.h>
#include <libc.h>
#include <thread.h>
#include <worker.h>

#define MS (1000000LL)
#define S (1000000000LL)

typedef struct Timer Timer;
struct Timer
{
	vlong	when;
	Channel	*c;
	void	(*f)(Worker*, void* arg);
	void	*arg;
	Timer	*next;
};

static QLock	tlock;
static QLock	waitlock;
static int	pid;
static Timer*	timers;

enum {
	Debug = 0,
};

#define Dprint if(!Debug){}else fprint
/* todo add shutdown handling */

static int
timerhandler(void*, char *s)
{
	return strcmp(s, "wakeup") == 0 || strcmp(s, "alarm") == 0;
}

static long sleepytime;
static vlong dt, now;

static void
timerproc(void*)
{
	Timer *t;

	threadsetname("timerproc");
	pid = getpid();
	threadnotify(timerhandler, 1);
	qunlock(&waitlock);
	Dprint(2, "timerproc started\n");
	for(;;){
		qlock(&tlock);
		now = nsec();
		while((t = timers) && now - t->when > 0ll){
			Dprint(2, "timerproc: dispatch %p(%p)\n", t->f, t->arg);
			timers = t->next;
			if(t->c){
				assert(t->f == nil);
				if(nbsendul(t->c, 1) == 0)
					print("timerproc\n");
			}else if(t->f)
				workerdispatch(t->f, t->arg);
			else
				sysfatal("timerproc: Must have chan or func");
			free(t);
		}
		if(timers == nil)
			dt = S;
		else {
			dt = timers->when - now;
			if(dt < MS)
				dt = MS;
		}
		qunlock(&tlock);
		Dprint(2, "timerproc: sleep(%lld)\n", dt/MS);
		sleepytime = dt/MS;
		if(sleepytime < 0 || sleepytime > 10000){
			print("sleepytime %ld\n", sleepytime);
			sleepytime = 10000;
		}
		if(sleep(sleepytime) < 0)
			Dprint(2, "timerproc: interrupted\n");
		else
			Dprint(2, "timerproc: awake\n");
	}
}

static void
dispatch(Timer *t)
{
	Timer **tt;
	static int initialized;
	int mustwake;

	qlock(&tlock);
	Dprint(2, "dispatch: begin\n");
	for(tt = &timers; *tt; tt = &(*tt)->next)
		if((*tt)->when - t->when > 0){
			Dprint(2, "dispatch: queueing\n");
			/* New timer, insert here */
			t->next = *tt;
			break;
		}
	*tt = t;
	mustwake = tt == &timers;
	if(Debug){
		Dprint(2, "dispatch: queue:");
		for(t = timers; t; t = t->next)
			Dprint(2, " %p", t->arg);
		Dprint(2, "\n");
	}
	if(!initialized){
		qlock(&waitlock);	/* qunlock in timerproc */
		Dprint(2, "dispatch: initializing\n");
		proccreate(timerproc, nil, 8192);
		initialized = 1;
	}
	qunlock(&tlock);
	if(mustwake){
		qlock(&waitlock);
		qunlock(&waitlock);
		Dprint(2, "dispatch: mustwake %d\n", pid);
		postnote(PNPROC, pid, "wakeup");
	}
	Dprint(2, "dispatch: end\n");
}

static int
cancel(Timer *t)
{
	Timer **tt;
	int yes;

	yes = 0;
	qlock(&tlock);
	for(tt = &timers; *tt != nil; tt = &(*tt)->next)
		if(*tt == t){
			Dprint(2, "timercancel: deleting %p(%p), %p\n",
				t->f, t->arg, t->c);
			*tt = t->next;
			free(t);
			yes = 1;
			break;
		}
	if(Debug){
		Dprint(2, "timerdispatch: queue:");
		for(t = timers; t; t = t->next)
			Dprint(2, " %p", t->arg);
		Dprint(2, "\n");
	}
	qunlock(&tlock);
	return yes;
}

void*
timerdispatch(void (*f)(Worker*, void*), void *arg, vlong when)
{
	Timer *t;

	t = mallocz(sizeof(Timer), 1);
	Dprint(2, "timerdispatch: %p(%p)\n", f, arg);
	t->f = f;
	t->arg = arg;
	t->when = when;
	dispatch(t);
	return t;
}

int
timerrecall(void *t)
{
	return cancel(t);
}

static int
runtop(int op, Channel *c, void *v, vlong when)
{
	int r, d;
	Alt a[3];
	Timer *t;
	Channel *tc;
	static int arg;

	t = mallocz(sizeof(Timer), 1);
	tc = chancreate(sizeof(int), 1);
	t->c = tc;
	t->arg = (void*)++arg;	/* guard against cancel/reuse races */
	t->when = when;
	dispatch(t);
	a[0].op = CHANRCV;
	a[0].c = tc;
	a[0].v = &d;
	a[1].op = op;
	a[1].c = c;
	a[1].v = v;
	a[2].op = CHANEND;
	switch(r=alt(a)){
	default:
		abort();
	case -1:	/* interrupted */
		fprint(2, "runt interrupted\n");
	case 1:	/* normal receive, cancel timer */
		if(!cancel(t))	/* couldn't cancel: wait. */
			if(nbrecvul(tc) == 0)
				print("runtop\n");
	case 0:	/* timer went off */
		chanfree(tc);
	}
	return r;
}

int
recvt(Channel *c, void *v, vlong when)
{
	return runtop(CHANRCV, c, v, when);
}

int
sendt(Channel *c, void *v, vlong when)
{
	return runtop(CHANSND, c, v, when);
}
