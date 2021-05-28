/* Xilink XPS interrupt controller */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "ethermii.h"
#include "../port/netif.h"

#include "io.h"

enum {
	/* mer bits */
	Merme	= 1<<0,		/* master enable */
	Merhie	= 1<<1,		/* hw intr enable */

	Maxintrs = 32,
	Ntimevec = 20,		/* # of time buckets for each intr */
};

typedef struct {
	ulong	isr;		/* status */
	ulong	ipr;		/* pending (ro) */
	ulong	ier;		/* enable */
	ulong	iar;		/* acknowledge (wo) */
	ulong	sieb;		/* set ie bits; avoid */
	ulong	cieb;		/* clear ie bits; avoid */
	ulong	ivr;		/* vector; silly */
	ulong	mer;		/* master enable */
} Intregs;

typedef struct {
	ulong	bit;
	int	(*svc)(ulong);
	char	*name;
	uvlong	count;
} Intr;

static Intregs *irp = (Intregs *)Intctlr;
static Intr intrs[Maxintrs];
static Intr *nextintr;
static int tmoutok;

ulong intrs1sec;			/* count interrupts in this second */
ulong intrtimes[256][Ntimevec];

static Intr *
bit2intr(ulong bit)
{
	Intr *ip;

	for (ip = intrs; ip->bit != 0 && ip->bit != bit; ip++)
		;
	if (ip->bit == 0)
		return nil;
	return ip;
}

/*
 *  keep histogram of interrupt service times
 */
void
intrtime(Mach*, int vno)
{
	ulong diff, x;

	x = perfticks();
	diff = x - m->perf.intrts;
	m->perf.intrts = x;

	m->perf.inintr += diff;
	if(up == nil && m->perf.inidle > diff)
		m->perf.inidle -= diff;

	diff /= (m->cpuhz/1000000)*100;		/* quantum = 100µsec */
	if(diff >= Ntimevec)
		diff = Ntimevec-1;
	intrtimes[vno][diff]++;
}

void
intrfmtcounts(char *s, char *se)
{
	Intr *ip;

	for (ip = intrs; ip->bit != 0; ip++)
		s = seprint(s, se, "bit %#lux %s\t%llud intrs\n",
			ip->bit, ip->name, ip->count);
}

static void
dumpcounts(void)
{
	Intr *ip;

	for (ip = intrs; ip->bit != 0; ip++)
		iprint("bit %#lux %s\t%llud intrs\n",
			ip->bit, ip->name, ip->count);
	delay(100);
}

extern Ureg *gureg;

/*
 * called from trap on external (non-clock) interrupts
 * to poll interrupt handlers.
 */
void
intr(Ureg *ureg)
{
	int handled;
	ulong pend;
	Intr *ip;

	if (m->machno != 0)		/* only 1st cpu handles interrupts */
		return;
	gureg = ureg;
	while ((pend = irp->ipr) != 0) {
		for (ip = intrs; pend != 0 && ip->bit != 0; ip++)
			if (pend & ip->bit) {
				handled = ip->svc(ip->bit);
				splhi();   /* in case handler went spllo() */
				if (handled) {
					intrtime(m, ip - intrs);
					ip->count++;
					pend &= ~ip->bit;
				}
			}
		if (pend != 0) {
			dumpcounts();
			iprint("interrupt with no handler; ipr %#lux\n", pend);
		}
		if (++intrs1sec > 960*2 + 3*30000 + 1000) {
			intrs1sec = 0;
			dumpcounts();
			panic("too many intrs in one second");
		}
	}
}

void
intrinit(void)
{
	/* disable and neuter the intr controller */
	intrack(~0);
	clrmchk();
	barriers();
	irp->ier = 0;
	barriers();
	intrack(~0);
	barriers();

	nextintr = intrs;
	nextintr->bit = 0;
	barriers();

	/* turn on the intr controller, initially with no intrs enabled */
	irp->mer = Merme | Merhie;
	barriers();

//	intrack(~0);
//	clrmchk();
	tmoutok = 1;
	barriers();
}

/* register func as the interrupt-service routine for bit */
void
intrenable(ulong bit, int (*func)(ulong), char *name)
{
	Intr *ip;

	for (ip = intrs; ip->bit != 0; ip++)
		if (bit == ip->bit) {
			assert(func == ip->svc);
			return;		/* already registered */
		}

	assert(nextintr < intrs + nelem(intrs));
	assert(bit != 0);
	assert(func != nil);
	ip = nextintr++;
	ip->bit = bit;
	ip->svc = func;
	ip->name = name;
	sync();
	irp->ier |= bit;
	barriers();
}

void
intrack(ulong bit)
{
	irp->iar = bit;
	barriers();
}

void
intrshutdown(void)
{
	irp->ier = 0;
	irp->mer = 0;
	barriers();
	intrack(~0);
}
