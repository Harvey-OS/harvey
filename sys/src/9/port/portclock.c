#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"

typedef struct Timers Timers;

struct Timers
{
	Lock;
	Timer	*head;
};

static Timers timers[MAXMACH];

ulong intrcount[MAXMACH];
ulong fcallcount[MAXMACH];

static uvlong
tadd(Timers *tt, Timer *nt)
{
	Timer *t, **last, *pt;

	pt = nil;
	for(last = &tt->head; t = *last; last = &t->next){
		if(t == nt){
			/* timer's changing, remove it before putting it back on */
			*last = t->next;
			break;
		}
		if(t->period == nt->period){
			/* look for another timer at same frequency for combining */
			pt = t;
		}
	}

	if(nt->when == 0){
		/* Try to synchronize periods to reduce # of interrupts */
		assert(nt->period);
		if(pt)
			nt->when = pt->when;
		else
			nt->when = (uvlong)fastticks(nil) + nt->period;
	}
	
	for(last = &tt->head; t = *last; last = &t->next){
		if(t->when > nt->when)
			break;
	}
	nt->next = *last;
	*last = nt;
	if(last == &tt->head)
		return nt->when;
	else
		return 0;
}

/* add of modify a timer */
void
timeradd(Timer *nt)
{
	Timers *tt;
	uvlong when;

	tt = &timers[m->machno];
	ilock(tt);
	when = tadd(tt, nt);
	if(when)
		timerset(when);
	iunlock(tt);
}

void
timerdel(Timer *dt)
{
	Timer *t, **last;
	Timers *tt;

	tt = &timers[m->machno];
	ilock(tt);
	for(last = &tt->head; t = *last; last = &t->next){
		if(t == dt){
			*last = t->next;
			break;
		}
	}
	if(last == &tt->head && tt->head)
		timerset(tt->head->when);
	iunlock(tt);
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

	if(active.exiting && (active.machs & (1<<m->machno))) {
		print("someone's exiting\n");
		exit(0);
	}

	checkalarms();

	if(up == 0 || up->state != Running)
		return;

	/* user profiling clock */
	if(userureg(ur)){
		(*(ulong*)(USTKTOP-BY2WD)) += TK2MS(1);
		segclock(ur->pc);
	}

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
		when = t->when;
		if(when > now){
			timerset(when);
			iunlock(tt);
			m->splpc = pc;	/* for kernel profiling */
			if(callhzclock)
				hzclock(u);
			return;
		}
		tt->head = t->next;
		fcallcount[m->machno]++;
		iunlock(tt);
		if(t->f){
			(*t->f)(u, t);
			splhi();
		} else
			callhzclock++;
		ilock(tt);
		if(t->period){
			t->when += t->period;
			tadd(tt, t);
		}
	}
	iunlock(tt);
}

uvlong hzperiod;

void
timersinit(void)
{
	Timer *t;

	hzperiod = ms2fastticks(1000/HZ);

	t = malloc(sizeof(*t));
	t->when = 0;
	t->period = hzperiod;
	t->f = nil;
	timeradd(t);
}

void
addclock0link(void (*f)(void), int ms)
{
	Timer *nt;

	/* Synchronize this to hztimer: reduces # of interrupts */
	nt = malloc(sizeof(Timer));
	nt->when = 0;
	if(ms == 0)
		ms = 1000/HZ;
	nt->period = ms2fastticks(ms);
	nt->f = (void (*)(Ureg*, Timer*))f;

	ilock(&timers[0]);
	tadd(&timers[0], nt);
	/* no need to restart timer:
	 * this one's synchronized with hztimer which is already running
	 */
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
