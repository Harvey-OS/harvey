#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"

typedef struct Clock0link Clock0link;
typedef struct Clock0link {
	void		(*clock)(void);
	Clock0link*	link;
} Clock0link;

static Clock0link *clock0link;
static Lock clock0lock;
ulong	clkrelinq;
void	(*kproftick)(ulong);	/* set by devkprof.c when active */
long	clkvv;
ulong	clkhigh;

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
delay(int l)
{
	ulong i, j;

	j = m->delayloop;
	while(l-- > 0)
		for(i=0; i < j; i++)
			;
}

void
microdelay(int l)
{
	ulong i;

	l *= m->delayloop;
	l += 500;
	l /= 1000;
	if(l <= 0)
		l = 1;
	for(i = 0; i < l; i++)
		;
}

// the following can be 4 or 16 depending on the clock multiplier
// see 15.3.3 in 860 manual
enum {
	Timebase = 16,	/* system clock cycles per time base cycle */
};

static	ulong	clkreload;

#ifdef XXX
ulong
millisec(void)
{
	ulong tbu, tbl, x;
	vlong t;

	do {
		tbu = gettbu();
		tbl = gettbl();
	} while (gettbu() != tbu);
	t = (((vlong)tbu)<<32) + tbl;

	t <<= 4;
	t /= m->oscclk * 1000;

	x = t;
	if((long)(ledtime-x) < 0)
		powerdownled();

	return (ulong)t;
}

#endif
ulong
millisec(void)
{
	ulong tbl, x;

	tbl = gettbl();
	x = tbl/((m->oscclk * 1000)/16);

	return x;
}

void
delayloopinit(void)
{
	long x, est;

	est = m->cpuhz/1000;	/* initial estimate */
	m->delayloop = est;
	do {
		x = gettbl();
		delay(1);
		x = gettbl() - x;
	} while(x < 0);

	/*
	 *  fix count
	 */
	m->delayloop = (m->delayloop*est/Timebase)/(x);
	if(m->delayloop == 0)
		m->delayloop = 1;
}

void
clockinit(void)
{
	delayloopinit();

	clkreload = (m->clockgen/Timebase)/HZ-1;
	putdec(clkreload);
}

void
clockintr(Ureg *ur)
{
	Clock0link *lp;
	long v;

	v = (clkvv = -getdec());
	if(v > clkreload)
		clkhigh++;
	if(v > clkreload/2){
		if(v > clkreload)
			m->ticks += v/clkreload;
		v = 0;
	}
	putdec(clkreload-v);

	// keep alive for watchdog
	m->iomem->swsr = 0x556c;
	m->iomem->swsr = 0xaa39;
	
	m->ticks++;

	if((m->iomem->pddat & SIBIT(15)) == 0) {
		print("button pressed\n");
		delay(200);
		reset();
	}

	if(m->proc)
		m->proc->pc = ur->pc;

	accounttime();
	if(kproftimer != nil)
		kproftimer(ur->pc);


	checkalarms();
	if(m->machno == 0) {
		lock(&clock0lock);
		for(lp = clock0link; lp; lp = lp->link)
			lp->clock();
		unlock(&clock0lock);
	}

	if(m->flushmmu){
		if(up)
			flushmmu();
		m->flushmmu = 0;
	}

	if(up == 0 || up->state != Running)
		return;

// user profiling clock 
//	if(ur->status & KUSER) {
//		(*(ulong*)(USTKTOP-BY2WD)) += TK2MS(1);
//		segclock(ur->pc);
//	}

	if(anyready())
		sched();
}

void
clkprint(void)
{
	print("clkr=%ld clkvv=%ld clkhigh=%lud\n", clkreload, clkvv, clkhigh);
	print("\n");
}

vlong
fastticks(uvlong *hz)
{
	if(hz)
		*hz = HZ;
	return m->ticks;
}
