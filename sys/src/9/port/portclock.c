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
portclock(Ureg *ur)
{
	Clock0link *lp;

	m->ticks++;
	if(m->proc)
		m->proc->pc = ur->pc;

	if(m->inclockintr)
		return;		/* interrupted ourself */
	m->inclockintr = 1;

	accounttime();

	if(kproftimer != nil)
		kproftimer(ur->pc);

	if((active.machs&(1<<m->machno)) == 0)
		return;

	if(active.exiting && (active.machs & (1<<m->machno))) {
		print("someone's exiting\n");
		exit(0);
	}

	checkalarms();
	if(m->machno == 0){
		if(canlock(&clock0lock))
			for(lp = clock0link; lp; lp = lp->link){
				lp->clock();
				splhi();
			}
		unlock(&clock0lock);
	}

	if(up == 0 || up->state != Running){
		m->inclockintr = 0;
		return;
	}

	if(anyready())
		sched();

	/* user profiling clock */
	if(userureg(ur)) {
		(*(ulong*)(USTKTOP-BY2WD)) += TK2MS(1);
		segclock(ur->pc);
	}
	m->inclockintr = 0;
}
