/* EDF scheduling */
#include	<u.h>
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"../port/edf.h"
#include	<trace.h>

/* debugging */
int	edfprint = 0;
#define DPRINT	if(edfprint)print

static vlong	now;
extern ulong	delayedscheds;
extern Schedq	runq[Nrq];
extern int	nrdy;
extern ulong	runvec;

/* Statistics stuff */
ulong		nilcount;
ulong		scheds;
vlong		edfruntime;
ulong		edfnrun;
int		misseddeadlines;

/* Edfschedlock protects modification of admission params */
int		edfinited;
QLock		edfschedlock;
static Lock	thelock;

enum{
	Dl,	/* Invariant for schedulability test: Dl < Rl */
	Rl,
};

static char *testschedulability(Proc*);
static Proc *qschedulability;

enum {
	Onemicrosecond =	1000ULL,
	Onemillisecond =	1000000ULL,
	Onesecond =		1000000000ULL,
	OneRound = 		Onemillisecond/2LL,
	MilliRound = 		Onemicrosecond/2LL,
};

static int
timeconv(Fmt *f)
{
	char buf[128], *sign;
	vlong t;

	buf[0] = 0;
	switch(f->r) {
	case 'U':
		t = va_arg(f->args, uvlong);
		break;
	case 't':		// vlong in nanoseconds
		t = va_arg(f->args, vlong);
		break;
	default:
		return fmtstrcpy(f, "(timeconv)");
	}
	if (t < 0) {
		sign = "-";
		t = -t;
	}
	else
		sign = "";
	if (t > Onesecond){
		t += OneRound;
		sprint(buf, "%s%d.%.3ds", sign, (int)(t / Onesecond), (int)(t % Onesecond)/1000000);
	}else if (t > Onemillisecond){
		t += MilliRound;
		sprint(buf, "%s%d.%.3dms", sign, (int)(t / Onemillisecond), (int)(t % Onemillisecond)/1000);
	}else if (t > Onemicrosecond)
		sprint(buf, "%s%d.%.3dµs", sign, (int)(t / Onemicrosecond), (int)(t % Onemicrosecond));
	else
		sprint(buf, "%s%dns", sign, (int)t);
	return fmtstrcpy(f, buf);
}

void
edflock(void)
{
	ilock(&thelock);
	now = todget(nil);
}

void
edfunlock(void)
{
	edfruntime += todget(nil) - now;
	edfnrun++;
	iunlock(&thelock);
}

void
edfinit(Proc*p)
{
	if(!edfinited){
		fmtinstall('t', timeconv);
		edfinited++;
	}
	now = todget(nil);
	DPRINT("%t edfinit %lud[%s]\n", now, p->pid, statename[p->state]);
	p->edf = malloc(sizeof(Edf));
	if(p->edf == nil)
		error(Enomem);
	return;
}

static void
deadlineintr(Ureg*, Timer *t)
{
	/* Proc reached deadline */
	extern int panicking;
	Proc *p;

	if(panicking || active.exiting)
		return;

	p = t->ta;

	DPRINT("%t deadlineintr %lud[%s]\n", todget(nil), p->pid, statename[p->state]);
	/* If we're interrupting something other than the proc pointed to by t->a,
	 * we've already achieved recheduling, so we need not do anything
	 * Otherwise, we must cause a reschedule, but if we call sched()
	 * here directly, the timer interrupt routine will not finish its business
	 * Instead, we cause the resched to happen when the interrupted proc
	 * returns to user space
	 */
	if (p == up){
		up->delaysched++;
 		delayedscheds++;
	}
}

static void
release(Proc *p)
{
	/* Called with edflock held */
	Edf *e;
	void (*pt)(Proc*, int, vlong);

	e = p->edf;
	e->flags &= ~Yield;
	if (e->d < now){
		e->periods++;
		e->r = now;
		if ((e->flags & Sporadic) == 0){
			/* Non sporadic processes stay true to their period;
			 * calculate next release time
			 */
			while(e->t <= now)
				e->t += e->T;
		}else{
			/* Sporadic processes may not be released earlier than
			 * one period after this release
			 */
			e->t = e->r + e->T;
		}
		e->d = e->r + e->D;
		e->S = e->C;
		DPRINT("%t release %lud[%s], r=%t, d=%t, t=%t, S=%t\n",
			now, p->pid, statename[p->state], e->r, e->d, e->t, e->S);
		if (pt = proctrace){
			pt(p, SRelease, e->r);
			pt(p, SDeadline, e->d);
		}
	}else{
		DPRINT("%t release %lud[%s], too late t=%t, called from 0x%lux\n",
			now, p->pid, statename[p->state], e->t, getcallerpc(&p));
	}
}

static void
releaseintr(Ureg*, Timer *t)
{
	Proc *p;
	Edf *e;
	extern int panicking;
	Schedq *rq;

	if(panicking || active.exiting)
		return;

	p = t->ta;
	e = p->edf;
	if ((e->flags & Admitted) == 0)
		return;
	edflock();
	DPRINT("%t releaseintr %lud[%s]\n", now, p->pid, statename[p->state]);
	switch(p->state){
	default:
		edfunlock();
		return;
	case Ready:
		/* remove proc from current runq */
		rq = &runq[p->priority];
		if (dequeueproc(rq, p) != p){
			print("releaseintr: can't find proc or lock race\n");
			release(p);	/* It'll start best effort */
			edfunlock();
			return;
		}
		p->state = Waitrelease;
		/* fall through */
	case Waitrelease:
		release(p);
		edfunlock();
		ready(p);
		if (up){
			up->delaysched++;
			delayedscheds++;
		}
		return;
	case Running:
		release(p);
		edfrun(p, 1);
		break;
	case Wakeme:
		release(p);
		edfunlock();
		if (p->trend)
			wakeup(p->trend);
		p->trend = nil;
		if (up){
			up->delaysched++;
			delayedscheds++;
		}
		return;
	}
	edfunlock();
}

void
edfrecord(Proc *p)
{
	vlong used;
	Edf *e;
	void (*pt)(Proc*, int, vlong);

	e = p->edf;
	edflock();
	used = now - e->s;
	if (e->d <= now)
		e->edfused += used;
	else
		e->extraused += used;
	if (e->S > 0){
		if (e->S <= used){
			if(pt = proctrace)
				pt(p, SSlice, now);
			DPRINT("%t edfrecord slice used up\n", now);
			e->d = now;
			e->S = 0;
		}else
			e->S -= used;
	}
	e->s = now;
	edfunlock();
}

void
edfrun(Proc *p, int edfpri)
{
	Edf *e;

	e = p->edf;
	/* Called with edflock held */
	if(edfpri){
		if (e->d <= now || e->S == 0){
			/* Deadline reached or resources exhausted,
			 * deschedule forthwith
			 */
			p->delaysched++;
 			delayedscheds++;
			e->s = now;
			return;
		}
		p->tns = now + e->S;
		if (e->d < p->tns)
			p->tns = e->d;
		if(p->tt == nil || p->tf != deadlineintr){
			DPRINT("%t edfrun, deadline=%t\n", now, p->tns);
		}else{
			DPRINT("v");
		}
		p->tmode = Tabsolute;
		p->tf = deadlineintr;
		p->ta = p;
		timeradd(p);
	}else{
		DPRINT("<");
	}
	e->s = now;
}

char *
edfadmit(Proc *p)
{
	char *err;
	Edf *e;
	int i;
	Proc *r;
	void (*pt)(Proc*, int, vlong);

	e = p->edf;
	if (e->flags & Admitted)
		return "task state";	/* should never happen */

	/* simple sanity checks */
	if (e->T == 0)
		return "T not set";
	if (e->C == 0)
		return "C not set";
	if (e->D > e->T)
		return "D > T";
	if (e->D == 0)	/* if D is not set, set it to T */
		e->D = e->T;
	if (e->C > e->D)
		return "C > D";

	qlock(&edfschedlock);
	if (err = testschedulability(p)){
		qunlock(&edfschedlock);
		return err;
	}
	edflock();

	e->flags |= Admitted;

	if(pt = proctrace)
		pt(p, SAdmit, now);

	/* Look for another proc with the same period to synchronize to */
	SET(r);
	for(i=0; i<conf.nproc; i++) {
		r = proctab(i);
		if(r->state == Dead || r == p)
			continue;
		if (r->edf == nil || (r->edf->flags & Admitted) == 0)
			continue;
		if (r->edf->T == e->T)
				break;
	}
	if (i == conf.nproc){
		/* Can't synchronize to another proc, release now */
		e->t = now;
		e->d = 0;
		release(p);
		if (p == up){
			DPRINT("%t edfadmit self %lud[%s], release now: r=%t d=%t t=%t\n",
				now, p->pid, statename[p->state], e->r, e->d, e->t);
			/* We're already running */
			edfrun(p, 1);
		}else{
			/* We're releasing another proc */
			DPRINT("%t edfadmit other %lud[%s], release now: r=%t d=%t t=%t\n",
				now, p->pid, statename[p->state], e->r, e->d, e->t);
			p->ta = p;
			releaseintr(nil, p);
		}
	}else{
		/* Release in synch to something else */
		e->t = r->edf->t;
		if (p == up){
			DPRINT("%t edfadmit self %lud[%s], release at %t\n",
				now, p->pid, statename[p->state], e->t);
			edfunlock();
			qunlock(&edfschedlock);
			edfyield();
			return nil;
		}else{
			DPRINT("%t edfadmit other %lud[%s], release at %t\n",
				now, p->pid, statename[p->state], e->t);
			if(p->tt == nil){
				p->tf = releaseintr;
				p->ta = p;
				p->tns = e->t;
				p->tmode = Tabsolute;
				timeradd(p);
			}
		}
	}
	edfunlock();
	qunlock(&edfschedlock);
	return nil;
}

void
edfstop(Proc *p)
{
	Edf *e;
	void (*pt)(Proc*, int, vlong);

	if ((e = p->edf) && (e->flags & Admitted)){
		edflock();
		DPRINT("%t edfstop %lud[%s]\n", now, p->pid, statename[p->state]);
		if(pt = proctrace)
			pt(p, SExpel, now);
		e->flags &= ~Admitted;
		if (p->tt)
			timerdel(p);
		edfunlock();
	}
}

static int
yfn(void *)
{
	return todget(nil) >= up->edf->r;
}

void
edfyield(void)
{
	/* sleep until next release */
	Edf *e;
	void (*pt)(Proc*, int, vlong);

	edflock();
	e = up->edf;
	if(pt = proctrace)
		pt(up, SYield, now);
	while(e->t < now)
		e->t += e->T;
	e->r = e->t;
	e->flags |= Yield;
	e->d = now;
	up->tns = e->t;
	up->tf = releaseintr;
	up->tmode = Tabsolute;
	up->ta = up;
	up->trend = &up->sleep;
	timeradd(up);
	edfunlock();
	sleep(&up->sleep, yfn, nil);
}

int
edfready(Proc *p)
{
	Edf *e;
	Schedq *rq;
	Proc *l, *pp;
	void (*pt)(Proc*, int, vlong);

	if ((e = p->edf) == nil || (e->flags & Admitted) == 0)
		return 0;	/* Not an edf process */

	edflock();
	if (e->d <= now){
		/* past deadline, arrange for next release */
		if ((e->flags & Sporadic) == 0){
			/* Non sporadic processes stay true to their period, calculate next release time */
			while(e->t < now)
				e->t += e->T;
		}
		if (now < e->t){
			/* Next release is in the future, schedule it */
			if (p->tt == nil || p->tf != releaseintr){
				p->tns = e->t;
				p->tmode = Tabsolute;
				p->tf = releaseintr;
				p->ta = p;
				timeradd(p);
				DPRINT("%t edfready %lud[%s], release=%t\n",
					now, p->pid, statename[p->state], e->t);
			}
			if(p->state == Running && (e->flags & (Yield|Yieldonblock)) == 0){
				/* If we were running, we've overrun our CPU allocation
				 * or missed the deadline, continue running best-effort at low priority
				 * Otherwise we were blocked.  If we don't yield on block, we continue
				 * best effort
				 */
				DPRINT(">");
				p->basepri = PriExtra;
				p->fixedpri = 1;
				edfunlock();
				return 0;	/* Stick on runq[PriExtra] */
			}
			DPRINT("%t edfready %lud[%s] wait release at %t\n",
				now, p->pid, statename[p->state], e->t);
			p->state = Waitrelease;
			edfunlock();
			return 1;	/* Make runnable later */
		}
		DPRINT("%t edfready %lud %s release now\n", now, p->pid, statename[p->state]);
		/* release now */
		release(p);
	}
	DPRINT("^");
	rq = &runq[PriEdf];
	/* insert in queue in earliest deadline order */
	lock(runq);
	l = nil;
	for(pp = rq->head; pp; pp = pp->rnext){
		if(pp->edf->d > e->d)
			break;
		l = pp;
	}
	p->rnext = pp;
	if (l == nil)
		rq->head = p;
	else
		l->rnext = p;
	if(pp == nil)
		rq->tail = p;
	rq->n++;
	nrdy++;
	runvec |= 1 << PriEdf;
	p->priority = PriEdf;
	unlock(runq);
	p->readytime = m->ticks;
	p->state = Ready;
	if(pt = proctrace)
		pt(p, SReady, now);
	edfunlock();
	return 1;
}


static void
testenq(Proc *p)
{
	Proc *xp, **xpp;
	Edf *e;

	e = p->edf;
	e->testnext = nil;
	if (qschedulability == nil) {
		qschedulability = p;
		return;
	}
	SET(xp);
	for (xpp = &qschedulability; *xpp; xpp = &xp->edf->testnext) {
		xp = *xpp;
		if (e->testtime < xp->edf->testtime
		|| (e->testtime == xp->edf->testtime && e->testtype < xp->edf->testtype)){
			e->testnext = xp;
			*xpp = p;
			return;
		}
	}
	assert(xp->edf->testnext == nil);
	xp->edf->testnext = p;
}

static char *
testschedulability(Proc *theproc)
{
	Proc *p;
	vlong H, G, Cb, ticks;
	int steps, i;

	/* initialize */
	DPRINT("schedulability test %lud\n", theproc->pid);
	qschedulability = nil;
	for(i=0; i<conf.nproc; i++) {
		p = proctab(i);
		if(p->state == Dead)
			continue;
		if ((p->edf == nil || (p->edf->flags & Admitted) == 0) && p != theproc)
			continue;
		p->edf->testtype = Rl;
		p->edf->testtime = 0;
		DPRINT("\tInit: edfenqueue %lud\n", p->pid);
		testenq(p);
	}
	H=0;
	G=0;
	for(steps = 0; steps < Maxsteps; steps++){
		p = qschedulability;
		qschedulability = p->edf->testnext;
		ticks = p->edf->testtime;
		switch (p->edf->testtype){
		case Dl:
			H += p->edf->C;
			Cb = 0;
			DPRINT("\tStep %3d, Ticks %t, pid %lud, deadline, H += %t → %t, Cb = %t\n",
				steps, ticks, p->pid, p->edf->C, H, Cb);
			if (H+Cb>ticks){
				DPRINT("not schedulable\n");
				return "not schedulable";
			}
			p->edf->testtime += p->edf->T - p->edf->D;
			p->edf->testtype = Rl;
			testenq(p);
			break;
		case Rl:
			DPRINT("\tStep %3d, Ticks %t, pid %lud, release, G  %t, C%t\n",
				steps, ticks, p->pid, p->edf->C, G);
			if(ticks && G <= ticks){
				DPRINT("schedulable\n");
				return nil;
			}
			G += p->edf->C;
			p->edf->testtime += p->edf->D;
			p->edf->testtype = Dl;
			testenq(p);
			break;
		default:
			assert(0);
		}
	}
	DPRINT("probably not schedulable\n");
	return "probably not schedulable";
}
