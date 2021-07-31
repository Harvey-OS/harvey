#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"mem.h"

struct
{
	Lock;
	ulong	pid;
} pidalloc;

struct
{
	Lock;
	User	*arena;
	User	*free;
} procalloc;

struct
{
	Lock;
	User	*head;
	User	*tail;
} runq;

char *statename[] =	/* BUG: generate automatically */
{
	[Dead]		"Dead",
	[Moribund]	"Moribund",
	[Zombie]	"Zombie",
	[Ready]		"Ready",
	[Scheding]	"Scheding",
	[Running]	"Running",
	[Queueing]	"Queueing",
	[Sending]	"Sending",
	[Recving]	"Recving",
	[MMUing]	"MMUing",
	[Exiting]	"Exiting",
	[Inwait]	"Inwait",
	[Wakeme]	"Wakeme",
	[Broken]	"Broken",
};

static
void
cmd_trace(int argc, char *argv[])
{
	int n;

	n = 0;
	if(argc > 1)
		n = number(argv[1], n, 10);
	dotrace(n);
}

static
void
cmd_cpu(int argc, char *argv[])
{
	int i, j;
	User *p;
	char *text;

	p = procalloc.arena;
	for(i=0; i<conf.nproc; i++,p++) {
		text = p->text;
		if(!text)
			continue;
		for(j=1; j<argc; j++)
			if(strcmp(argv[j], text) == 0)
				goto found;
		if(argc > 1)
			continue;
	found:
		print("	%2d %.3s %9s%7W%7W%7W\n",
			p->pid, text,
			statename[p->state],
			p->time+0, p->time+1, p->time+2);
		prflush();
	}
}

/*
 * Always splhi()'ed.
 */
void
schedinit(void)		/* never returns */
{
	User *p;

	setlabel(&m->sched);
	if(u) {
		m->proc = 0;
		p = u;
		u = 0;
		if(p->state == Running)
			ready(p);
		p->mach = 0;
	}
	sched();
}

void
sched(void)
{
	User *p;
	void (*f)(Ureg *, ulong);

	if(u) {
		splhi();
		if(setlabel(&u->sched)) {	/* woke up */
			if(u->mach)
				panic("mach non zero");
			u->state = Running;
			u->mach = m;
			m->proc = u;
			spllo();
			return;
		}
		gotolabel(&m->sched);
	}
	if(f = m->intr){			/* assign = */
		m->intr = 0;
		(*f)(m->ureg, m->cause);
	}
	spllo();
	p = runproc();
	splhi();
	u = p;
	gotolabel(&p->sched);
}

void
ready(User *p)
{
	int s;

	s = splhi();
	lock(&runq);
	p->rnext = 0;
	if(runq.tail)
		runq.tail->rnext = p;
	else
		runq.head = p;
	runq.tail = p;
	p->state = Ready;
	unlock(&runq);
	splx(s);
}

/*
 * Always called spllo
 */
User*
runproc(void)
{
	User *p;

loop:
	while(runq.head == 0)
		;
	splhi();
	lock(&runq);
	p = runq.head;
	if(p==0 || p->mach){
		unlock(&runq);
		spllo();
		goto loop;
	}
	if(p->rnext == 0)
		runq.tail = 0;
	runq.head = p->rnext;
	if(p->state != Ready)
		print("runproc %d %s\n", p->pid, statename[p->state]);
	unlock(&runq);
	p->state = Scheding;
	spllo();
	return p;
}

User*
newproc(void)
{
	User *p;

loop:
	lock(&procalloc);
	if(p = procalloc.free) {		/* assign = */
		procalloc.free = p->qnext;
		p->state = Zombie;
		unlock(&procalloc);
		p->mach = 0;
		p->qnext = 0;
		p->exiting = 0;
		lock(&pidalloc);
		p->pid = ++pidalloc.pid;
		unlock(&pidalloc);
		return p;
	}
	unlock(&procalloc);
	panic("no procs");
	goto loop;
}

void
procinit(void)
{
	User *p;
	int i;

	procalloc.free = ialloc(conf.nproc*sizeof(User), 0);
	procalloc.arena = procalloc.free;

	p = procalloc.free;
	for(i=0; i<conf.nproc-1; i++,p++) {
		p->qnext = p+1;
		wakeup(&p->tsleep);
	}
	p->qnext = 0;
	wakeup(&p->tsleep);

	cmd_install("cpu", "[proc] -- process cpu time", cmd_cpu); /**/
	cmd_install("trace", "[number] -- stack trace/qlocks", cmd_trace); /**/
}

void
sleep(Rendez *r, int (*f)(void*), void *arg)
{
	User *p;
	int s;

	/*
	 * spl is to allow lock to be called
	 * at interrupt time. lock is mutual exclusion
	 */
	s = splhi();
	lock(r);

	/*
	 * if condition happened, never mind
	 */
	if((*f)(arg)) {	
		unlock(r);
		splx(s);
		return;
	}

	/*
	 * now we are committed to
	 * change state and call scheduler
	 */
	p = r->p;
	if(p)
		print("double sleep %d\n", p->pid);
	u->state = Wakeme;
	r->p = u;

	/*
	 * sched does the unlock(u) when it is safe
	 * it also allows to be called splhi.
	 */
	unlock(r);
	sched();
}

int
tfn(void *arg)
{
	return MACHP(0)->ticks >= u->twhen || (*u->tfn)(arg);
}

void
tsleep(Rendez *r, int (*fn)(void*), void *arg, int ms)
{
	ulong when;
	User *f, **l;

	when = MS2TK(ms)+MACHP(0)->ticks;

	lock(&talarm);
	/* take out of list if checkalarm didn't */
	if(u->trend) {
		l = &talarm.list;
		for(f = *l; f; f = f->tlink) {
			if(f == u) {
				*l = u->tlink;
				break;
			}
			l = &f->tlink;
		}
	}
	/* insert in increasing time order */
	l = &talarm.list;
	for(f = *l; f; f = f->tlink) {
		if(f->twhen >= when)
			break;
		l = &f->tlink;
	}
	u->trend = r;
	u->twhen = when;
	u->tfn = fn;
	u->tlink = *l;
	*l = u;
	unlock(&talarm);

	sleep(r, tfn, arg);
	u->twhen = 0;
}

void
wakeup(Rendez *r)
{
	User *p;
	int s;

	s = splhi();
	lock(r);
	p = r->p;
	if(p) {
		r->p = 0;
		if(p->state != Wakeme)
			panic("wakeup: not Wakeme");
		ready(p);
	}
	unlock(r);
	splx(s);
}

void
dotrace(int n)
{
	User *p;
	QLock *q;
	int i, j, f;

	p = procalloc.arena;
	for(i=0; i<conf.nproc; i++, p++) {
		if(n) {
			if(p->pid == n)
				dumpstack(p);
			continue;
		}
		if(q = p->has.want) {
			if(q->name)
				print("pid %d wants %.10s\n",
					p->pid, q->name);
			else
				print("pid %d wants %p\n",
					p->pid, q);
		}
		f = 0;
		for(j=0; j<NHAS; j++) {
			if(q = p->has.q[j]) {
				if(f == 0) {
					if(p->has.want == 0)
						print("pid %d wants nothing\n", p->pid);
					print("	has");
					f = 1;
				}
				if(q->name)
					print(" %.10s", q->name);
				else
					print(" %p", q);
			}
		}
		if(f)
			print("\n");
	}
}
