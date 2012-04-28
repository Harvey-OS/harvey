#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"

enum {
	Maxtimerloops = 20*1000,
};

struct Timers
{
	Lock;
	Timer	*head;
};

static Timers timers[MAXMACH];
static int timersinited;

ulong intrcount[MAXMACH];
ulong fcallcount[MAXMACH];

static vlong
tadd(Timers *tt, Timer *nt)
{
	Timer *t, **last;

	/* Called with tt locked */
	assert(nt->tt == nil);
	switch(nt->tmode){
	default:
		panic("timer");
		break;
	case Trelative:
		if(nt->tns <= 0)
			nt->tns = 1;
		nt->twhen = fastticks(nil) + ns2fastticks(nt->tns);
		break;
	case Tperiodic:
		assert(nt->tns >= 100000);	/* At least 100 µs period */
		if(nt->twhen == 0){
			/* look for another timer at same frequency for combining */
			for(t = tt->head; t; t = t->tnext){
				if(t->tmode == Tperiodic && t->tns == nt->tns)
					break;
			}
			if (t)
				nt->twhen = t->twhen;
			else
				nt->twhen = fastticks(nil);
		}
		nt->twhen += ns2fastticks(nt->tns);
		break;
	}

	for(last = &tt->head; t = *last; last = &t->tnext){
		if(t->twhen > nt->twhen)
			break;
	}
	nt->tnext = *last;
	*last = nt;
	nt->tt = tt;
	if(last == &tt->head)
		return nt->twhen;
	return 0;
}

static uvlong
tdel(Timer *dt)
{

	Timer *t, **last;
	Timers *tt;

	tt = dt->tt;
	if (tt == nil)
		return 0;
	for(last = &tt->head; t = *last; last = &t->tnext){
		if(t == dt){
			assert(dt->tt);
			dt->tt = nil;
			*last = t->tnext;
			break;
		}
	}
	if(last == &tt->head && tt->head)
		return tt->head->twhen;
	return 0;
}

/* add or modify a timer */
void
timeradd(Timer *nt)
{
	Timers *tt;
	vlong when;

	/* Must lock Timer struct before Timers struct */
	ilock(nt);
	if(tt = nt->tt){
		ilock(tt);
		tdel(nt);
		iunlock(tt);
	}
	tt = &timers[m->machno];
	ilock(tt);
	when = tadd(tt, nt);
	if(when)
		timerset(when);
	iunlock(tt);
	iunlock(nt);
}


void
timerdel(Timer *dt)
{
	Timers *tt;
	uvlong when;

	ilock(dt);
	if(tt = dt->tt){
		ilock(tt);
		when = tdel(dt);
		if(when && tt == &timers[m->machno])
			timerset(tt->head->twhen);
		iunlock(tt);
	}
	iunlock(dt);
}

void
hzclock(Ureg *ur)
{
	m->ticks++;
	if(m->proc)
		m->proc->pc = ur->pc;

	if(m->flushmmu){
		if(up)
			flushmmu();
		m->flushmmu = 0;
	}

	accounttime();
	kmapinval();

	if(kproftimer != nil)
		kproftimer(ur->pc);

	if((active.machs&(1<<m->machno)) == 0)
		return;

	if(active.exiting) {
		print("someone's exiting\n");
		exit(0);
	}

	checkalarms();

	if(up && up->state == Running)
		hzsched();	/* in proc.c */
}

void
timerintr(Ureg *u, Tval)
{
	Timer *t;
	Timers *tt;
	uvlong when, now;
	int count, callhzclock;

	intrcount[m->machno]++;
	callhzclock = 0;
	tt = &timers[m->machno];
	now = fastticks(nil);
	if(now == 0)
		panic("timerintr: zero fastticks()");
	ilock(tt);
	count = Maxtimerloops;
	while((t = tt->head) != nil){
		/*
		 * No need to ilock t here: any manipulation of t
		 * requires tdel(t) and this must be done with a
		 * lock to tt held.  We have tt, so the tdel will
		 * wait until we're done
		 */
		when = t->twhen;
		if(when > now){
			timerset(when);
			iunlock(tt);
			if(callhzclock)
				hzclock(u);
			return;
		}
		tt->head = t->tnext;
		assert(t->tt == tt);
		t->tt = nil;
		fcallcount[m->machno]++;
		iunlock(tt);
		if(t->tf)
			(*t->tf)(u, t);
		else
			callhzclock++;
		ilock(tt);
		if(t->tmode == Tperiodic)
			tadd(tt, t);
		if (--count <= 0) {
			count = Maxtimerloops;
			iprint("timerintr: probably stuck in while loop; "
				"scrutinise clock.c or use faster cycle "
				"counter\n");
		}
	}
	iunlock(tt);
}

void
timersinit(void)
{
	Timer *t;

	/*
	 * T->tf == nil means the HZ clock for this processor.
	 */
	timersinited = 1;
	todinit();
	t = malloc(sizeof(*t));
	if(t == nil)
		error(Enomem);
	t->tmode = Tperiodic;
	t->tt = nil;
	t->tns = 1000000000/HZ;
	t->tf = nil;
	timeradd(t);
}

Timer*
addclock0link(void (*f)(void), int ms)
{
	Timer *nt;
	uvlong when;

	if(!timersinited)
		panic("addclock0link: timersinit not called yet");
	/* Synchronize to hztimer if ms is 0 */
	nt = malloc(sizeof(Timer));
	if(nt == nil)
		error(Enomem);
	if(ms == 0)
		ms = 1000/HZ;
	nt->tns = (vlong)ms*1000000LL;
	nt->tmode = Tperiodic;
	nt->tt = nil;
	nt->tf = (void (*)(Ureg*, Timer*))f;

	ilock(&timers[0]);
	when = tadd(&timers[0], nt);
	if(when)
		timerset(when);
	iunlock(&timers[0]);
	return nt;
}

/*
 *  This tk2ms avoids overflows that the macro version is prone to.
 *  It is a LOT slower so shouldn't be used if you're just converting
 *  a delta.
 */
ulong
tk2ms(ulong ticks)
{
	uvlong t, hz;

	t = ticks;
	hz = HZ;
	t *= 1000L;
	t = t/hz;
	ticks = t;
	return ticks;
}

ulong
ms2tk(ulong ms)
{
	/* avoid overflows at the cost of precision */
	if(ms >= 1000000000/HZ)
		return (ms/1000)*HZ;
	return (ms*HZ+500)/1000;
}
