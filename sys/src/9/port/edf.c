/* EDF scheduling */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"../port/devrealtime.h"
#include	"../port/edf.h"

/* debugging */
int			edfprint = 0;
#define DPRINT	if(edfprint)iprint

char *edfstatename[] = {
	[EdfUnused] =		"Unused",
	[EdfExpelled] =		"Expelled",
	[EdfAdmitted] =	"Admitted",
	[EdfIdle] =			"Idle",
	[EdfAwaitrelease] =	"Awaitrelease",
	[EdfReleased] =		"Released",
	[EdfRunning] =		"Running",
	[EdfExtra] =		"Extra",
	[EdfPreempted] =	"Preempted",
	[EdfBlocked] =		"Blocked",
	[EdfDeadline] =		"Deadline",
};

static Timer	deadlinetimer[MAXMACH];	/* Time of next deadline */
static Timer	releasetimer[MAXMACH];		/* Time of next release */

static int		initialized;
static uvlong	fasthz;	
static Ticks	now;
QLock		edfschedlock;
Lock			edflock;

Task			tasks[Maxtasks];
int			ntasks;
Resource		resources[Maxresources];
int			nresources;
int			edfstateupdate;
int			misseddeadlines;

enum{
	Deadline,	/* Invariant for schedulability test: Deadline < Release */
	Release,
};

static int earlierrelease(Task *t1, Task *t2) {return t1->r < t2->r;}
static int earlierdeadline(Task *t1, Task *t2) {return t1->d < t2->d;}

/* Tasks waiting for release, head earliest release time */
Taskq		qwaitrelease =	{{0}, nil, earlierrelease};

/* Released tasks waiting to run, head earliest deadline */
Taskq		qreleased	=	{{0}, nil, earlierdeadline};

/* Exhausted EDF tasks, append at end */
Taskq		qextratime;

/* Running/Preempted EDF tasks, head running, one stack per processor */
Taskq		edfstack[MAXMACH];

void (*devrt)(Task*, Ticks, int);

static void		edfresched(Task *t);
static void		setdelta(void);
static void		testdelta(Task *thetask);
static void		edfreleaseintr(Ureg *, Timer *cy);
static void		edfdeadlineintr(Ureg*, Timer*);
static char *	edftestschedulability(Task *thetask);

void
edfinit(void)
{
	int i;

	if (initialized)
		return;
	ilock(&edflock);
	if (initialized){
		iunlock(&edflock);
		return;
	}
	fastticks(&fasthz);
	for (i = 0; i < conf.nmach; i++){
		deadlinetimer[i].f = edfdeadlineintr;
		deadlinetimer[i].a = &deadlinetimer[i];
		deadlinetimer[i].when = 0;
		releasetimer[i].f = edfreleaseintr;
		releasetimer[i].a = &releasetimer[i];
		releasetimer[i].when = 0;
	}
	initialized = 1;
	iunlock(&edflock);
}

int
isedf(Proc *p)
{
	return p && p->task && p->task->state >= EdfIdle;
}

int
edfanyready(void)
{
	return edfstack[m->machno].head || qreleased.head;
}

static void
edfpush(Task *t)
{
	Taskq *q;

	DPRINT("%d edfpush, %s, %d\n", m->machno, edfstatename[t->state], t->runq.n);
	q = edfstack + m->machno;
	assert(t->runq.n || (up && up->task == t));
	if (q->head){
		assert(q->head->state == EdfRunning);
		q->head->state = EdfPreempted;
		if(devrt) devrt(q->head, now, SPreempt);
	}
	t->rnext = q->head;
	if(devrt) devrt(t, now, SRun);
	q->head = t;
}

static Task*
edfpop(void)
{
	Task *t;
	Taskq *q;

	DPRINT("%d edfpop\n", m->machno);
	q = edfstack + m->machno;
	if (t = q->head){
		assert(t->state == EdfRunning);
		q->head = t->rnext;
		t->rnext = nil;
		if (q->head){
			assert(q->head->state == EdfPreempted);
			q->head->state = EdfRunning;
			if(devrt) devrt(q->head, now, SRun);
		}
	}
	return t;
}

static Task*
edfenqueue(Taskq *q, Task *t)
{
	Task *tt, **ttp;

	DPRINT("%d edfenqueue, %s, %d\n", m->machno, edfstatename[t->state], t->runq.n);
	t->rnext = nil;
	if (q->head == nil) {
		q->head = t;
		return t;
	}
	SET(tt);
	for (ttp = &q->head; *ttp; ttp = &tt->rnext) {
		tt = *ttp;
		if (q->before && q->before(t, tt)) {
			t->rnext = tt;
			*ttp = t;
			break;
		}
	}
	if (*ttp == nil)
		tt->rnext = t;
	if (t != q->head)
		t = nil;
	return t;
}

static Task*
edfdequeue(Taskq *q)
{
	Task *t;

	DPRINT("%d edfdequeue\n", m->machno);
	if (t = q->head){
		q->head = t->rnext;
		t->rnext = nil;
	}
	return t;
}

static Task*
edfqremove(Taskq *q, Task *t)
{
	Task **tp;

	DPRINT("%d edfqremove, %s, %d\n", m->machno, edfstatename[t->state], t->runq.n);
	for (tp = &q->head; *tp; tp = &(*tp)->rnext){
		if (*tp == t){
			*tp = t->rnext;
			t = (tp == &q->head) ? q->head : nil;
			return t;
		}
	}
	return nil;
}

			
void
edfreleasetimer(void)
{
	Task *t;

	if ((t = qwaitrelease.head) == nil)
		return;
	DPRINT("edfreleasetimer clock\n");
	if (releasetimer[m->machno].when)
		timerdel(&releasetimer[m->machno]);
	releasetimer[m->machno].when = t->r;
	if (releasetimer[m->machno].when <= now)
		releasetimer[m->machno].when = now;
	timeradd(&releasetimer[m->machno]);
}

void
edfblock(Proc *p)
{
	Task *t, *pt;

	/* The current proc has blocked */
	ilock(&edflock);
	t = p->task;
	assert(t);
	if (t->state != EdfRunning){
		/* called by a proc just joining the task */
		iunlock(&edflock);
		return;
	}
	DPRINT("%d edfblock, %s, %d\n", m->machno, edfstatename[t->state], t->runq.n);

	if (t->runq.n){
		/* There's another runnable proc in the running task, leave task where it is */
		iunlock(&edflock);
		return;
	}
	pt = edfpop();
	assert(pt == t);
	t->state = EdfBlocked;
	if(devrt) devrt(t, now, SBlock);
	iunlock(&edflock);
}

static void
deadline(Proc *p, SEvent why)
{
	Task *t, *nt;

	/* Task has reached its deadline, lock must be held */
	DPRINT("%d deadline, %s, %d\n", m->machno, edfstatename[p->task->state], p->task->runq.n);
	SET(nt);
	if (p){
		nt = p->task;
		if (nt == nil || nt->state != EdfRunning)
			return;
	}
	t = edfpop();

	if(p != nil && nt != t){
		iprint("deadline, %s, %d\n", edfstatename[p->task->state], p->task->runq.n);
		iunlock(&edflock);
		assert(0 && p == nil || nt == t);
	}
	if (deadlinetimer[m->machno].when){
		timerdel(&deadlinetimer[m->machno]);
		deadlinetimer[m->machno].when = 0;
	}
	t->d = now;
	t->state = EdfDeadline;
	if(devrt) devrt(t, now, why);
	edfresched(t);
}

void
edfdeadline(Proc *p)
{
	DPRINT("%d edfdeadline\n", m->machno);
	/* Task has reached its deadline */
	ilock(&edflock);
	now = fastticks(nil);
	deadline(p, SYield);
	iunlock(&edflock);
}

char *
edfadmit(Task *t)
{
	char *err;

	if (t->state != EdfExpelled)
		return "task state";	/* should never happen */

	/* simple sanity checks */
	if (t->T == 0)
		return "T not set";
	if (t->C == 0)
		return "C not set";
	if (t->D > t->T)
		return "D > T";
	if (t->D == 0)	/* if D is not set, set it to T */
		t->D = t->T;
	if (t->C > t->D)
		return "C > D";

	qlock(&edfschedlock);
	if (err = edftestschedulability(t)){
		qunlock(&edfschedlock);
		return err;
	}
	ilock(&edflock);
	DPRINT("%d edfadmit, %s, %d\n", m->machno, edfstatename[t->state], t->runq.n);
	now = fastticks(nil);

	t->state = EdfAdmitted;
	if(devrt) devrt(t, t->d, SAdmit);
	if (up->task == t){
		DPRINT("%d edfadmitting self\n", m->machno);
		/* Admitting self, fake reaching deadline */
		t->r = now;
		t->t = now + t->T;
		t->d = now + t->D;
		if(devrt) devrt(t, t->d, SDeadline);
		t->S = t->C;
		t->scheduled = now;
		t->state = EdfRunning;
		if(devrt) devrt(t, now, SRun);
		setdelta();
		assert(t->runq.n > 0 || (up && up->task == t));
		edfpush(t);
		if (deadlinetimer[m->machno].when)
			timerdel(&deadlinetimer[m->machno]);
		deadlinetimer[m->machno].when = t->d;
		timeradd(&deadlinetimer[m->machno]);
	}else{
		if (t->runq.n){
			if (edfstack[m->machno].head == nil){
				t->state = EdfAdmitted;
				t->r = now;
				edfrelease(t);
				setdelta();
				edfresched(t);
			}
		}
	}
	iunlock(&edflock);
	qunlock(&edfschedlock);
	return nil;
}

void
edfexpel(Task *t)
{
	Task *tt;

	qlock(&edfschedlock);
	ilock(&edflock);
	DPRINT("%d edfexpel, %s, %d\n", m->machno, edfstatename[t->state], t->runq.n);
	now = fastticks(nil);
	switch(t->state){
	case EdfUnused:
	case EdfExpelled:
		/* That was easy */
		iunlock(&edflock);
		qunlock(&edfschedlock);
		return;
	case EdfAdmitted:
	case EdfIdle:
		/* Just reset state */
		break;
	case EdfAwaitrelease:
		if (edfqremove(&qwaitrelease, t))
			edfreleasetimer();
		break;
	case EdfReleased:
		edfqremove(&qreleased, t);
		break;
	case EdfRunning:
		/* Task must be expelling itself */
		tt = edfpop();
		assert(t == tt);
		break;
	case EdfExtra:
		edfqremove(&qextratime, t);
		break;
	case EdfPreempted:
		edfqremove(edfstack + m->machno, t);
		break;
	case EdfBlocked:
	case EdfDeadline:
		break;
	}
	t->state = EdfExpelled;
	if(devrt) devrt(t, now, SExpel);
	setdelta();
	iunlock(&edflock);
	qunlock(&edfschedlock);
	return;
}

static void
edfreleaseintr(Ureg*, Timer*)
{
	Task *t;

	DPRINT("%d edfreleaseintr\n", m->machno);

	timerdel(&releasetimer[m->machno]);
	releasetimer[m->machno].when = 0;

	if(active.exiting)
		return;

	ilock(&edflock);
	now = fastticks(nil);
	while((t = qwaitrelease.head) && t->r <= now){
		/* There's something waiting to be released and its time has come */
		edfdequeue(&qwaitrelease);
		edfreleasetimer();
		edfrelease(t);
	}
	iunlock(&edflock);
	sched();
	splhi();
}

static void
edfdeadlineintr(Ureg*, Timer*)
{
	/* Task reached deadline */

	Ticks used;
	Task *t;

	DPRINT("%d edfdeadlineintr\n", m->machno);

	timerdel(&deadlinetimer[m->machno]);
	deadlinetimer[m->machno].when = 0;

	if(active.exiting)
		return;

	ilock(&edflock);
	// If up is not set, we're running inside the scheduler
	// for non-real-time processes. 
	if (up && isedf(up)) {
		now = fastticks(nil);

		t = up->task;
		assert(t->scheduled > 0);
	
		used = now - t->scheduled;
		t->scheduled = now;

		if (t->r < now){
			if (t->S <= used)
				t->S = 0LL;
			else
				t->S -= used;
	
			if (t->d <= now || t->S == 0LL){
				/* Task has reached its deadline/slice, remove from queue */
				if (t->S > 0LL)
					misseddeadlines++;
				deadline(up, SSlice);
				while (t = edfstack[m->machno].head){
					if (now < t->d)
						break;
					deadline(nil, SSlice);
				}	
			}
		}
	}
	iunlock(&edflock);
	sched();
	splhi();
}

void
edfbury(Proc *p)
{
	Task *t;
	Proc **pp;

	DPRINT("%d edfbury\n", m->machno);
	ilock(&edflock);
	now = fastticks(nil);
	if ((t = p->task) == nil){
		/* race condition? */
		iunlock(&edflock);
		DPRINT("%d edf bury race, pid %lud\n", m->machno, p->pid);
		return;
	}
	assert(edfstack[m->machno].head == t);
	for (pp = t->procs; pp < t->procs + nelem(t->procs); pp++)
		if (*pp == p){
			t->nproc--;
			*pp = nil;
		}
	if (t->runq.head == nil){
		edfpop();
		t->state = EdfBlocked;
	}
	if (t->nproc == 0){
		assert(t->runq.head == nil);
		t->state = EdfIdle;
	}
	if(devrt) devrt(t, now, SBlock);
	p->task = nil;
	iunlock(&edflock);
}

void
edfready(Proc *p)
{
	Task *t;

	ilock(&edflock);
	DPRINT("%d edfready, %s, %d\n", m->machno, edfstatename[p->task->state], p->task->runq.n);
	if ((t = p->task) == nil){
		/* Must be a race */
		iunlock(&edflock);
		DPRINT("%d edf ready race, pid %lud\n", m->machno, p->pid);
		return;
	}
	p->rnext = 0;
	p->readytime = m->ticks;
	p->state = Ready;
	t->runq.n++;
	if(t->runq.tail){
		t->runq.tail->rnext = p;
		t->runq.tail = p;
	}else{
		t->runq.head = p;
		t->runq.tail = p;

		/* first proc to become runnable in this task */
		now = fastticks(nil);
		edfresched(t);
	}
	iunlock(&edflock);
}

static void
edfresched(Task *t)
{
	Task *xt;

	DPRINT("%d edfresched, %s, %d\n", m->machno, edfstatename[t->state], t->runq.n);
	if (t->nproc == 0){
		/* No member processes */
		if (t->state > EdfIdle){
			t->state = EdfIdle;
			if(devrt) devrt(t, now, SBlock);
		}
		return;
	}
	if (t->runq.n == 0 && (up == nil || up->task != t)){
		/* Member processes but none runnable */
		DPRINT("%d edfresched, nothing runnable\n", m->machno);
		if (t->state == EdfRunning)
			edfpop();

		if (t->state >= EdfIdle && t->state != EdfBlocked){
			t->state = EdfBlocked;
			if(devrt) devrt(t, now, SBlock);
		}
		return;
	}

	/* There are runnable processes */

	switch (t->state){
	case EdfUnused:
		iprint("%d attempt to schedule unused task\n", m->machno);
	case EdfExpelled:
		return;	/* Not admitted */
	case EdfIdle:
		/* task was idle, schedule release now or later */
		if (t->r < now){
			if (t->t < now)
				t->t = now + t->T;
			t->r = t->t;
		}
		edfrelease(t);
		break;
	case EdfAwaitrelease:
	case EdfReleased:
	case EdfExtra:
	case EdfPreempted:
		/* dealt with by timer */
		break;
	case EdfAdmitted:
		/* test whether task can be started */
		if (edfstack[m->machno].head != nil){
			return;
		}
		/* fall through */
	case EdfRunning:
		if (t->r <= now){
			if (t->t < now){
				DPRINT("%d edfresched, rerelease\n", m->machno);
				/* Period passed, rerelease */
				t->r = now;
				xt = edfpop();
				assert(xt == t);
				edfrelease(t);
				return;
			}
			if (now < t->d){
				if (t->S > 0){
					DPRINT("%d edfresched, resume\n", m->machno);
					/* Running, not yet at deadline, leave it */
					return;
				}else
					t->d = now;
			}
			/* Released, but deadline is past, release at t->t */
			t->r = t->t;
		}
		xt = edfpop();
		assert(xt == t);
		t->state = EdfAwaitrelease;
		if (edfenqueue(&qwaitrelease, t))
			edfreleasetimer();
		break;
	case EdfBlocked:
	case EdfDeadline:
		if (t->r <= now){
			if (t->t < now){
				DPRINT("%d edfresched, rerelease\n", m->machno);
				/* Period passed, rerelease */
				t->r = now;
				edfrelease(t);
				return;
			}
			if (now < t->d && (t->flags & Useblocking) == 0){
				if (t->S > 0){
					DPRINT("%d edfresched, resume\n", m->machno);
					/* Released, not yet at deadline, release (again) */
					t->state = EdfReleased;
					edfenqueue(&qreleased, t);
					if(devrt) devrt(t, now, SResume);
					return;
				}else
					t->d = now;
			}
			/* Released, but deadline is past, release at t->t */
			t->r = t->t;
		}
		t->state = EdfAwaitrelease;
		if (edfenqueue(&qwaitrelease, t))
			edfreleasetimer();
		break;
	}
}

void
edfrelease(Task *t)
{
	DPRINT("%d edfrelease, %s, %d\n", m->machno, edfstatename[t->state], t->runq.n);
	assert(t->runq.n > 0 || (up && up->task == t));
	t->t = t->r + t->T;
	t->d = t->r + t->D;
	if(devrt) devrt(t, t->d, SDeadline);
	t->S = t->C;
	t->state = EdfReleased;
	edfenqueue(&qreleased, t);
	if(devrt) devrt(t, now, SRelease);
}

Proc *
edfrunproc(void)
{
	/* Return an edf proc to run or nil */
	
	Task *t, *nt, *xt;
	Proc *p;
	Ticks when;
	static ulong nilcount;
	int i;

	if (edfstack[m->machno].head == nil && qreleased.head== nil){
		// quick way out
		nilcount++;
		return nil;
	}
	/* Figure out if the current proc should be preempted*/
	ilock(&edflock);
	now = fastticks(nil);

	/* first candidate is at the top of the stack of running procs */
	t = edfstack[m->machno].head;

	/* check out head of the release queue for a proc with a better deadline */
	nt = qreleased.head;

	if (t == nil && nt == nil){
		nilcount++;
		iunlock(&edflock);
		return nil;
	}
	DPRINT("edfrunproc %lud\n", nilcount);
	if (nt && (t == nil || (nt->d < t->d && nt->D < t->Delta))){
		if (conf.nmach > 1){
			for (i = 0; i < conf.nmach; i++){
				if (i == m->machno)
					continue;
				xt = edfstack[i].head;
				if (xt && xt->Delta <= nt->D){
					DPRINT("%d edfrunproc: interprocessor conflict, run current\n", m->machno);
					if (t)
						goto runt;
					nilcount++;
					iunlock(&edflock);
					return nil;
				}
			}
		}
		/* released task is better than current */
		DPRINT("%d edfrunproc: released\n", m->machno);
		edfdequeue(&qreleased);
		assert(nt->runq.n >= 1);
		edfpush(nt);
		nt->state = EdfRunning;
		t = nt;
		t->scheduled = now;
	}else{
		DPRINT("%d edfrunproc: current\n", m->machno);
	}
runt:
	assert (t->runq.n);

	/* Get first proc off t's run queue
	 * No need to lock runq, edflock always held to access runq
	 */
	t->state = EdfRunning;
	p = t->runq.head;
	if ((t->runq.head = p->rnext) == nil)
		t->runq.tail = nil;
	t->runq.n--;
	p->state = Scheding;
	if(p->mp != MACHP(m->machno))
		p->movetime = MACHP(0)->ticks + HZ/10;
	p->mp = MACHP(m->machno);

	when = now + t->S;
	if (t->d < when)
		when = t->d;

	if (when < now){
		DPRINT("%d edftimer: %T too late\n", m->machno, ticks2time(now-when));
		when = now;
	}
	if (deadlinetimer[m->machno].when){
		if(deadlinetimer[m->machno].when == when){
			iunlock(&edflock);
			return p;
		}
		timerdel(&deadlinetimer[m->machno]);
	}
	deadlinetimer[m->machno].when = when;
	timeradd(&deadlinetimer[m->machno]);
	iunlock(&edflock);
	return p;
}

/* Schedulability testing and its supporting routines */

static void
setdelta(void)
{
	Resource *r, **rr;
	Task **tt, *t;

	for (r = resources; r < resources + nelem(resources); r++){
		if (r->name == nil)
			continue;
		r->Delta = ~0LL;
		for (tt = r->tasks; tt < r->tasks + nelem(r->tasks); tt++)
			if (*tt && (*tt)->D < r->Delta)
				r->Delta = (*tt)->D;
	}
	for (t = tasks; t < tasks + nelem(tasks); t++){
		if (t->state < EdfIdle)
			continue;
		t->Delta = t->D;
		for (rr = t->res; rr < t->res + nelem(t->res); rr++)
			if (*rr && (*rr)->Delta < t->Delta)
				t->Delta = (*rr)->Delta;
	}
}

static void
testdelta(Task *thetask)
{
	Resource *r, **rr;
	Task **tt, *t;

	for (r = resources; r < resources + nelem(resources); r++){
		if (r->name == nil)
			continue;
		r->testDelta = ~0ULL;
		for (tt = r->tasks; tt < r->tasks + nelem(r->tasks); tt++)
			if (*tt && (*tt)->D < r->testDelta)
				r->testDelta = (*tt)->D;
	}
	for (t = tasks; t < tasks + nelem(tasks); t++){
		if (t->state <= EdfExpelled && t != thetask)
			continue;
		t->testDelta = t->D;
		for (rr = t->res; rr < t->res + nelem(t->res); rr++)
			if (*rr && (*rr)->testDelta < t->testDelta)
				t->testDelta = (*rr)->testDelta;
	}
}

static Ticks
blockcost(Ticks ticks, Task *thetask)
{
	Task *t;
	Ticks Cb;

	Cb = 0;
	for (t = tasks; t < tasks + Maxtasks; t++){
		if (t->state <= EdfExpelled && t != thetask)
			continue;
		if (t->testDelta <= ticks && ticks < t->D && Cb < t->C)
			Cb = t->C;
	}
	return Cb;
}

static Task *qschedulability;

static void
testenq(Task *t)
{
	Task *tt, **ttp;

	t->testnext = nil;
	if (qschedulability == nil) {
		qschedulability = t;
		return;
	}
	SET(tt);
	for (ttp = &qschedulability; *ttp; ttp = &tt->testnext) {
		tt = *ttp;
		if (t->testtime < tt->testtime
		|| (t->testtime == tt->testtime && t->testtype < tt->testtype)){
			t->testnext = tt;
			*ttp = t;
			return;
		}
	}
	assert(tt->testnext == nil);
	tt->testnext = t;
}

static char *
edftestschedulability(Task *thetask)
{
	Task *t;
	Ticks H, G, Cb, ticks;
	int steps;

	/* initialize */
	testdelta(thetask);
	if (thetask && (thetask->flags & Verbose))
		pprint("schedulability test\n");
	qschedulability = nil;
	for (t = tasks; t < tasks + Maxtasks; t++){
		if (t->state <= EdfExpelled && t != thetask)
			continue;
		t->testtype = Release;
		t->testtime = 0;
		if (thetask && (thetask->flags & Verbose))
			pprint("\tInit: enqueue task %lud\n", t - tasks);
		testenq(t);
	}
	H=0;
	G=0;
	for(steps = 0; steps < Maxsteps; steps++){
		t = qschedulability;
		qschedulability = t->testnext;
		ticks = t->testtime;
		switch (t->testtype){
		case Deadline:
			H += t->C;
			Cb = blockcost(ticks, thetask);
			if (thetask && (thetask->flags & Verbose))
				pprint("\tStep %3d, Ticks %T, task %lud, deadline, H += %T → %T, Cb = %T\n",
					steps, ticks2time(ticks), t - tasks,
					ticks2time(t->C), ticks2time(H), ticks2time(Cb));
			if (H+Cb>ticks)
				return "not schedulable";
			t->testtime += t->T - t->D;
			t->testtype = Release;
			testenq(t);
			break;
		case Release:
			if (thetask && (thetask->flags & Verbose))
				pprint("\tStep %3d, Ticks %T, task %lud, release, G  %T, C%T\n",
					steps, ticks2time(ticks), t - tasks,
					ticks2time(t->C), ticks2time(G));
			if(ticks && G <= ticks)
				return nil;
			G += t->C;
			t->testtime += t->D;
			t->testtype = Deadline;
			testenq(t);
			break;
		default:
			assert(0);
		}
	}
	return "probably not schedulable";
}

static uvlong
uvmuldiv(uvlong x, ulong num, ulong den)
{
	/* multiply, then divide, avoiding overflow */
	uvlong hi;

	hi = (x & 0xffffffff00000000LL) >> 32;
	x &= 0xffffffffLL;
	hi *= num;
	return (x*num + (hi%den << 32)) / den + (hi/den << 32);
}

Time
ticks2time(Ticks ticks)
{
	assert(ticks >= 0);
	if (fasthz == 0)
		fastticks(&fasthz);
	return uvmuldiv(ticks, Onesecond, fasthz);
}

Ticks
time2ticks(Time time)
{
	assert(time >= 0);
	return uvmuldiv(time, fasthz, Onesecond);
}
