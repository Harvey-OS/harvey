#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"

void (*kproftimer)(ulong);

typedef struct Clock0link Clock0link;
typedef struct Clock0link {
	void		(*clock)(void);
	Clock0link*	link;
} Clock0link;

static Clock0link *clock0link;
static Lock clock0lock;

void
addclock0link(void (*clock)(void))
{
	Clock0link *lp;

	if((lp = malloc(sizeof(Clock0link))) == 0){
		print("addclock0link: too many links\n");
		return;
	}
	ilock(&clock0lock);
	lp->clock = clock;
	lp->link = clock0link;
	clock0link = lp;
	iunlock(&clock0lock);
}

void
clockintr(Ureg* ureg, void*)
{
	Clock0link *lp;

	// this needs to be here for synchronizing mp clocks
	// see mp.c's squidboy()
	fastticks(nil);
	m->ticks++;
	if(m->proc)
		m->proc->pc = ureg->pc;

	accounttime();
	if(kproftimer != nil)
		kproftimer(ureg->pc);
	if((active.machs & (1<<m->machno)) == 0)
		return;
	if(active.exiting && (active.machs & (1<<m->machno)))
		exit(0);

	checkalarms();
	if(m->machno == 0){
		ilock(&clock0lock);
		for(lp = clock0link; lp; lp = lp->link)
			lp->clock();
		iunlock(&clock0lock);
	}

	if(m->flushmmu){
		if(up)
			flushmmu();
		m->flushmmu = 0;
	}

	if(up == 0 || up->state != Running)
		return;

	if(anyready())
		sched();
	
	if((ureg->cs & 0xFFFF) == UESEL) {
		(*(ulong*)(USTKTOP-BY2WD)) += TK2MS(1);
		segclock(ureg->pc);
	}
}

void
delay(int millisecs)
{
	millisecs *= m->loopconst;
	if(millisecs <= 0)
		millisecs = 1;
	aamloop(millisecs);
}

void
microdelay(int microsecs)
{
	microsecs *= m->loopconst;
	microsecs /= 1000;
	if(microsecs <= 0)
		microsecs = 1;
	aamloop(microsecs);
}

ulong
TK2MS(ulong ticks)
{
	uvlong t, hz;

	t = ticks;
	hz = HZ;
	t *= 1000L;
	t = t/hz;
	ticks = t;
	return ticks;
}
