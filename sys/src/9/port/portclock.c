#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"

struct Timers
{
	Lock;
	Timer	*head;
};

static Timers timers[MAXMACH];

ulong intrcount[MAXMACH];
ulong fcallcount[MAXMACH];

static uvlong
tadd(Timers *tt, Timer *nt, uvlong now)
{
	Timer *t, **last;

	/* Called with tt locked */
	assert(nt->tt == nil);
	switch(nt->tmode){
	default:
		panic("timer");
		break;
	case Trelative:
		assert(nt->tns > 0);
		nt->twhen = now + ns2fastticks(nt->tns);
		break;
	case Tabsolute:
		nt->twhen = tod2fastticks(nt->tns);
		break;
	case Tperiodic:
		assert(nt->tns > 0);
		if(nt->twhen == 0){
			/* look for another timer at same frequency for combining */
			for(t = tt->head; t; t = t->tnext){
				if(t->tmode == Tperiodic && t->tns == nt->tns)
					break;
			}
			if (t)
				nt->twhen = t->twhen;
			else
				nt->twhen = now + ns2fastticks(nt->tns);
		}else
			nt->twhen = now + ns2fastticks(nt->tns);
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

/* add or modify a timer */
void
timeradd(Timer *nt)
{
	Timers *tt;
	uvlong when;

	if (nt->tmode == Tabsolute){
		when = todget(nil);
		if (nt->tns <= when){
			if (nt->tns + MS2NS(1) <= when)	/* Give it some slack, small deviations will happen */
				print("timeradd (%lld %lld) %lld too early 0x%lux\n",
					when, nt->tns, when - nt->tns, getcallerpc(&nt));
			nt->tns = when;
		}
	}
	if (nt->tt)
		timerdel(nt);
	tt = &timers[m->machno];
	ilock(tt);
	when = tadd(tt, nt, fastticks(nil));
	if(when)
		timerset(when);
	iunlock(tt);
}

void
timerdel(Timer *dt)
{
	Timer *t, **last;
	Timers *tt;

	while(tt = dt->tt){
		ilock(tt);
		if (tt != dt->tt){
			iunlock(tt);
			continue;
		}
		for(last = &tt->head; t = *last; last = &t->tnext){
			if(t == dt){
				assert(dt->tt);
				dt->tt = nil;
				*last = t->tnext;
				break;
			}
		}
		if(last == &tt->head && tt->head)
			timerset(tt->head->twhen);
		iunlock(tt);
	}
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
timerintr(Ureg *u, uvlong)
{
	Timer *t;
	Timers *tt;
	uvlong when, now;
	int callhzclock;
	ulong pc;
	static int sofar;

	pc = m->splpc;	/* remember last splhi pc for kernel profiling */

	intrcount[m->machno]++;
	callhzclock = 0;
	tt = &timers[m->machno];
	now = fastticks(nil);
	ilock(tt);
	while(t = tt->head){
		when = t->twhen;
		if(when > now){
			timerset(when);
			iunlock(tt);
			m->splpc = pc;	/* for kernel profiling */
			if(callhzclock)
				hzclock(u);
			return;
		}
		tt->head = t->tnext;
		assert(t->tt == tt);
		t->tt = nil;
		fcallcount[m->machno]++;
		iunlock(tt);
		if(t->tf){
			(*t->tf)(u, t);
			splhi();
		} else
			callhzclock++;
		ilock(tt);
		if(t->tmode == Tperiodic){
			t->twhen += ns2fastticks(t->tns);
			tadd(tt, t, now);
		}
	}
	iunlock(tt);
}

void
timersinit(void)
{
	Timer *t;

	todinit();
	t = malloc(sizeof(*t));
	t->tmode = Tperiodic;
	t->tt = nil;
	t->tns = 1000000000/HZ;
	t->tf = nil;
	timeradd(t);
}

void
addclock0link(void (*f)(void), int ms)
{
	Timer *nt;
	uvlong ft;

	/* Synchronize to hztimer if ms is 0 */
	nt = malloc(sizeof(Timer));
	if(ms == 0)
		ms = 1000/HZ;
	nt->tns = (vlong)ms*1000000LL;
	nt->tmode = Tperiodic;
	nt->tt = nil;
	nt->tf = (void (*)(Ureg*, Timer*))f;

	ft = fastticks(nil);

	ilock(&timers[0]);
	tadd(&timers[0], nt, ft);
	iunlock(&timers[0]);
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
