#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"axp.h"

#include	"ureg.h"

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
clockinit(void)
{
}

uvlong
cycletimer(void)
{
	ulong pcc;
	vlong delta;

	pcc = rpcc(nil) & 0xFFFFFFFF;
	if(m->cpuhz == 0){
		/*
		 * pcclast is needed to detect wraparound of
		 * the cycle timer which is only 32-bits.
		 * m->cpuhz is set from the info passed from
		 * the firmware.
		 * This could be in clockinit if can
		 * guarantee no wraparound between then and now.
		 *
		 * All the clock stuff needs work.
		 */
		m->cpuhz = hwrpb->cfreq;
		m->pcclast = pcc;
	}
	delta = pcc - m->pcclast;
	if(delta < 0)
		delta += 0x100000000LL;
	m->pcclast = pcc;
	m->fastclock += delta;

	return MACHP(0)->fastclock;
}

vlong
fastticks(uvlong* hz)
{
	uvlong ticks;
	int x;

	x = splhi();
	ticks = cycletimer();
	splx(x);

	if(hz)
		*hz = m->cpuhz;

	return (vlong)ticks;
}

void
microdelay(int us)
{
	uvlong eot;

	eot = fastticks(nil) + (m->cpuhz/1000000)*us;
	while(fastticks(nil) < eot)
		;
}

void
/*milli*/delay(int ms)
{
	microdelay(ms*1000);
}

void
clock(Ureg *ur)
{
	Clock0link *lp;
	static int count;

	cycletimer();
	/* HZ == 100, timer == 1024Hz.  error < 1ms */
	count += 100;
	if (count < 1024)
		return;
	count -= 1024;

	m->ticks++;
	if(m->proc)
		m->proc->pc = ur->pc;

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
		lock(&clock0lock);
		for(lp = clock0link; lp; lp = lp->link)
			lp->clock();
		unlock(&clock0lock);
	}

	if(up == 0 || up->state != Running)
		return;

	if(anyready())
		sched();

	/* user profiling clock */
	if(ur->status & UMODE) {
		(*(ulong*)(USTKTOP-BY2WD)) += TK2MS(1);
		segclock(ur->pc);
	}
}
