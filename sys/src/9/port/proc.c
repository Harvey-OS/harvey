/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"../port/edf.h"
#include	"errstr.h"
#include	<trace.h>

extern int nosmp;

enum
{
	Scaling=2,

	AMPmincores = 5,
};

Ref	noteidalloc;

static Ref pidalloc;

static Sched run;


struct Procalloc procalloc;

extern Proc* psalloc(void);
extern void pshash(Proc*);
extern void psrelease(Proc*);
extern void psunhash(Proc*);

static int reprioritize(Proc*);
static void updatecpu(Proc*);

static void rebalance(void);

char *statename[] =
{	/* BUG: generate automatically */
	"Dead",
	"Moribund",
	"Ready",
	"Scheding",
	"Running",
	"Queueing",
	"QueueingR",
	"QueueingW",
	"Wakeme",
	"Broken",
	"Stopped",
	"Rendez",
	"Waitrelease",
	"Exotic",
	"Down",
};

#if 0
void
debuggotolabel(Label *p)
{
	Proc *up = externup();
	if(0)hi("debuggotolabel");
	iprint("gotolabel: pid %p rip %p sp %p\n",
		m && up? up->pid : 0,
		(void *)p->pc,
		(void *)p->sp);
/*
*/
	if (!p->pc)
		die("PC IS ZERO!");
	/* this is an example of putting a breakpoint
	 * here so we can capture a particular process.
	 * startup is very deterministic so this can
	 * be very useful. You can then attach with
	 * gdb and single step. In practice this helped us show
	 * that our return stack for sysrforkret was bogus.
	if (m && up && up->pid == 6)
		die("PID 6\n");
	 */
	gotolabel(p);
}
#endif

Sched*
procsched(Proc *p)
{
	return &run;
}

/*
 * bad planning, once more.
 */
void
procinit0(void)
{
	run.schedgain = 30;

}

/*
 * Always splhi()'ed.
 */
void
schedinit(void)		/* never returns */
{
	Edf *e;

	machp()->inidle = 1;
	machp()->proc = nil;
	ainc(&run.nmach);

	setlabel(&machp()->sched);

	Proc *up = externup();

	if(up) {
		if((e = up->edf) && (e->flags & Admitted))
			edfrecord(up);
		machp()->qstart = 0;
		machp()->qexpired = 0;
		coherence();
		machp()->proc = 0;
		switch(up->state) {
		case Running:
			ready(up);
			break;
		case Moribund:
			up->state = Dead;
			stopac();
			edfstop(up);
			if (up->edf)
				free(up->edf);
			up->edf = nil;

			/*
			 * Holding locks from pexit:
			 * 	procalloc
			 *	pga
			 */
			mmurelease(up);
			unlock(&pga);

			psrelease(up);
			unlock(&procalloc);
			break;
		}
		up->mach = nil;
		updatecpu(up);
		machp()->externup = nil;
	}
	sched();
}

/*
 *  If changing this routine, look also at sleep().  It
 *  contains a copy of the guts of sched().
 */
void
sched(void)
{
	Proc *up = externup();
	Proc *p;

	if(machp()->ilockdepth)
		panic("cpu%d: ilockdepth %d, last lock %#p at %#p, sched called from %#p",
			machp()->machno,
			machp()->ilockdepth,
			up? up->lastilock: nil,
			(up && up->lastilock)? up->lastilock->_pc: 0,
			getcallerpc(&p+2));

	kstackok();
	if(up){
		/*
		 * Delay the sched until the process gives up the locks
		 * it is holding.  This avoids dumb lock loops.
		 * Don't delay if the process is Moribund.
		 * It called sched to die.
		 * But do sched eventually.  This avoids a missing unlock
		 * from hanging the entire kernel.
		 * But don't reschedule procs holding palloc or procalloc.
		 * Those are far too important to be holding while asleep.
		 *
		 * This test is not exact.  There can still be a few
		 * instructions in the middle of taslock when a process
		 * holds a lock but Lock.p has not yet been initialized.
		 */
		if(up->nlocks)
		if(up->state != Moribund)
		if(up->delaysched < 20
		|| pga.Lock.p == up
		|| procalloc.Lock.p == up){
			up->delaysched++;
 			run.delayedscheds++;
			return;
		}
		up->delaysched = 0;

		splhi();
		/* statistics */
		if(up->nqtrap == 0 && up->nqsyscall == 0)
			up->nfullq++;
		machp()->cs++;

		procsave(up);
		mmuflushtlb(machp()->pml4->pa);
		if(setlabel(&up->sched)){
			procrestore(up);
			spllo();
			return;
		}
		/*debug*/gotolabel(&machp()->sched);
	}

	machp()->inidle = 1;
	p = runproc();	/* core 0 never returns */
	machp()->inidle = 0;

	if(!p->edf){
		updatecpu(p);
		p->priority = reprioritize(p);
	}
	if(nosmp){
		if(p != machp()->readied)
			machp()->schedticks = machp()->ticks + HZ/10;
		machp()->readied = 0;
	}
	machp()->externup = p;
	up = p;
	machp()->qstart = machp()->ticks;
	up->nqtrap = 0;
	up->nqsyscall = 0;
	up->state = Running;
	//up->mach = m;
	up->mach = sys->machptr[machp()->machno];
	machp()->proc = up;
//	iprint("up->sched.sp %p * %p\n", up->sched.sp,
//		*(void **) up->sched.sp);
	mmuswitch(up);

	assert(!up->wired || up->wired == machp());
	if (0) hi("gotolabel\n");
	/*debug*/gotolabel(&up->sched);
}

int
anyready(void)
{
	return run.runvec;
}

int
anyhigher(void)
{
	Proc *up = externup();
	return run.runvec & ~((1<<(up->priority+1))-1);
}

/*
 *  here once per clock tick to see if we should resched
 */

void
hzsched(void)
{
	Proc *up = externup();
	/* once a second, rebalance will reprioritize ready procs */
	if(machp()->machno == 0)
		rebalance();

	/* with <= 4 cores, we use SMP and core 0 does not set qexpired for us */
	//if(sys->nmach <= AMPmincores)
	if(machp()->ticks - machp()->qstart >= HZ/100)
		machp()->qexpired = 1;

	/* unless preempted, get to run */
	if(machp()->qexpired && anyready())
		up->delaysched++;
}

#if 0
void
hzsched(void)
{
	Proc *up = externup();
	/* once a second, rebalance will reprioritize ready procs */
	if(machp()->machno == 0)
		rebalance();

	/* unless preempted, get to run for at least 100ms */
	if(anyhigher()
	|| (!up->fixedpri && machp()->ticks > m->schedticks && anyready())){
		m->readied = nil;	/* avoid cooperative scheduling */
		up->delaysched++;
	}
}
#endif

/*
 *  here at the end of non-clock interrupts to see if we should preempt the
 *  current process.  Returns 1 if preempted, 0 otherwise.
 */

int
preempted(void)
{
	Proc *up = externup();
	if(up && up->state == Running)
	if(up->preempted == 0)
	if(anyhigher())
	if(!active.exiting){
		/*  Core 0 is dispatching all interrupts, so no core
		 *  actually running a user process is ever going call preempted, unless
		 *  we consider IPIs for preemption or we distribute interrupts.
		 *  But we are going to use SMP for machines with few cores.
		panic("preemted used");
		 */

		up->preempted = 1;
		sched();
		splhi();
		up->preempted = 0;
		return 1;
	}
	return 0;
}

#if 0
int
preempted(void)
{
	Proc *up = externup();
	if(up && up->state == Running)
	if(up->preempted == 0)
	if(anyhigher())
	if(!active.exiting){
		m->readied = nil;	/* avoid cooperative scheduling */
		up->preempted = 1;
		sched();
		splhi();
		up->preempted = 0;
		return 1;
	}
	return 0;
}
#endif

/*
 * Update the cpu time average for this particular process,
 * which is about to change from up -> not up or vice versa.
 * p->lastupdate is the last time an updatecpu happened.
 *
 * The cpu time average is a decaying average that lasts
 * about D clock ticks.  D is chosen to be approximately
 * the cpu time of a cpu-intensive "quick job".  A job has to run
 * for approximately D clock ticks before we home in on its
 * actual cpu usage.  Thus if you manage to get in and get out
 * quickly, you won't be penalized during your burst.  Once you
 * start using your share of the cpu for more than about D
 * clock ticks though, your p->cpu hits 1000 (1.0) and you end up
 * below all the other quick jobs.  Interactive tasks, because
 * they basically always use less than their fair share of cpu,
 * will be rewarded.
 *
 * If the process has not been running, then we want to
 * apply the filter
 *
 *	cpu = cpu * (D-1)/D
 *
 * n times, yielding
 *
 *	cpu = cpu * ((D-1)/D)^n
 *
 * but D is big enough that this is approximately
 *
 * 	cpu = cpu * (D-n)/D
 *
 * so we use that instead.
 *
 * If the process has been running, we apply the filter to
 * 1 - cpu, yielding a similar equation.  Note that cpu is
 * stored in fixed point (* 1000).
 *
 * Updatecpu must be called before changing up, in order
 * to maintain accurate cpu usage statistics.  It can be called
 * at any time to bring the stats for a given proc up-to-date.
 */
static void
updatecpu(Proc *p)
{
	Proc *up = externup();
	int D, n, t, ocpu;

	if(p->edf)
		return;

	//t = sys->ticks*Scaling + Scaling/2;
	t = sys->machptr[0]->ticks*Scaling + Scaling/2; //Originally MACHP(0)
	n = t - p->lastupdate;
	p->lastupdate = t;

	if(n == 0)
		return;
	D = run.schedgain*HZ*Scaling;
	if(n > D)
		n = D;

	ocpu = p->cpu;
	if(p != up)
		p->cpu = (ocpu*(D-n))/D;
	else{
		t = 1000 - ocpu;
		t = (t*(D-n))/D;
		p->cpu = 1000 - t;
	}

//iprint("pid %d %s for %d cpu %d -> %d\n", p->pid,p==up?"active":"inactive",n, ocpu,p->cpu);
}

/*
 * On average, p has used p->cpu of a cpu recently.
 * Its fair share is nmach/m->load of a cpu.  If it has been getting
 * too much, penalize it.  If it has been getting not enough, reward it.
 * I don't think you can get much more than your fair share that
 * often, so most of the queues are for using less.  Having a priority
 * of 3 means you're just right.  Having a higher priority (up to p->basepri)
 * means you're not using as much as you could.
 */
static int
reprioritize(Proc *p)
{
	int fairshare, n, load, ratio;

	load = sys->load;
	if(load == 0)
		return p->basepri;

	/*
	 *  fairshare = 1.000 * conf.nproc * 1.000/load,
	 * except the decimal point is moved three places
	 * on both load and fairshare.
	 */
	fairshare = (sys->nmach*1000*1000)/load;
	n = p->cpu;
	if(n == 0)
		n = 1;
	ratio = (fairshare+n/2) / n;
	if(ratio > p->basepri)
		ratio = p->basepri;
	if(ratio < 0)
		panic("reprioritize");
//iprint("pid %d cpu %d load %d fair %d pri %d\n", p->pid, p->cpu, load, fairshare, ratio);
	return ratio;
}

/*
 * add a process to a scheduling queue
 */
static void
queueproc(Sched *sch, Schedq *rq, Proc *p, int locked)
{
	int pri;

	pri = rq - sch->runq;
	if(!locked)
		lock(sch);
	else if(canlock(sch))
		panic("queueproc: locked and can lock");
	p->priority = pri;
	p->rnext = 0;
	if(rq->tail)
		rq->tail->rnext = p;
	else
		rq->head = p;
	rq->tail = p;
	rq->n++;
	sch->nrdy++;
	sch->runvec |= 1<<pri;
	if(!locked)
		unlock(sch);
}

/*
 *  try to remove a process from a scheduling queue (called splhi)
 */
Proc*
dequeueproc(Sched *sch, Schedq *rq, Proc *tp)
{
	Proc *l, *p;

	if(!canlock(sch))
		return nil;

	/*
	 *  the queue may have changed before we locked runq,
	 *  refind the target process.
	 */
	l = 0;
	for(p = rq->head; p; p = p->rnext){
		if(p == tp)
			break;
		l = p;
	}

	/*
	 *  p->mach==0 only when process state is saved
	 */

	if(p == 0 || p->mach){
		unlock(sch);
		return nil;
	}
	if(p->rnext == 0)
		rq->tail = l;
	if(l)
		l->rnext = p->rnext;
	else
		rq->head = p->rnext;
	if(rq->head == nil)
		sch->runvec &= ~(1<<(rq-sch->runq));
	rq->n--;
	sch->nrdy--;
	if(p->state != Ready)
		print("dequeueproc %s %d %s\n", p->text, p->pid, statename[p->state]);

	unlock(sch);
	return p;
}

static void
schedready(Sched *sch, Proc *p, int locked)
{
	Mpl pl;
	int pri;
	Schedq *rq;

	pl = splhi();
	if(edfready(p)){
		splx(pl);
		return;
	}

/*	if(up != p)
		m->readied = p;	*//* group scheduling, will be removed */

	updatecpu(p);
	pri = reprioritize(p);
	p->priority = pri;
	rq = &sch->runq[pri];
	p->state = Ready;
	queueproc(sch, rq, p, locked);
	if(p->trace)
		proctrace(p, SReady, 0);
	splx(pl);
}

/*
 *  ready(p) picks a new priority for a process and sticks it in the
 *  runq for that priority.
 */
void
ready(Proc *p)
{
	schedready(procsched(p), p, 0);
}

/*
 *  yield the processor and drop our priority
 */
void
yield(void)
{
	Proc *up = externup();
	if(anyready()){
		/* pretend we just used 1/2 tick */
		up->lastupdate -= Scaling/2;
		sched();
	}
}

/*
 *  recalculate priorities once a second.  We need to do this
 *  since priorities will otherwise only be recalculated when
 *  the running process blocks.
 */
static void
rebalance(void)
{
	Mpl pl;
	int pri, npri, t;
	Schedq *rq;
	Proc *p;

	t = machp()->ticks;
	if(t - run.balancetime < HZ)
		return;
	run.balancetime = t;

	for(pri=0, rq=run.runq; pri<Npriq; pri++, rq++){
another:
		p = rq->head;
		if(p == nil)
			continue;
		if(p->mp != sys->machptr[machp()->machno]) //MACHP(machp()->machno)
			continue;
		if(pri == p->basepri)
			continue;
		updatecpu(p);
		npri = reprioritize(p);
		if(npri != pri){
			pl = splhi();
			p = dequeueproc(&run, rq, p);
			if(p)
				queueproc(&run, &run.runq[npri], p, 0);
			splx(pl);
			goto another;
		}
	}
}

/*
 * Process p is ready to run, but there's no available core.
 * Try to make a core available by
 * 1. preempting a process with lower priority, or
 * 2. preempting one with the same priority that had more than HZ/10, or
 * 3. rescheduling one that run more than HZ, in the hope he gets his priority lowered.
 */
static void
preemptfor(Proc *p)
{
	Proc *up = externup();
	uint32_t delta;
	uint i, /*j,*/ rr;
	Proc *mup;
	Mach *mp;

	assert(machp()->machno == 0);
	/*
	 * try to preempt a lower priority process first, default back to
	 * round robin otherwise.
	 */
	for(rr = 0; rr < 2; rr++)
		for(i = 0; i < MACHMAX; i++){
			/*j = pickcore(p->color, i);
			if((mp = sys->machptr[j]) != nil && mp->online && mp->nixtype == NIXTC){*/
			if((mp = sys->machptr[i]) != nil && mp->online && mp->nixtype == NIXTC){
				if(mp == machp())
					continue;
				/*
				 * Caution here: mp->proc can change, even die.
				 */
				mup = mp->proc;
				if(mup == nil)		/* one got idle */
					return;
				delta = mp->ticks - mp->qstart;
				if(up->priority < p->priority){
					mp->qexpired = 1;
					return;
				}
				if(rr && up->priority == p->priority && delta > HZ/10){
					mp->qexpired = 1;
					return;
				}
				if(rr & delta > HZ){
					mp->qexpired = 1;
					return;
				}
			}
	}
}

/*
 * Scheduling thread run as the main loop of cpu 0
 * Used in AMP sched.
 */
static void
mach0sched(void)
{
	Schedq *rq;
	Proc *p;
	Mach *mp;
	uint32_t start, now;
	int n, i; //, j;

	assert(machp()->machno == 0);
	acmodeset(NIXKC);		/* we don't time share any more */
	n = 0;
	start = perfticks();
loop:

	/*
	 * find a ready process that we might run.
	 */
	spllo();
	for(rq = &run.runq[Nrq-1]; rq >= run.runq; rq--)
		for(p = rq->head; p; p = p->rnext){
			/*
			 * wired processes may only run when their core is available.
			 */
			if(p->wired != nil){
				if(p->wired->proc == nil)
					goto found;
				continue;
			}
			/*
			 * find a ready process that did run at an available core
			 * or one that has not moved for some time.
			 */
			if(p->mp == nil || p->mp->proc == nil || n>0){
				goto found;
			}
		}
	/* waste time or halt the CPU */
	idlehands();
	/* remember how much time we're here */
	now = perfticks();
	machp()->perf.inidle += now-start;
	start = now;
	n++;
	goto loop;

found:
	assert(machp()->machno == 0);
	splhi();
	/*
	 * find a core for this process, but honor wiring.
	 */
	mp = p->wired;
	if(mp != nil){
		if(mp->proc != nil)
			goto loop;
	}else{
		for(i = 0; i < MACHMAX; i++){
			/*j = pickcore(p->color, i);
			if((mp = sys->machptr[j]) != nil && mp->online && mp->nixtype == NIXTC){*/
			if((mp = sys->machptr[i]) != nil){ // && mp->online && mp->nixtype == NIXTC){
				if(mp != machp() && mp->proc == nil)
					break;
			}
		}
		if(i == MACHMAX){
			preemptfor(p);
			goto loop;
		}
	}

	p = dequeueproc(&run, rq, p);
	mp->proc = p;
	if(p != nil){
		p->state = Scheding;
		p->mp = mp;
	}

	n = 0;
	goto loop;
}

/*
 * SMP performs better than AMP with few cores.
 * So, leave this here by now. We should probably
 * write a unified version of runproc good enough for
 * both SMP and AMP.
 */
static Proc*
smprunproc(void)
{
	Schedq *rq;
	Proc *p;
	uint32_t start, now;
	int i;

	start = perfticks();
	run.preempts++;

loop:
	/*
	 *  find a process that last ran on this processor (affinity),
	 *  or one that hasn't moved in a while (load balancing).  Every
	 *  time around the loop affinity goes down.
	 */
	spllo();
	for(i = 0;; i++){
		/*
		 *  find the highest priority target process that this
		 *  processor can run given affinity constraints.
		 *
		 */
		for(rq = &run.runq[Nrq-1]; rq >= run.runq; rq--){
			for(p = rq->head; p; p = p->rnext){
				if(p->mp == nil || p->mp == sys->machptr[machp()->machno]
				|| (!p->wired && i > 0))
					goto found;
			}
		}

		/* waste time or halt the CPU */
		idlehands();
		/* remember how much time we're here */
		now = perfticks();
		machp()->perf.inidle += now-start;
		start = now;
	}

found:
	splhi();
	p = dequeueproc(&run, rq, p);
	if(p == nil)
		goto loop;

	p->state = Scheding;
	p->mp = sys->machptr[machp()->machno];

	if(edflock(p)){
		edfrun(p, rq == &run.runq[PriEdf]);	/* start deadline timer and do admin */
		edfunlock();
	}
	if(p->trace)
		proctrace(p, SRun, 0);
	return p;
}

/*
 * It's possible to force to single core even
 * in a multiprocessor machine
 */

static Proc*
singlerunproc(void)
{
	Schedq *rq;
	Proc *p;
	uint32_t start, now, skipscheds;
	int i;

	start = perfticks();

	/* cooperative scheduling until the clock ticks */
	if((p=machp()->readied) && p->mach==0 && p->state==Ready
	&& &run.runq[Nrq-1].head == nil && &run.runq[Nrq-2].head == nil){
		skipscheds++;
		rq = &run.runq[p->priority];
		if(0)hi("runproc going to found before loop...\n");
		goto found;
	}

	run.preempts++;

loop:
	/*
	 *  find a process that last ran on this processor (affinity),
	 *  or one that hasn't moved in a while (load balancing).  Every
	 *  time around the loop affinity goes down.
	 */
	spllo();
	for(i = 0;; i++){
		/*
		 *  find the highest priority target process that this
		 *  processor can run given affinity constraints.
		 *
		 */
		for(rq = &run.runq[Nrq-1]; rq >= run.runq; rq--){
			for(p = rq->head; p; p = p->rnext){
				if(p->mp == nil || p->mp == sys->machptr[machp()->machno]
				|| (!p->wired && i > 0))
				{
					if(0)hi("runproc going to found inside loop...\n");
					goto found;
				}
			}
		}

		/* waste time or halt the CPU */
		idlehands();

		/* remember how much time we're here */
		now = perfticks();
		machp()->perf.inidle += now-start;
		start = now;
	}

found:
	splhi();
	if(0)hi("runproc into found...\n");
	p = dequeueproc(&run, rq, p);
	if(p == nil)
	{
		if(0)hi("runproc p=nil :(\n");
			goto loop;
	}

	p->state = Scheding;
	if(0)hi("runproc, pm->mp = sys->machptr[machp()->machno]\n");
	p->mp = sys->machptr[machp()->machno];
	if(0){hi("runproc, sys->machptr[machp()->machno] = "); put64((uint64_t)p->mp); hi("\n");}

	if(edflock(p)){
		edfrun(p, rq == &run.runq[PriEdf]);	/* start deadline timer and do admin */
		edfunlock();
	}
	if(p->trace)
		proctrace(p, SRun, 0);
	/* avoiding warnings, this will be removed */
	USED(mach0sched); USED(smprunproc);
	if(0){hi("runproc, returning p ");
	put64((uint64_t)p);
	hi("\n");}


	return p;
}

/*
 *  pick a process to run.
 *  most of this is used in AMP sched.
 *  (on a quad core or less, we use SMP).
 *  In the case of core 0 we always return nil, but
 *  schedule the picked process at any other available TC.
 *  In the case of other cores we wait until a process is given
 *  by core 0.
 */

Proc*
runproc(void)
{
	Schedq *rq;
	Proc *p;
	uint32_t start, now;

	if(nosmp)
		return singlerunproc();

	//NIX modeset cannot work without halt every cpu at boot
	//if(sys->nmach <= AMPmincores)
	else
		return smprunproc();

	start = perfticks();
	run.preempts++;
	rq = nil;
	if(machp()->machno != 0){
		do{
			spllo();
			while(machp()->proc == nil)
				idlehands();
			now = perfticks();
			machp()->perf.inidle += now-start;
			start = now;
			splhi();
			p = machp()->proc;
		}while(p == nil);
		p->state = Scheding;
		p->mp = sys->machptr[machp()->machno];

		if(edflock(p)){
			edfrun(p, rq == &run.runq[PriEdf]);	/* start deadline timer and do admin */
			edfunlock();
		}
		if(p->trace)
			proctrace(p, SRun, 0);
		return p;
	}

	mach0sched();
	return nil;	/* not reached */
}

int
canpage(Proc *p)
{
	int ok;
	Sched *sch;

	splhi();
	sch = procsched(p);
	lock(sch);
	/* Only reliable way to see if we are Running */
	if(p->mach == 0) {
		p->newtlb = 1;
		ok = 1;
	}
	else
		ok = 0;
	unlock(sch);
	spllo();

	return ok;
}

Proc*
newproc(void)
{
	Proc *up = externup();
	Proc *p;

	p = psalloc();

	p->state = Scheding;
	p->psstate = "New";
	p->mach = 0;
	p->qnext = 0;
	p->nchild = 0;
	p->nwait = 0;
	p->waitq = 0;
	p->parent = 0;
	p->pgrp = 0;
	p->egrp = 0;
	p->fgrp = 0;
	p->rgrp = 0;
	p->pdbg = 0;
	p->kp = 0;
	if(up != nil && up->procctl == Proc_tracesyscall)
		p->procctl = Proc_tracesyscall;
	else
		p->procctl = 0;
	p->syscalltrace = nil;
	p->notepending = 0;
	p->ureg = 0;
	p->privatemem = 0;
	p->noswap = 0;
	p->errstr = p->errbuf0;
	p->syserrstr = p->errbuf1;
	p->errbuf0[0] = '\0';
	p->errbuf1[0] = '\0';
	p->nlocks = 0;
	p->delaysched = 0;
	p->trace = 0;
	kstrdup(&p->user, "*nouser");
	kstrdup(&p->text, "*notext");
	kstrdup(&p->args, "");
	p->nargs = 0;
	p->setargs = 0;
	memset(p->seg, 0, sizeof p->seg);
	p->pid = incref(&pidalloc);
	pshash(p);
	p->noteid = incref(&noteidalloc);
	if(p->pid <= 0 || p->noteid <= 0)
		panic("pidalloc");
	if(p->kstack == 0){
		p->kstack = smalloc(KSTACK);
		*(uintptr_t*)p->kstack = STACKGUARD;
	}

	/* sched params */
	p->mp = 0;
	p->wired = 0;
	procpriority(p, PriNormal, 0);
	p->cpu = 0;
	p->lastupdate = sys->ticks*Scaling;
	p->edf = nil;

	p->ntrap = 0;
	p->nintr = 0;
	p->nsyscall = 0;
	p->nactrap = 0;
	p->nacsyscall = 0;
	p->nicc = 0;
	p->actime = 0ULL;
	p->tctime = 0ULL;
	p->ac = nil;
	p->nfullq = 0;
	p->req = nil;
	p->resp = nil;
	memset(&p->PMMU, 0, sizeof p->PMMU);
	return p;
}

/*
 * wire this proc to a machine
 */
void
procwired(Proc *p, int bm)
{
	Proc *up = externup();
	Proc *pp;
	int i;
	char nwired[MACHMAX];
	Mach *wm;

	if(bm < 0){
		/* pick a machine to wire to */
		memset(nwired, 0, sizeof(nwired));
		p->wired = 0;
		for(i=0; (pp = psincref(i)) != nil; i++){
			wm = pp->wired;
			if(wm && pp->pid)
				nwired[wm->machno]++;
			psdecref(pp);
		}
		bm = 0;
		for(i=0; i<sys->nmach; i++)
			if(nwired[i] < nwired[bm])
				bm = i;
	} else {
		/* use the virtual machine requested */
		bm = bm % sys->nmach;
	}

	p->wired = sys->machptr[bm];
	p->mp = p->wired;

	/*
	 * adjust our color to the new domain.
	 */
	if(up == nil || p != up)
		return;
	up->color = corecolor(up->mp->machno);
	qlock(&up->seglock);
	for(i = 0; i < NSEG; i++)
		if(up->seg[i])
			up->seg[i]->color = up->color;
	qunlock(&up->seglock);
}

void
procpriority(Proc *p, int pri, int fixed)
{
	if(pri >= Npriq)
		pri = Npriq - 1;
	else if(pri < 0)
		pri = 0;
	p->basepri = pri;
	p->priority = pri;
	if(fixed){
		p->fixedpri = 1;
	} else {
		p->fixedpri = 0;
	}
}

/*
 *  sleep if a condition is not true.  Another process will
 *  awaken us after it sets the condition.  When we awaken
 *  the condition may no longer be true.
 *
 *  we lock both the process and the rendezvous to keep r->p
 *  and p->r synchronized.
 */
void
sleep(Rendez *r, int (*f)(void*), void *arg)
{
	Proc *up = externup();
	Mpl pl;

	pl = splhi();

	if(up->nlocks)
		print("process %d sleeps with %d locks held, last lock %#p locked at pc %#p, sleep called from %#p\n",
			up->pid, up->nlocks, up->lastlock, up->lastlock->_pc, getcallerpc(&r));
	lock(r);
	lock(&up->rlock);
	if(r->_p){
		print("double sleep called from %#p, %d %d\n",
			getcallerpc(&r), r->_p->pid, up->pid);
		dumpstack();
	}

	/*
	 *  Wakeup only knows there may be something to do by testing
	 *  r->p in order to get something to lock on.
	 *  Flush that information out to memory in case the sleep is
	 *  committed.
	 */
	r->_p = up;

	if((*f)(arg) || up->notepending){
		/*
		 *  if condition happened or a note is pending
		 *  never mind
		 */
		r->_p = nil;
		unlock(&up->rlock);
		unlock(r);
	} else {
		/*
		 *  now we are committed to
		 *  change state and call scheduler
		 */
		if(up->trace)
			proctrace(up, SSleep, 0);
		up->state = Wakeme;
		up->r = r;

		/* statistics */
		machp()->cs++;

		procsave(up);
		mmuflushtlb(machp()->pml4->pa);
		if(setlabel(&up->sched)) {
			/*
			 *  here when the process is awakened
			 */
			procrestore(up);
			spllo();
		} else {
			/*
			 *  here to go to sleep (i.e. stop Running)
			 */
			unlock(&up->rlock);
			unlock(r);
			/*debug*/gotolabel(&machp()->sched);
		}
	}

	if(up->notepending) {
		up->notepending = 0;
		splx(pl);
		if(up->procctl == Proc_exitme && up->closingfgrp)
			forceclosefgrp();
		error(Eintr);
	}

	splx(pl);
}

static int
tfn(void *arg)
{
	Proc *up = externup();
	return up->trend == nil || up->tfn(arg);
}

void
twakeup(Ureg* ureg, Timer *t)
{
	Proc *p;
	Rendez *trend;

	p = t->ta;
	trend = p->trend;
	p->trend = 0;
	if(trend)
		wakeup(trend);
}

void
tsleep(Rendez *r, int (*fn)(void*), void *arg, int32_t ms)
{
	Proc *up = externup();
	if (up->tt){
		print("tsleep: timer active: mode %d, tf %#p\n",
			up->tmode, up->tf);
		timerdel(up);
	}
	up->tns = MS2NS(ms);
	up->tf = twakeup;
	up->tmode = Trelative;
	up->ta = up;
	up->trend = r;
	up->tfn = fn;
	timeradd(up);

	if(waserror()){
		timerdel(up);
		nexterror();
	}
	sleep(r, tfn, arg);
	if (up->tt)
		timerdel(up);
	up->twhen = 0;
	poperror();
}

/*
 *  Expects that only one process can call wakeup for any given Rendez.
 *  We hold both locks to ensure that r->p and p->r remain consistent.
 *  Richard Miller has a better solution that doesn't require both to
 *  be held simultaneously, but I'm a paranoid - presotto.
 */
Proc*
wakeup(Rendez *r)
{
	Mpl pl;
	Proc *p;

	pl = splhi();

	lock(r);
	p = r->_p;

	if(p != nil){
		lock(&p->rlock);
		if(p->state != Wakeme || p->r != r)
			panic("wakeup: state");
		r->_p = nil;
		p->r = nil;
		ready(p);
		unlock(&p->rlock);
	}
	unlock(r);

	splx(pl);

	return p;
}

/*
 *  if waking a sleeping process, this routine must hold both
 *  p->rlock and r->lock.  However, it can't know them in
 *  the same order as wakeup causing a possible lock ordering
 *  deadlock.  We break the deadlock by giving up the p->rlock
 *  lock if we can't get the r->lock and retrying.
 */
int
postnote(Proc *p, int dolock, char *n, int flag)
{
	Mpl pl;
	int ret;
	Rendez *r;
	Proc *d, **l;

	if(dolock)
		qlock(&p->debug);

	if(flag != NUser && (p->notify == 0 || p->notified))
		p->nnote = 0;

	ret = 0;
	if(p->nnote < NNOTE) {
		strcpy(p->note[p->nnote].msg, n);
		p->note[p->nnote++].flag = flag;
		ret = 1;
	}
	p->notepending = 1;

	/* NIX  */
	if(p->state == Exotic){
		/* it could be that the process is not running
		 * in the AC when we interrupt the AC, but then
		 * we'd only get an extra interrupt in the AC, and
		 * nothing should happen.
		 */
		intrac(p);
	}

	if(dolock)
		qunlock(&p->debug);

	/* this loop is to avoid lock ordering problems. */
	for(;;){
		pl = splhi();
		lock(&p->rlock);
		r = p->r;

		/* waiting for a wakeup? */
		if(r == nil)
			break;	/* no */

		/* try for the second lock */
		if(canlock(r)){
			if(p->state != Wakeme || r->_p != p)
				panic("postnote: state %d %d %d", r->_p != p, p->r != r, p->state);
			p->r = nil;
			r->_p = nil;
			ready(p);
			unlock(r);
			break;
		}

		/* give other process time to get out of critical section and try again */
		unlock(&p->rlock);
		splx(pl);
		sched();
	}
	unlock(&p->rlock);
	splx(pl);

	if(p->state != Rendezvous){
		if(p->state == Semdown)
			ready(p);
		return ret;
	}
	/* Try and pull out of a rendezvous */
	lock(p->rgrp);
	if(p->state == Rendezvous) {
		p->rendval = ~0;
		l = &REND(p->rgrp, p->rendtag);
		for(d = *l; d; d = d->rendhash) {
			if(d == p) {
				*l = p->rendhash;
				break;
			}
			l = &d->rendhash;
		}
		ready(p);
	}
	unlock(p->rgrp);
	return ret;
}

/*
 * weird thing: keep at most NBROKEN around
 */
#define	NBROKEN 4
struct
{
	QLock;
	int	n;
	Proc	*p[NBROKEN];
}broken;

void
addbroken(Proc *p)
{
	Proc *up = externup();
	qlock(&broken);
	if(broken.n == NBROKEN) {
		ready(broken.p[0]);
		memmove(&broken.p[0], &broken.p[1], sizeof(Proc*)*(NBROKEN-1));
		--broken.n;
	}
	broken.p[broken.n++] = p;
	qunlock(&broken);

	stopac();
	edfstop(up);
	p->state = Broken;
	p->psstate = 0;
	sched();
}

void
unbreak(Proc *p)
{
	int b;

	qlock(&broken);
	for(b=0; b < broken.n; b++)
		if(broken.p[b] == p) {
			broken.n--;
			memmove(&broken.p[b], &broken.p[b+1],
					sizeof(Proc*)*(NBROKEN-(b+1)));
			ready(p);
			break;
		}
	qunlock(&broken);
}

int
freebroken(void)
{
	int i, n;

	qlock(&broken);
	n = broken.n;
	for(i=0; i<n; i++) {
		ready(broken.p[i]);
		broken.p[i] = 0;
	}
	broken.n = 0;
	qunlock(&broken);
	return n;
}

void
pexit(char *exitstr, int freemem)
{
	Proc *up = externup();
	Proc *p;
	Segment **s, **es;
	int32_t utime, stime;
	Waitq *wq, *f, *next;
	Fgrp *fgrp;
	Egrp *egrp;
	Rgrp *rgrp;
	Pgrp *pgrp;
	Chan *dot;

	if(0 && up->nfullq > 0)
		iprint(" %s=%d", up->text, up->nfullq);
	if(0 && up->nicc > 0)
		iprint(" [%s nicc %ud tctime %ulld actime %ulld]\n",
			up->text, up->nicc, up->tctime, up->actime);
	if(up->syscalltrace != nil)
		free(up->syscalltrace);
	up->syscalltrace = nil;
	up->alarm = 0;

	if (up->tt)
		timerdel(up);
	if(up->trace)
		proctrace(up, SDead, 0);

	/* nil out all the resources under lock (free later) */
	qlock(&up->debug);
	fgrp = up->fgrp;
	up->fgrp = nil;
	egrp = up->egrp;
	up->egrp = nil;
	rgrp = up->rgrp;
	up->rgrp = nil;
	pgrp = up->pgrp;
	up->pgrp = nil;
	dot = up->dot;
	up->dot = nil;
	qunlock(&up->debug);


	if(fgrp)
		closefgrp(fgrp);
	if(egrp)
		closeegrp(egrp);
	if(rgrp)
		closergrp(rgrp);
	if(dot)
		cclose(dot);
	if(pgrp)
		closepgrp(pgrp);

	/*
	 * if not a kernel process and have a parent,
	 * do some housekeeping.
	 */
	if(up->kp == 0) {
		p = up->parent;
		if(p == 0) {
			if(exitstr == 0)
				exitstr = "unknown";
			//die("bootprocessdeath");
			panic("boot process died: %s", exitstr);
		}

		while(waserror())
			;

		wq = smalloc(sizeof(Waitq));
		poperror();

		wq->w.pid = up->pid;
		utime = up->time[TUser] + up->time[TCUser];
		stime = up->time[TSys] + up->time[TCSys];
		wq->w.time[TUser] = tk2ms(utime);
		wq->w.time[TSys] = tk2ms(stime);
		wq->w.time[TReal] = tk2ms(sys->machptr[0]->ticks - up->time[TReal]);
		if(exitstr && exitstr[0])
			snprint(wq->w.msg, sizeof(wq->w.msg), "%s %d: %s",
				up->text, up->pid, exitstr);
		else
			wq->w.msg[0] = '\0';

		lock(&p->exl);
		/*
		 * Check that parent is still alive.
		 */
		if(p->pid == up->parentpid && p->state != Broken) {
			p->nchild--;
			p->time[TCUser] += utime;
			p->time[TCSys] += stime;
			/*
			 * If there would be more than 128 wait records
			 * processes for my parent, then don't leave a wait
			 * record behind.  This helps prevent badly written
			 * daemon processes from accumulating lots of wait
			 * records.
		 	 */
			if(p->nwait < 128) {
				wq->next = p->waitq;
				p->waitq = wq;
				p->nwait++;
				wq = nil;
				wakeup(&p->waitr);
			}
		}
		unlock(&p->exl);
		if(wq)
			free(wq);
	}

	if(!freemem)
		addbroken(up);

	qlock(&up->seglock);
	es = &up->seg[NSEG];
	for(s = up->seg; s < es; s++) {
		if(*s) {
			putseg(*s);
			*s = 0;
		}
	}
	qunlock(&up->seglock);

	lock(&up->exl);		/* Prevent my children from leaving waits */
	psunhash(up);
	up->pid = 0;
	wakeup(&up->waitr);
	unlock(&up->exl);

	for(f = up->waitq; f; f = next) {
		next = f->next;
		free(f);
	}

	/* release debuggers */
	qlock(&up->debug);
	if(up->pdbg) {
		wakeup(&up->pdbg->sleep);
		up->pdbg = 0;
	}
	qunlock(&up->debug);

	/* Sched must not loop for these locks */
	lock(&procalloc);
	lock(&pga);

	stopac();
	edfstop(up);
	up->state = Moribund;
	sched();
	panic("pexit");
}

int
haswaitq(void *x)
{
	Proc *p;

	p = (Proc *)x;
	return p->waitq != 0;
}

int
pwait(Waitmsg *w)
{
	Proc *up = externup();
	int cpid;
	Waitq *wq;

	if(!canqlock(&up->qwaitr))
		error(Einuse);

	if(waserror()) {
		qunlock(&up->qwaitr);
		nexterror();
	}

	lock(&up->exl);
	if(up->nchild == 0 && up->waitq == 0) {
		unlock(&up->exl);
		error(Enochild);
	}
	unlock(&up->exl);

	sleep(&up->waitr, haswaitq, up);

	lock(&up->exl);
	wq = up->waitq;
	up->waitq = wq->next;
	up->nwait--;
	unlock(&up->exl);

	qunlock(&up->qwaitr);
	poperror();

	if(w)
		memmove(w, &wq->w, sizeof(Waitmsg));
	cpid = wq->w.pid;
	free(wq);

	return cpid;
}

void
dumpaproc(Proc *p)
{
	uintptr_t bss;
	char *s;
	int sno;

	if(p == 0)
		return;

	bss = 0;
	for(sno = 0; sno < NSEG; sno++)
		if(p->seg[sno] != nil && (p->seg[sno]->type & SG_TYPE) == SG_BSS)
			bss = p->seg[sno]->top;

	s = p->psstate;
	if(s == 0)
		s = statename[p->state];
	print("%3d:%10s pc %#p dbgpc %#p  %8s (%s) ut %ld st %ld bss %#p qpc %#p nl %d nd %lud lpc %#p pri %lud\n",
		p->pid, p->text, p->pc, dbgpc(p), s, statename[p->state],
		p->time[0], p->time[1], bss, p->qpc, p->nlocks,
		p->delaysched, p->lastlock ? p->lastlock->_pc : 0, p->priority);
}

void
procdump(void)
{
	Proc *up = externup();
	int i;
	Proc *p;

	if(up)
		print("up %d\n", up->pid);
	else
		print("no current process\n");
	for(i=0; (p = psincref(i)) != nil; i++) {
		if(p->state != Dead)
			dumpaproc(p);
		psdecref(p);
	}
}

/*
 *  wait till all processes have flushed their mmu
 *  state about segement s
 */
void
procflushseg(Segment *s)
{
	int i, ns, nm, nwait;
	Proc *p;
	Mach *mp;

	/*
	 *  tell all processes with this
	 *  segment to flush their mmu's
	 */
	nwait = 0;
	for(i=0; (p = psincref(i)) != nil; i++) {
		if(p->state == Dead){
			psdecref(p);
			continue;
		}
		for(ns = 0; ns < NSEG; ns++){
			if(p->seg[ns] == s){
				p->newtlb = 1;
				for(nm = 0; nm < MACHMAX; nm++)
					if((mp = sys->machptr[nm]) != nil && mp->online)
						if(mp->proc == p){
							mp->mmuflush = 1;
							nwait++;
						}
				break;
			}
		}
		psdecref(p);
	}

	if(nwait == 0)
		return;

	/*
	 *  wait for all processors to take a clock interrupt
	 *  and flush their mmu's.
	 *  NIX BUG: this won't work if another core is in AC mode.
	 *  In that case we must IPI it, but only if that core is
	 *  using this segment.
	 */
	for(i = 0; i < MACHMAX; i++)
		if((mp = sys->machptr[i]) != nil && mp->online)
			if(mp != machp())
				while(mp->mmuflush)
					sched();
}

void
scheddump(void)
{
	Proc *p;
	Schedq *rq;

	for(rq = &run.runq[Nrq-1]; rq >= run.runq; rq--){
		if(rq->head == 0)
			continue;
		print("run[%ld]:", rq-run.runq);
		for(p = rq->head; p; p = p->rnext)
			print(" %d(%lud)", p->pid, machp()->ticks - p->readytime);
		print("\n");
		delay(150);
	}
	print("nrdy %d\n", run.nrdy);
}

void
kproc(char *name, void (*func)(void *), void *arg)
{
	Proc *up = externup();
	Proc *p;
	static Pgrp *kpgrp;

	p = newproc();
	p->psstate = 0;
	p->procmode = 0640;
	p->kp = 1;
	p->noswap = 1;

	p->scallnr = up->scallnr;
	memmove(p->arg, up->arg, sizeof(up->arg));
	p->nerrlab = 0;
	p->slash = up->slash;
	p->dot = up->dot;
	if(p->dot)
		incref(p->dot);

	memmove(p->note, up->note, sizeof(p->note));
	p->nnote = up->nnote;
	p->notified = 0;
	p->lastnote = up->lastnote;
	p->notify = up->notify;
	p->ureg = 0;
	p->dbgreg = 0;

	procpriority(p, PriKproc, 0);

	kprocchild(p, func, arg);

	kstrdup(&p->user, eve);
	kstrdup(&p->text, name);
	if(kpgrp == 0)
		kpgrp = newpgrp();
	p->pgrp = kpgrp;
	incref(kpgrp);

	memset(p->time, 0, sizeof(p->time));
	p->time[TReal] = sys->ticks;
	ready(p);
	/*
	 *  since the bss/data segments are now shareable,
	 *  any mmu info about this process is now stale
	 *  and has to be discarded.
	 */
	p->newtlb = 1;
	mmuflush();
}

/*
 *  called splhi() by notify().  See comment in notify for the
 *  reasoning.
 */
void
procctl(Proc *p)
{
	Proc *up = externup();
	Mpl pl;
	char *state;

	switch(p->procctl) {
	case Proc_exitbig:
		spllo();
		pexit("Killed: Insufficient physical memory", 1);

	case Proc_exitme:
		spllo();		/* pexit has locks in it */
		pexit("Killed", 1);

	case Proc_traceme:
		if(p->nnote == 0)
			return;
		/* No break */

	case Proc_stopme:
		p->procctl = 0;
		state = p->psstate;
		p->psstate = "Stopped";
		/* free a waiting debugger */
		pl = spllo();
		qlock(&p->debug);
		if(p->pdbg) {
			wakeup(&p->pdbg->sleep);
			p->pdbg = 0;
		}
		qunlock(&p->debug);
		splhi();
		p->state = Stopped;
		sched();
		p->psstate = state;
		splx(pl);
		return;

	case Proc_toac:
		p->procctl = 0;
		/*
		 * This pretends to return from the system call,
		 * by moving to a core, but never returns (unless
		 * the process gets moved back to a TC.)
		 */
		spllo();
		runacore();
		return;

	case Proc_totc:
		p->procctl = 0;
		if(p != up)
			panic("procctl: stopac: p != up");
		spllo();
		stopac();
		return;
	}
}

void
error(char *err)
{
	Proc *up = externup();
	spllo();

	assert(up->nerrlab < NERR);
	kstrcpy(up->errstr, err, ERRMAX);
	setlabel(&up->errlab[NERR-1]);
	nexterror();
}

void
nexterror(void)
{
	Proc *up = externup();
	/*debug*/gotolabel(&up->errlab[--up->nerrlab]);
}

void
exhausted(char *resource)
{
	char buf[ERRMAX];

	sprint(buf, "no free %s", resource);
	iprint("%s\n", buf);
	error(buf);
}

void
killbig(char *why)
{
	int i, x;
	Segment *s;
	uintptr_t l, max;
	Proc *p, *kp;

	max = 0;
	kp = nil;
	for(x = 0; (p = psincref(x)) != nil; x++) {
		if(p->state == Dead || p->kp){
			psdecref(p);
			continue;
		}
		l = 0;
		for(i=1; i<NSEG; i++) {
			s = p->seg[i];
			if(s != 0)
				l += s->top - s->base;
		}
		if(l > max && ((p->procmode&0222) || strcmp(eve, p->user)!=0)) {
			if(kp != nil)
				psdecref(kp);
			kp = p;
			max = l;
		}
		else
			psdecref(p);
	}
	if(kp == nil)
		return;

	print("%d: %s killed: %s\n", kp->pid, kp->text, why);
	for(x = 0; (p = psincref(x)) != nil; x++) {
		if(p->state == Dead || p->kp){
			psdecref(p);
			continue;
		}
		/* TODO(aki): figure out what this was for. the oom killer is broken anyway though?
		if(p != kp && p->seg[BSEG] && p->seg[BSEG] == kp->seg[BSEG])
			p->procctl = Proc_exitbig;
		*/
		psdecref(p);
	}

	kp->procctl = Proc_exitbig;
	for(i = 0; i < NSEG; i++) {
		s = kp->seg[i];
		if(s != 0 && canqlock(&s->lk)) {
			mfreeseg(s, s->base, (s->top - s->base)/BIGPGSZ);
			qunlock(&s->lk);
		}
	}
	psdecref(kp);
}

/*
 *  change ownership to 'new' of all processes owned by 'old'.  Used when
 *  eve changes.
 */
void
renameuser(char *old, char *new)
{
	int i;
	Proc *p;

	for(i = 0; (p = psincref(i)) != nil; i++){
		if(p->user!=nil && strcmp(old, p->user)==0)
			kstrdup(&p->user, new);
		psdecref(p);
	}
}

/*
 *  time accounting called by clock() splhi'd
 *  only cpu1 computes system load average
 *  but the system load average is accounted for cpu0.
 */
void
accounttime(void)
{
	Proc *p;
	uint32_t n, per;

	p = machp()->proc;
	if(p) {
		if(machp()->machno == 1)
			run.nrun++;
		p->time[p->insyscall]++;
	}

	/* calculate decaying duty cycles */
	n = perfticks();
	per = n - machp()->perf.last;
	machp()->perf.last = n;
	per = (machp()->perf.period*(HZ-1) + per)/HZ;
	if(per != 0)
		machp()->perf.period = per;

	machp()->perf.avg_inidle = (machp()->perf.avg_inidle*(HZ-1)+machp()->perf.inidle)/HZ;
	machp()->perf.inidle = 0;

	machp()->perf.avg_inintr = (machp()->perf.avg_inintr*(HZ-1)+machp()->perf.inintr)/HZ;
	machp()->perf.inintr = 0;

	/* only one processor gets to compute system load averages.
	 * it has to be mach 1 when we use AMP.
	 */
	//if(sys->nmach > 1 && machp()->machno != 1)
	 if(machp()->machno != 0) //Change to non-AMP
		return;

	/*
	 * calculate decaying load average.
	 * if we decay by (n-1)/n then it takes
	 * n clock ticks to go from load L to .36 L once
	 * things quiet down.  it takes about 5 n clock
	 * ticks to go to zero.  so using HZ means this is
	 * approximately the load over the last second,
	 * with a tail lasting about 5 seconds.
	 */
	n = run.nrun;
	run.nrun = 0;
	n = (run.nrdy+n)*1000;
	sys->load = (sys->load*(HZ-1)+n)/HZ;
}

void
halt(void)
{
	if(run.nrdy != 0)
		return;
	hardhalt();
}
