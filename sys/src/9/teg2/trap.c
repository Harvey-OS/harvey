/*
 * arm mpcore generic interrupt controller (gic) v1 in cortex-a9
 * traps, exceptions, interrupts, system calls.
 * the cortex-a9 gic is similar to the pl390, but slightly different.
 * beware security extensions.
 *
 * there are two pieces: the interrupt distributor and the cpu interface.
 * most of their registers must be accessed as (u)longs, else you get:
 *
 *	panic: external abort 0x28 pc 0xc048bf68 addr 0x50041800
 *
 * apparently irqs 0—15 (SGIs) are always enabled.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "ureg.h"
#include "arm.h"

#define ISSGI(irq)	((uint)(irq) < Nsgi)
#define ISPPI(irq)	((uint)(irq) < Nppi)

enum {
	Debug		= 0,
	Measurestack	= 0,
	Checkstack	= 0,

	/*
	 * arm uses backwards priority field values:
	 * 0 is highest priority; 0xff is lowest.
	 * actually using nested interrupts would require
	 * something more nuanced than splhi/spllo
	 * involving the primask registers.
	 */
	Prilowest= 0xff,
	Prinorm	= 0,
	Priclock= 0,

	/*
	 * constants fixed by hardware
	 */

	Nvec = 8,		/* # of vectors at start of lexception.s */
	Bi2long = BI2BY * sizeof(long),

	Nsgi	= 16,		/* software-generated (inter-processor) intrs */
	Nppi	= 32,		/* sgis + other private peripheral intrs */
	Nusableirqs = 1020,
	Nirqs	= 1024,

	Nirqwords = Nirqs / Bi2long,
	Cfgwidth = 2,
	Cfgsperwd = Bi2long / Cfgwidth,
};

typedef struct Intrcpuregs Intrcpuregs;
typedef struct Intrdistregs Intrdistregs;

/*
 * almost this entire register set is buggered.
 * the distributor is supposed to be per-system, not per-cpu,
 * yet some registers are banked per-cpu, as marked.
 */
struct Intrdistregs {			/* distributor */
	ulong	ctl;
	ulong	ctlrtype;
	ulong	distid;
	uchar	_pad0[0x80 - 0xc];

	/* botch: *[0] are banked per-cpu from here */
	/* bit maps */
	ulong	grp[Nirqwords];		/* in group 1 (non-secure) */
	ulong	setena[Nirqwords];	/* forward to cpu interfaces */
	ulong	clrena[Nirqwords];
	ulong	setpend[Nirqwords];	/* pending */
	ulong	clrpend[Nirqwords];
	ulong	active[Nirqwords];	/* active (ro) */
	ulong	_pad1[Nirqwords];
	/* botch: *[0] are banked per-cpu until here */

	uchar	pri[Nusableirqs];  /* botch: 0 — Nppi-1 are banked per-cpu */
	ulong	_rsrvd1;
	/* botch: 0 — Nppi-1 are banked per-cpu and RO */
	uchar	targ[Nusableirqs]; /* byte bitmasks: cpu targets indexed by intr */
	ulong	_rsrvd2;
	/* botch: cfg[0] is RO, cfg[1] is banked per-cpu */
	ulong	cfg[2*Nirqwords];	/* config bit pairs: edge? 1-N? */
	ulong	ists[Nirqwords];
	uchar	_pad2[0xdd4 - 0xd80];

	/* test registers */
	ulong	legacy_int;
	uchar	_pad3[0xde0 - 0xdd8];
	ulong	match;
	ulong	enable;

	uchar	_pad4[0xf00 - 0xde8];
	/* software-generated intrs (a.k.a. sgi) */
	ulong	swgen;			/* intr targets (wo) */
	uchar	_pad5[0xf10 - 0xf04];

	uchar	clrsgipend[Nsgi];	/* bit map (v2 only) */
	uchar	setsgipend[Nsgi];	/* bit map (v2 only) */
	uchar	_pad6[0xfc0 - 0xf30];
	ulong	id8;
	uchar	_pad7[0xfd0 - 0xfc4];
	ulong	id4[4];			/* 4—7 */
	ulong	id0[4];			/* 0—3 */
	ulong	compid[4];		/* 0—3 */
};

enum {
	/* ctl bits */
	Forw2cpuif =	1,

	/* ctlrtype bits */
	Tz	  =	1<<10,		/* trustzone: has (non)secure modes */
	Cpunoshft =	5,
	Cpunomask =	MASK(3),
	Intrlines =	MASK(5),

	/* cfg bits */
	Level =		0<<1,
	Edge =		1<<1,		/* edge-, not level-sensitive */
	Toall =		0<<0,
	To1 =		1<<0,		/* vs. to all */

	/* swgen bits */
	Totargets =	0,
	Tonotme =	1<<24,
	Tome =		2<<24,
};

/* each cpu sees its own registers at the same base address (soc.intr) */
struct Intrcpuregs {
	ulong	ctl;
	ulong	primask;

	ulong	binpt;			/* group pri vs subpri split */
	ulong	ack;
	ulong	end;
	ulong	runpri;
	ulong	hipripend;

	/* aliased regs (secure, for group 1) */
	ulong	alibiunpt;
	ulong	aliack;			/* (v2 only) */
	ulong	aliend;			/* (v2 only) */
	ulong	alihipripend;		/* (v2 only) */

	uchar	_pad0[0xd0 - 0x2c];
	ulong	actpri[4];		/* (v2 only) */
	ulong	nsactpri[4];		/* (v2 only) */

	uchar	_pad0[0xfc - 0xf0];
	ulong	ifid;			/* ro */
};

enum {
	/* ctl bits */
	Enablesec =	1<<0,
	Enablens =	1<<1,
	Ackctl =	1<<2,
	Eoinodeact =	1<<9,		/* (v2 only) */

	/* (ali) ack/end/hipriend/deact bits */
	Intrmask =	MASK(10),
	Cpuidshift =	10,
	Cpuidmask =	MASK(3),

	/* ifid bits */
	Archversshift =	16,
	Archversmask =	MASK(4),
};

typedef struct Vctl Vctl;
typedef struct Vctl {
	Vctl*	next;		/* handlers on this vector */
	char	*name;		/* of driver, xallocated */
	void	(*f)(Ureg*, void*);	/* handler to call */
	void*	a;		/* argument to call it with */
} Vctl;

static Lock vctllock;
static Vctl* vctl[Nirqs];

/*
 *   Layout at virtual address 0.
 */
typedef struct Vpage0 {
	void	(*vectors[Nvec])(void);
	u32int	vtable[Nvec];
} Vpage0;

enum
{
	Ntimevec = 20		/* number of time buckets for each intr */
};
/* not used aside from increment; examined manually, if at all */
#ifdef MEASURE
ulong intrtimes[Nirqs][Ntimevec];
#endif

int irqtooearly = 1;

static ulong shadena[Nirqwords]; /* copy of enable bits, saved by intcmaskall */
static Lock distlock;
static uint itlines, itwords;

extern int notify(Ureg*);

static void dumpstackwithureg(Ureg *ureg);

void
printrs(int base, ulong word)
{
	int bit;

	for (bit = 0; word; bit++, word >>= 1)
		if (word & 1)
			iprint(" %d", base + bit);
}

void
dumpintrs(char *what, ulong *bits)
{
	int i, first, some;
	ulong word;
	Intrdistregs *idp = (Intrdistregs *)soc.intrdist;

	first = 1;
	some = 0;
	USED(idp);
	for (i = 0; i < itwords; i++) {
		word = bits[i];
		if (word) {
			if (first) {
				first = 0;
				iprint("%s", what);
			}
			some = 1;
			printrs(i * Bi2long, word);
		}
	}
	if (!some)
		iprint("%s none", what);
	iprint("\n");
}

void
dumpintrpend(void)
{
	Intrdistregs *idp = (Intrdistregs *)soc.intrdist;

	iprint("\ncpu%d gic regs:\n", m->machno);
	dumpintrs("secure", idp->grp);
	dumpintrs("enabled", idp->setena);
	dumpintrs("pending", idp->setpend);
	dumpintrs("active ", idp->active);
}

/*
 *  keep histogram of interrupt service times
 */
void
intrtime(Mach*, int vno)
{
	ulong diff;
	ulong x;

	x = perfticks();
	diff = x - m->perf.intrts;
	m->perf.intrts = x;

	m->perf.inintr += diff;
	if(up == nil && m->perf.inidle > diff)
		m->perf.inidle -= diff;
#ifdef MEASURE
	if (m->cpumhz == 0)
		return;			/* don't divide by zero */
	diff /= m->cpumhz*100;		/* quantum = 100µsec */
	if(diff >= Ntimevec)
		diff = Ntimevec-1;
	if ((uint)vno >= Nirqs)
		vno = Nirqs-1;
	intrtimes[vno][diff]++;
#endif
	USED(vno);
}

static void
intdismiss(Intrcpuregs *icp, ulong ack)
{
	icp->end = ack;
	coherence();
}

static int
irqinuse(uint irq)
{
	Intrdistregs *idp = (Intrdistregs *)soc.intrdist;

	return idp->setena[irq / Bi2long] & (1 << (irq % Bi2long));
}

void
intcunmask(uint irq)
{
	Intrdistregs *idp = (Intrdistregs *)soc.intrdist;

	ilock(&distlock);
	idp->setena[irq / Bi2long] = 1 << (irq % Bi2long);
	/* keep a copy for secondary cpus */
	if (m->machno == 0 && ISPPI(irq))
		shadena[0] = idp->setena[0];
	iunlock(&distlock);
}

void
intcmask(uint irq)
{
	Intrdistregs *idp = (Intrdistregs *)soc.intrdist;

	ilock(&distlock);
	idp->clrena[irq / Bi2long] = 1 << (irq % Bi2long);
	/* keep a copy for secondary cpus */
	if (m->machno == 0 && ISPPI(irq))
		shadena[0] = idp->setena[0];
	iunlock(&distlock);
}

static void
intcmaskall(Intrdistregs *idp)		/* mask all intrs for all cpus */
{
	int i;

	for (i = 0; i < itwords; i++) {
		shadena[i] = idp->setena[i];
		idp->clrena[i] = ~0;
	}
	coherence();
}

static void
intcunmaskall(Intrdistregs *idp)	/* unused */
{
	int i;

	for (i = 0; i < itwords; i++)
		idp->setena[i] = shadena[i];
	coherence();
}

static void
intrdistcfgcpu0(Intrdistregs *idp, ulong ppicfg, ulong spicfg)
{
	int i, cfgshift;

	if (Debug)
		iprint("gic v%ld part #%#lux tz %d\n",
			(idp->id0[2] >> 4) & MASK(4),
			(idp->id0[1] & MASK(4)) << 8 | idp->id0[0],
			idp->ctlrtype & Tz? 1: 0);
	itwords = (idp->ctlrtype & Intrlines) + 1;
	itlines = Bi2long * itwords;

	for (i = 0; i < itwords; i++)
		idp->grp[i] = 0;		/* secure */
	/* pri for irqs 0 to Nppi-1 are banked */
	for (i = 0; i < itlines; i++)
		idp->pri[i] = Prinorm;
	idp->pri[Tn0irq] = idp->pri[Loctmrirq] = idp->pri[Wdtmrirq] =
		Priclock;

	/*
	 * cfg for irqs 0 — Nsgi-1 is read-only;
	 * for Nsgi — Nppi-1 are banked.
	 */
	idp->cfg[Nsgi/Cfgsperwd] = ppicfg;	/* non-sgi ppis */
	for (i = Nppi/Cfgsperwd; i < nelem(idp->cfg); i++)
		idp->cfg[i] = spicfg;
	/*
	 * timers are exceptions: they must be edge-triggered.
	 * Tn0irq is only used by the watchdog.
	 */
	cfgshift = Cfgwidth * (Tn0irq % Cfgsperwd);
	spicfg &= ~(MASK(Cfgwidth) << cfgshift);
	spicfg |= (Edge | Toall) << cfgshift;
	idp->cfg[Tn0irq / Cfgsperwd] = spicfg;

	/* first Nppi targs are banked & read-only for SGIs and PPIs */
	for (i = Nppi; i < itlines; i++)
		idp->targ[i] = 1<<0;	/* route to cpu 0 */
	coherence();

	intcmaskall(idp);		/* makes a copy in shadena */
	for (i = 0; i < itwords; i++) {
		idp->clrpend[i] = ~0;
		idp->active[i] = 0;
	}
}


static void
intrdistcfgcpun(Intrdistregs *idp, ulong ppicfg)
{
	int i;

	idp->grp[0] = 0;		/* secure */
	/* pri for irqs 0 to Nppi-1 are banked */
	for (i = 0; i < Nppi; i++)
		idp->pri[i] = Prinorm;
	idp->pri[Tn0irq] = idp->pri[Loctmrirq] = idp->pri[Wdtmrirq] =
		Priclock;

	/*
	 * cfg for irqs 0 — Nsgi-1 is read-only;
	 * for Nsgi — Nppi-1 are banked.
	 */
	idp->cfg[Nsgi/Cfgsperwd] = ppicfg;	/* non-sgi ppis */

	/* first Nppi targs are banked & read-only for SGIs and PPIs */
	coherence();

	idp->clrpend[0] = idp->clrena[0] = ~0;
	idp->active[0] = 0;
	coherence();
	/* on cpu1, irq Extpmuirq (118) is always pending here */
	idp->clrpend[Extpmuirq / Bi2long] = 1 << (Extpmuirq % Bi2long);
	coherence();
	idp->clrena [Extpmuirq / Bi2long] = 1 << (Extpmuirq % Bi2long);
}

static void
intrdistcfg(Intrdistregs *idp)
{
	int i;
	ulong ppicfg, spicfg;

	/*
	 * configure all interrupts as level-sensitive.
	 * ppis go to one cpu, but spis go to all.
	 */
	ppicfg = spicfg = 0;
	for (i = 0; i < Bi2long; i += Cfgwidth)
		ppicfg |= (Level | To1) << i;
	for (i = 0; i < Bi2long; i += Cfgwidth)
		spicfg |= (Level | Toall) << i;

	if (m->machno == 0)			/* system-wide & cpu0 cfg */
		intrdistcfgcpu0(idp, ppicfg, spicfg);
	else					/* per-cpu config */
		intrdistcfgcpun(idp, ppicfg);
	coherence();
	idp->setena[0] = MASK(Nppi - Nsgi);	/* enable non-sgi ppis */
	coherence();
}

/* route irq to cpu */
void
intrto(int cpu, int irq)
{
	Intrdistregs *idp = (Intrdistregs *)soc.intrdist;

	/* first Nppi are read-only for SGIs and the like */
	ilock(&distlock);
	idp->targ[irq] = 1 << cpu;
	iunlock(&distlock);
}

void
intrcpu(int cpu, int dolock)
{
	Intrdistregs *idp = (Intrdistregs *)soc.intrdist;

	if (dolock)
		ilock(&distlock);
	idp->swgen = Totargets | 1 << (cpu + 16) | m->machno;
	if (dolock)
		iunlock(&distlock);
	else
		coherence();
}

static void
memmovewb(void *dest, void *src, uint size)
{
	memmove(dest, src, size);
	allcacheswbse(dest, size);	 /* make visible to all */
}

/*
 * set up the exception vectors, high and low, and low-memory wfi loop.
 *
 * we can't use mva cache ops on HVECTORS address, since they
 * work on virtual addresses, and only those that have a
 * physical address == PADDR(virtual).
 */
void
vecinit(void)
{
	if (m->machno != 0)
		return;
	memmove((Vpage0 *)HVECTORS, vectors, sizeof(Vpage0));
	memmovewb((Vpage0 *)KADDR(0), vectors, sizeof(Vpage0));

	wfiloop = (void (*)(void))KADDR(sizeof(Vpage0));
	/* ugh: 2 is WFI, B */
	memmovewb(wfiloop, _wfiloop, 2 * sizeof(ulong));
	cacheiinv();
}

void
exceptinit(void)
{
	int s;

	/*
	 * set up the stack pointers for the exception modes for this cpu.
	 * they point to small `save areas' in Mach, not actual stacks.
	 */
	s = splhi();			/* make these modes ignore intrs too */
	setr13(PsrMfiq, m->sfiq);
	setr13(PsrMirq, m->sirq);
	setr13(PsrMmon, m->smon);
	setr13(PsrMabt, m->sabt);
	setr13(PsrMund, m->sund);
	setr13(PsrMsys, m->ssys);
	splx(s);
}

/*
 *  set up for exceptions.  some of it is per-cpu.
 */
void
trapinit(void)
{
	Intrdistregs *idp = (Intrdistregs *)soc.intrdist;
	Intrcpuregs *icp = (Intrcpuregs *)soc.intr;

	vecinit();
	exceptinit();

	assert((idp->distid & MASK(12)) == 0x43b);	/* made by arm */
	assert((icp->ifid   & MASK(12)) == 0x43b);	/* made by arm */

	ilock(&distlock);
	if (m->machno == 0) {
		/* if intr ctlr is running, stop it */
		idp->ctl = 0;
		coherence();
		delay(10);
	}
	icp->primask = Priclock;	/* let no priorities through */
	icp->binpt = 0;
	coherence();
	icp->ctl = 0;
	coherence();
	delay(10);

	intrdistcfg(idp);		/* some per-cpu cfg here */

	icp->primask = Prilowest;	/* let all priorities through */
	icp->binpt = 0;
	coherence();
	icp->ctl = Enablesec | Enablens | Ackctl;
	iunlock(&distlock);
	delay(10);

	/* arm recommend enabling the distributor last */
	idp->ctl = Forw2cpuif;	/* only needed on cpu0? */
	coherence();
	delay(10);
}

void
intrsoff(void)
{
	ilock(&distlock);
	intcmaskall((Intrdistregs *)soc.intrdist);
	iunlock(&distlock);
}

/* shutdown this cpu's part of the interrupt controller */
void
intrcpushutdown(void)
{
	Intrcpuregs *icp = (Intrcpuregs *)soc.intr;
	Intrdistregs *idp = (Intrdistregs *)soc.intrdist;

	icp->primask = Priclock;	/* let no priorities through */
	coherence();
	icp->ctl = 0;
	coherence();
	idp->clrena[0] = ~0;		/* turn off all local intrs */
	coherence();
}

/*
 * called from cpu0 after other cpus are shutdown to completely shutdown
 * the interrupt controller.
 */
void
intrshutdown(void)
{
	Intrdistregs *idp = (Intrdistregs *)soc.intrdist;

	intrsoff();
	intrcpushutdown();
	idp->ctl = 0;
	coherence();
}

/*
 *  enable an irq interrupt
 *  note that the same private interrupt may be enabled on multiple cpus
 */
int
irqenable(uint irq, void (*f)(Ureg*, void*), void* a, char *name)
{
	Vctl *v;

	if(irq >= nelem(vctl))
		panic("irqenable: irq %d too big", irq);

	if (irqtooearly) {
		iprint("irqenable for %d %s called too early\n", irq, name);
		return -1;
	}
	/*
	 * if in use, could be a private interrupt on a secondary cpu,
	 * so don't add anything to the vector chain.  irqs should
	 * otherwise be one-to-one with devices.
	 */
	if(!ISSGI(irq) && irqinuse(irq)) {
		lock(&vctllock);
		if (vctl[irq] == nil) {
			dumpintrpend();
			panic("non-sgi irq %d in use yet no Vctl allocated", irq);
		}
		unlock(&vctllock);
	}
	/* could be 1st use of this irq or could be an sgi (always in use) */
	else if (vctl[irq] == nil) {
		v = malloc(sizeof(Vctl));
		if (v == nil)
			panic("irqenable: malloc Vctl");
		v->f = f;
		v->a = a;
		v->name = malloc(strlen(name)+1);
		if (v->name == nil)
			panic("irqenable: malloc name");
		strcpy(v->name, name);

		lock(&vctllock);
		if (vctl[irq] != nil) {
			/* allocation race: someone else did it first */
			free(v->name);
			free(v);
		} else {
			v->next = vctl[irq];
			vctl[irq] = v;
		}
		unlock(&vctllock);
	}
	intcunmask(irq);
	return 0;
}

/*
 * spread the interrupt load from peripherals around.
 * we only use timers, uart and ether.
 */
void
irqreassign(void)
{
	int irq, cpu;

	if (active.nmachs <= 1)
		return;
	for (irq = Nppi; irq < Nirqs; irq++)
		if (vctl[irq] != nil) {
			cpu = 0;
			switch (irq) {
			case Pcieirq:
			case Pciemsiirq:
				cpu = 1;
				break;
			}
			if (cpu != 0)
				iprint("irqenable: %s: route irq %d to cpu %d\n",
					vctl[irq]->name, irq, cpu);
			intrto(cpu, irq);
		}
}

/*
 *  disable an irq interrupt
 */
int
irqdisable(uint irq, void (*f)(Ureg*, void*), void* a, char *name)
{
	Vctl **vp, *v;

	if(irq >= nelem(vctl))
		panic("irqdisable irq %d", irq);

	lock(&vctllock);
	for(vp = &vctl[irq]; v = *vp; vp = &v->next)
		if (v->f == f && v->a == a && strcmp(v->name, name) == 0){
			if(Debug)
				print("irqdisable: remove %s\n", name);
			*vp = v->next;
			free(v->name);
			free(v);
			break;
		}

	if(v == nil)
		print("irqdisable: irq %d, name %s not enabled\n", irq, name);
	if(vctl[irq] == nil){
		if(Debug)
			print("irqdisable: clear icmr bit %d\n", irq);
		intcmask(irq);
	}
	unlock(&vctllock);

	return 0;
}

static void
badfault(uintptr va, int read)
{
	char buf[ERRMAX];

	/* don't dump registers; programs suicide all the time */
	snprint(buf, sizeof buf, "sys: trap: fault %s va=%#p",
		read? "read": "write", va);
	postnote(up, 1, buf, NDebug);
}

/*
 *  called by trap to handle access faults
 */
static void
faultarm(Ureg *ureg, uintptr va, int user, int read)
{
	int n, insyscall;

	if(up == nil) {
		dumpstackwithureg(ureg);
		panic("faultarm: cpu%d: nil up, %sing %#p at %#p",
			m->machno, (read? "read": "writ"), va, ureg->pc);
	}
	insyscall = up->insyscall;
	up->insyscall = 1;

	n = fault(va, read);		/* goes spllo */
	splhi();
	if(n < 0){
		if(!user){
			dumpstackwithureg(ureg);
			panic("fault: cpu%d: kernel %sing %#p at %#p",
				m->machno, read? "read": "writ", va, ureg->pc);
		}
		badfault(va, read);	/* no return */
	}
	up->insyscall = insyscall;
}

/*
 *  called by trap to handle interrupts.
 *  returns true iff a clock interrupt, thus maybe reschedule.
 */
static int
irq(Ureg* ureg)
{
	int clockintr, ack;
	uint irqno, handled;
	Intrcpuregs *icp = (Intrcpuregs *)soc.intr;
	Intrdistregs *idp = (Intrdistregs *)soc.intrdist;
	Vctl *v;
	void (*isr)(Ureg*, void*);

	ack = icp->hipripend;			/* erratum 801120 dummy read */
	USED(ack);
	coherence();

	ack = icp->ack;
	coherence();
	irqno = ack & Intrmask;
	if (irqno == 0 && !(idp->active[0] & (1<<0))) {
		/*
		 * erratum 733075: spurious interrupt: if ID is 0, 0x3FE or
		 * 0x3FF, CPU interface may be locked-up.
		 */
		ack = idp->pri[0];
		idp->pri[0] = ack;		/* unlock cpu interface */
		coherence();
		return 0;
	}
	if (irqno >= nelem(vctl)) {
		iprint("trap: irq %d >= # vectors (%d)\n", irqno, (int)nelem(vctl));
		intdismiss(icp, ack);
		return 0;
	}
	if (irqno == Loctmrirq)			/* this is a clock intr? */
		m->inclockintr++;		/* yes, count nesting */
	if(m->machno && m->inclockintr > 1) {
		iprint("cpu%d: nested clock intrs\n", m->machno);
		m->inclockintr--;
		intdismiss(icp, ack);
		return 0;
	}

	handled = 0;
	for(v = vctl[irqno]; v != nil; v = v->next) {
		isr = v->f;
		if (isr) {
			isr(ureg, v->a);
			handled++;
		}
	}
	if(!handled)
		if (irqno >= 1022) {
			/* maybe count these instead of printing? */
			// iprint("cpu%d: ignoring spurious interrupt\n", m->machno);
		} else {
			intcmask(irqno);
			iprint("cpu%d: unexpected interrupt %d, now masked\n",
				m->machno, irqno);
		}
	clockintr = m->inclockintr == 1;
	if (irqno == Loctmrirq)
		m->inclockintr--;

	intdismiss(icp, ack);
	intrtime(m, irqno);
	return clockintr;
}

/*
 *  returns 1 if the instruction writes memory, 0 otherwise
 */
int
writetomem(ulong inst)
{
	/* swap always write memory */
	if((inst & 0x0FC00000) == 0x01000000)
		return 1;

	/* loads and stores are distinguished by bit 20 */
	if(inst & (1<<20))
		return 0;

	return 1;
}

static void
badalign(Ureg *ureg, uintptr va)
{
	char buf[ERRMAX];

	snprint(buf, sizeof buf, "sys: alignment: pc %#lux va %#p\n",
		ureg->pc, va);
	postnote(up, 1, buf, NDebug);
}

static void
accviolation(Ureg *ureg, uintptr va)
{
	char buf[ERRMAX];

	snprint(buf, sizeof buf, "sys: access violation: pc %#lux va %#p\n",
		ureg->pc, va);
	postnote(up, 1, buf, NDebug);
}

/* can't call validalign here */
static void
datafault(Ureg *ureg, int user)
{
	int x;
	ulong inst, fsr;
	uintptr va;
	static ulong tottrapped;

	va = farget();

	if (m->probing && !user) {
		if (m->trapped++ > 0) {
			dumpstackwithureg(ureg);
			panic("trap: recursive probe %#lux", va);
		}
		ureg->pc += 4;	/* continue after faulting instr'n */
		return;
	}

	inst = 0;
	if ((ureg->pc & (BY2WD-1)) == 0)
		inst = *(ulong*)ureg->pc;
	/* bits 12 and 10 have to be concatenated with status */
	x = fsrget();
	fsr = (x>>7) & 0x20 | (x>>6) & 0x10 | x & 0xf;
	switch(fsr){
	default:
	case 0xa:		/* ? was under external abort */
		panic("unknown data fault, 6b fsr %#lux", fsr);
		break;
	case 0x0:
		panic("vector exception at %#lux", ureg->pc);
		break;
	case 0x1:		/* alignment fault */
	case 0x3:		/* access flag fault (section) */
		if(user)
			badalign(ureg, va);	/* no return */
		else {
			dumpstackwithureg(ureg);
			panic("kernel alignment: pc %#lux va %#p", ureg->pc, va);
		}
		break;
	case 0x2:
		panic("terminal exception at %#lux", ureg->pc);
		break;
	case 0x4:		/* icache maint fault */
	case 0x6:		/* access flag fault (page) */
	case 0x8:		/* precise external abort, non-xlat'n */
	case 0x28:
	case 0x16:		/* imprecise ext. abort, non-xlt'n */
	case 0x36:
		panic("external non-translation abort %#lux pc %#lux addr %#p",
			fsr, ureg->pc, va);
		break;
	case 0xc:		/* l1 translation, precise ext. abort */
	case 0x2c:
	case 0xe:		/* l2 translation, precise ext. abort */
	case 0x2e:
		panic("external translation abort %#lux pc %#lux addr %#p",
			fsr, ureg->pc, va);
		break;
	case 0x1c:		/* l1 translation, precise parity err */
	case 0x1e:		/* l2 translation, precise parity err */
	case 0x18:		/* imprecise parity or ecc err */
		panic("translation parity error %#lux pc %#lux addr %#p",
			fsr, ureg->pc, va);
		break;
	case 0x5:		/* translation fault, no section entry */
	case 0x7:		/* translation fault, no page entry */
		faultarm(ureg, va, user, !writetomem(inst));
		break;
	case 0x9:
	case 0xb:
		/* domain fault, accessing something we shouldn't */
		if(user)
			accviolation(ureg, va);		/* no return */
		else
			panic("cpu%d: kernel access violation: pc %#lux va %#p",
				m->machno, ureg->pc, va);
		break;
	case 0xd:
	case 0xf:
		/* permission error, copy on write or real permission error */
		faultarm(ureg, va, user, !writetomem(inst));
		break;
	}
}

enum {
	Machstk,
	Procstk,
	Stks,
};

long bigstk[Stks];

static void
biggeststk(int stktype, int stksize)
{
	if (stksize > bigstk[stktype])
		bigstk[stktype] = stksize;
}

static ulong
howclean(ulong *base)
{
	ulong *wdp;

	for (wdp = base; *wdp == 0; wdp++)
		;
	return (wdp - base)*sizeof(ulong);
}

static void
prmachdirty(Mach *mach)
{
	ulong *wdp;

	for (wdp = (ulong *)mach->stack; *wdp == 0; wdp++)
		;
	iprint("cpu%d clean Mach stack %ld bytes\n", mach->machno,
		howclean((ulong *)mach->stack));
}

void
prstkuse(void)
{
	int i;
	ulong clean, leastclean;
	Proc *p;

	if (!Measurestack)
		return;
	iprint("biggest Mach stack %#ld\n", bigstk[0]);
	iprint("biggest proc k stack %#ld\n", bigstk[1]);
	for (i = 0; i < conf.nmach; i++)
		prmachdirty(machaddr[i]);
	leastclean = KSTKSIZE;
	for(i=0; i<conf.nproc; i++){
		p = proctab(i);
		if (p && p->kstack) {
			clean = howclean((ulong *)p->kstack);
			if (clean < leastclean)
				leastclean = clean;
		}
	}
	iprint("least clean proc k stack %ld\n", leastclean);
}

/*
 * ureg is the current stack pointer.  verify that it's within
 * a plausible range.
 */
void
ckstack(Ureg **uregp)
{
	int rem;
	uintptr kstack;
	Ureg *ureg;

	/*
	 * we assume that up != nil means that we are using a process's
	 * kernel stack.
	 */
	USED(uregp);
	if (Measurestack)
		if(up != nil)
			biggeststk(Procstk, (uintptr)up->kstack + KSTKSIZE -
				(uintptr)uregp);
		else
			biggeststk(Machstk, (uintptr)m + MACHSIZE -
				(uintptr)uregp);
	if (!Checkstack)
		return;
	kstack = (up? (uintptr)up->kstack: (uintptr)m->stack);
	rem = (uintptr)uregp - kstack;
	/*
	 * difference between sp at entry and maximum stack use
	 * of 956 bytes has been seen, so be cautious.
	 */
	if(rem < 1200) {
		char *which;

		which = up != nil? "process": "mach";
		ureg = *uregp;
		dumpstackwithureg(ureg);
		panic("trap: %d %s stack bytes left, up %#p ureg (sp) %#p "
			"kstack %#p m %#p cpu%d at pc %#lux",
			rem, which, up, ureg, kstack, m, m->machno, ureg->pc);
	}
}

/* move stack usage for error case here */
static void
undef(Ureg *ureg)
{
	ulong inst;
	char buf[ERRMAX];

	inst = 0;
	if ((ureg->pc & (BY2WD-1)) == 0)
		inst = *(ulong *)ureg->pc;
	snprint(buf, sizeof buf,
		"undefined instruction: pc %#lux instr %#8.8lux\n",
		ureg->pc, inst);
	postnote(up, 1, buf, NDebug);
}

/*
 *  here on all exceptions other than syscall (SWI) and reset
 */
void
trap(Ureg *ureg)
{
	int clockintr, user;
	uintptr va, ifar, ifsr;

	splhi();  /* paranoia - probably interferes with nested interrupts */
	m->perf.intrts = perfticks();
	ckstack(&ureg);
	user = (ureg->psr & PsrMask) == PsrMusr;
	if(user){
		up->dbgreg = ureg;
		cycles(&up->kentry);
	}

	/*
	 * All interrupts/exceptions should be resumed at ureg->pc-4,
	 * except for Data Abort which resumes at ureg->pc-8.
	 */
	if(ureg->type == PsrMabt+1)
		ureg->pc -= 8;
	else
		ureg->pc -= 4;

	clockintr = 0;		/* if set, may call sched() before return */
	switch(ureg->type){
	default:
		panic("unknown trap; type %#lux, psr mode %#lux", ureg->type,
			ureg->psr & PsrMask);
		break;
	case PsrMirq:
		m->intr++;
		clockintr = irq(ureg);
		if (!clockintr) {
			if(up)
				preempted();
			wakewfi();	/* may be (k)proc to run now */
		}
		break;
	case PsrMabt:			/* prefetch (instruction) fault */
		va = ureg->pc;
		ifsr = fsriget();
		switch((ifsr>>7) & 0x8 | ifsr & 0x7){
		case 0x02:		/* instruction debug event (BKPT) */
			if(user)
				postnote(up, 1, "sys: breakpoint", NDebug);
			else{
				iprint("kernel bkpt: pc %#lux inst %#ux\n",
					va, *(u32int*)va);
				panic("kernel bkpt");
			}
			break;
		default:
			ifar = fariget();
			if (va != ifar)
				iprint("trap: cpu%d: i-fault va %#p != ifar %#p\n",
					m->machno, va, ifar);
			faultarm(ureg, va, user, 1);
			break;
		}
		break;
	case PsrMabt+1:			/* data fault */
		datafault(ureg, user);
		break;
	case PsrMund:			/* undefined instruction */
		if(!user) {
			if (ureg->pc & (BY2WD-1)) {
				iprint("rounding fault pc %#lux down to word\n",
					ureg->pc);
				ureg->pc &= ~(BY2WD-1);
			}
			if (Debug)
				iprint("mathemu: cpu%d fpon %d instr %#8.8lux at %#p\n",
					m->machno, m->fpon, *(ulong *)ureg->pc,
				ureg->pc);
			dumpstackwithureg(ureg);
			panic("cpu%d: undefined instruction: pc %#lux inst %#ux",
				m->machno, ureg->pc, ((u32int*)ureg->pc)[0]);
		} else if(seg(up, ureg->pc, 0) != nil &&
		    (ureg->pc & (BY2WD-1)) == 0 &&
		    *(u32int*)ureg->pc == 0xe1200070)	/* see libmach/5db.c */
			postnote(up, 1, "sys: breakpoint", NDebug);
		else if(fpuemu(ureg) == 0)	/* didn't find any FP instrs? */
			undef(ureg);
		break;
	}
	splhi();

	/* delaysched set because we held a lock or because our quantum ended */
	if(up && up->delaysched && clockintr){
		sched();		/* can cause more traps */
		splhi();
	}

	if(user){
		if(up->procctl || up->nnote)
			notify(ureg);
		kexit(ureg);
	}
}

/*
 * Fill in enough of Ureg to get a stack trace, and call a function.
 * Used by debugging interface rdb.
 */
void
callwithureg(void (*fn)(Ureg*))
{
	Ureg ureg;

	memset(&ureg, 0, sizeof ureg);
	ureg.pc = getcallerpc(&fn);
	ureg.sp = PTR2UINT(&fn);
	fn(&ureg);
}

static void
dumpstackwithureg(Ureg *ureg)
{
	int x;
	uintptr l, v, i, estack;
	char *s;

	dumpregs(ureg);
	if((s = getconf("*nodumpstack")) != nil && strcmp(s, "0") != 0){
		iprint("dumpstack disabled\n");
		return;
	}
	delay(1000);
	iprint("dumpstack\n");

	x = 0;
	x += iprint("ktrace /kernel/path %#.8lux %#.8lux %#.8lux # pc, sp, link\n",
		ureg->pc, ureg->sp, ureg->r14);
	delay(20);
	i = 0;
	if(up
	&& (uintptr)&l >= (uintptr)up->kstack
	&& (uintptr)&l <= (uintptr)up->kstack+KSTACK)
		estack = (uintptr)up->kstack+KSTACK;
	else if((uintptr)&l >= (uintptr)m->stack
	&& (uintptr)&l <= (uintptr)m+MACHSIZE)
		estack = (uintptr)m+MACHSIZE;
	else
		return;
	x += iprint("estackx %p\n", estack);

	for(l = (uintptr)&l; l < estack; l += sizeof(uintptr)){
		v = *(uintptr*)l;
		if((KTZERO < v && v < (uintptr)etext) || estack-l < 32){
			x += iprint("%.8p ", v);
			delay(20);
			i++;
		}
		if(i == 8){
			i = 0;
			x += iprint("\n");
			delay(20);
		}
	}
	if(i)
		iprint("\n");
	delay(3000);
}

void
dumpstack(void)
{
	callwithureg(dumpstackwithureg);
}

/*
 * dump system control coprocessor registers
 */
static void
dumpscr(void)
{
	iprint("0:\t%#8.8ux id\n", cpidget());
	iprint("\t%8.8#ux ct\n", cpctget());
	iprint("1:\t%#8.8ux control\n", controlget());
	iprint("2:\t%#8.8ux ttb\n", ttbget());
	iprint("3:\t%#8.8ux dac\n", dacget());
	iprint("4:\t(reserved)\n");
	iprint("5:\t%#8.8ux fsr\n", fsrget());
	iprint("6:\t%#8.8ux far\n", farget());
	iprint("7:\twrite-only cache\n");
	iprint("8:\twrite-only tlb\n");
	iprint("13:\t%#8.8ux pid\n", pidget());
	delay(10);
}

/*
 * dump general registers
 */
static void
dumpgpr(Ureg* ureg)
{
	if(up != nil)
		iprint("cpu%d: registers for %s %lud\n",
			m->machno, up->text, up->pid);
	else
		iprint("cpu%d: registers for kernel\n", m->machno);

	delay(20);
	iprint("%#8.8lux\tr0\n", ureg->r0);
	iprint("%#8.8lux\tr1\n", ureg->r1);
	iprint("%#8.8lux\tr2\n", ureg->r2);
	delay(20);
	iprint("%#8.8lux\tr3\n", ureg->r3);
	iprint("%#8.8lux\tr4\n", ureg->r4);
	iprint("%#8.8lux\tr5\n", ureg->r5);
	delay(20);
	iprint("%#8.8lux\tr6\n", ureg->r6);
	iprint("%#8.8lux\tr7\n", ureg->r7);
	iprint("%#8.8lux\tr8\n", ureg->r8);
	delay(20);
	iprint("%#8.8lux\tr9 (up)\n", ureg->r9);
	iprint("%#8.8lux\tr10 (m)\n", ureg->r10);
	iprint("%#8.8lux\tr11 (loader temporary)\n", ureg->r11);
	iprint("%#8.8lux\tr12 (SB)\n", ureg->r12);
	delay(20);
	iprint("%#8.8lux\tr13 (sp)\n", ureg->r13);
	iprint("%#8.8lux\tr14 (link)\n", ureg->r14);
	iprint("%#8.8lux\tr15 (pc)\n", ureg->pc);
	delay(20);
	iprint("%10.10lud\ttype\n", ureg->type);
	iprint("%#8.8lux\tpsr\n", ureg->psr);
	delay(500);
}

void
dumpregs(Ureg* ureg)
{
	dumpgpr(ureg);
	dumpscr();
}

vlong
probeaddr(uintptr addr)
{
	ulong fetch;
	vlong v;

	ilock(&m->probelock);
	m->trapped = 0;
	m->probing = 1;
	coherence();

	/*
	 * this fetch may cause a fault, but it may be imprecise
	 * and asynchronous.
	 */
	fetch = *(volatile ulong *)addr;
	coherence();
	coherence();
	delay(1);

	m->probing = 0;
	if (m->trapped) {
		v = -1;
		m->trapped = 0;
	} else
		v = fetch;
	iunlock(&m->probelock);
	return v;
}
