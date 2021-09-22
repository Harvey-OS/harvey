#include	"u.h"
#include	"tos.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"mp.h"			/* for lapiceoi */
#include	"ureg.h"
#include	"../port/error.h"

#define isclockirq(irq) ((irq) == IrqCLOCK || (irq) == IrqTIMER)

enum
{
	/* tunable */
	Lotsintrs = 500000,
	Ntimevec = 20,		/* number of time buckets for each intr */

	Nirqs = MaxVectorAPIC+1,

	/* i/o ports */
	/* keyboard control B i/o port bits */
	 Ctlbchanchk=	1 << 3,	/* channel check status/enable */
	/*
	 *  real time clock and non-volatile ram
	 */
	Paddr=		NVRADDR,
	 Paddrnonmi=	0x80,
	Pdata=		NVRDATA,
};

static int trapinited;

void	noted(Ureg*, ulong);

static int debugbpt(Ureg*, void*);
static int gpf(Ureg*, void*);
static int fault386(Ureg*, void*);
static int doublefault(Ureg*, void*);
static int unexpected(Ureg*, void*);
static void badtrap(Ureg *ureg, uint vno, int user);
static void _dumpstack(Ureg*);

static Lock vctllock;
static Vctl *vctl[Nirqs];

/* not used aside from increment; examined manually, if at all */
#ifdef MEASURE
ulong intrtimes[Nirqs][Ntimevec];
#endif

/*
 * allocate & populate a vector for an isr (f).
 * irq, if any, is not known yet.
 */
static Vctl *
newvect(int (*f)(Ureg*, void*), void* a, char *name, int tbdf)
{
	Vctl *v;

	v = xalloc(sizeof(Vctl));
	v->f = f;
	v->a = a;
	strncpy(v->name, name, KNAMELEN-1);
	v->name[KNAMELEN-1] = 0;
	v->tbdf = tbdf;
	v->isintr = v->ismsi = v->irq = 0;	/* defaults */
	v->cpu = v->lapic = -1;			/* not yet known */
	/* v->isr and v->eoi remain unset (zero) */
	return v;
}

/*
 * Old interface.  Return -1 or the chosen vector number.
 * Note careful insertion of a new vector so that an interrupt for that vector
 * occurring on another cpu will always see a consistent linked list.
 */
int
intrenable(int irq, int (*f)(Ureg*, void*), void* a, int tbdf, char *name)
{
	int vno;
	Vctl *v, *vsh;

	if(f == nil){
		print("intrenable: nil handler for %d, tbdf %T for %s\n",
			irq, tbdf, name);
		return -1;
	}

	v = newvect(f, a, name, tbdf);
	v->isintr = 1;
	v->irq = irq;

	ilock(&vctllock);
	vno = arch->intrenable(v);
	if(vno == -1){
		iunlock(&vctllock);
		print("intrenable: couldn't enable irq %d, tbdf %T for %s\n",
			irq, tbdf, v->name);
		xfree(v);
		return -1;
	}
	vsh = vctl[vno];		/* head of list for this intr. */
	if(vsh != nil){			/* shared vector? */
		if(vsh->isr != v->isr || vsh->eoi != v->eoi)
			panic("intrenable: handler: %s %s %#p %#p %#p %#p",
				vsh->name, v->name, vsh->isr, v->isr,
				vsh->eoi, v->eoi);
		v->next = vsh;
	}
	vctl[vno] = v;
	iunlock(&vctllock);
	return vno;
}

/* new, portable interface (between 9 and 9k) */
int
enableintr(Intrcommon *ic, Intrsvcret (*f)(Ureg*, void*), void *ctlr, char *name)
{
	Pcidev *pcidev;

	pcidev = ic->pcidev;
	ic->irq = intrenable(ic->irq, f, ctlr, (pcidev? pcidev->tbdf: 0), name);
	if (ic->irq >= 0)
		ic->intrenabled = 1;
	return ic->irq;
}

/* old 9 interface */
int
intrdisable(int irq, int (*f)(Ureg *, void *), void *a, int tbdf, char *name)
{
	Vctl **pv, *v;
	int vno;

	/*
	 * For now, none of this will work with the APIC code,
	 * there is no mapping between irq and vector as the IRQ
	 * is pretty meaningless.
	 */
	if(arch->intrvecno == nil)
		return -1;
	vno = arch->intrvecno(irq);
	ilock(&vctllock);
	pv = &vctl[vno];
	while (*pv &&
	    ((*pv)->irq != irq || (*pv)->tbdf != tbdf ||
	     (*pv)->f != f || (*pv)->a != a ||
	     strcmp((*pv)->name, name) != 0))
		pv = &((*pv)->next);
	/*
	 * *pv is now sometimes nil at reboot on soekris 5501.
	 * if there's no match, don't mess with Vctls.
	 */
	v = nil;
	if (*pv) {
		v = *pv;
		*pv = (*pv)->next;	/* Link out the entry */
	}

	if(vctl[vno] == nil && arch->intrdisable != nil)
		arch->intrdisable(irq);
	iunlock(&vctllock);
	if (v)
		xfree(v);
	return 0;
}

/* new, portable interface (between 9 and 9k) */
int
disableintr(Intrcommon *ic, Intrsvcret (*f)(Ureg*, void*), void *ctlr, char *name)
{
	Pcidev *pcidev;

	if (ic->intrenabled == 0)
		return 0;
	pcidev = ic->pcidev;
	if (intrdisable(ic->irq, f, ctlr, (pcidev? pcidev->tbdf: 0), name) < 0)
		return -1;
	ic->intrenabled = 0;
	return 0;
}

static char *
vctlseprint(char *s, char *e, Vctl *v, int vno)
{
	s = seprint(s, e, "%3d %3d %10lud %3s %.*s", vno, v->irq, v->count,
		v->ismsi? "msi": "- ", KNAMELEN, v->name);
	if (v->unclaimed || v->intrunknown)
		s = seprint(s, e, " unclaimed %lud unknown %lud", v->unclaimed,
			v->intrunknown);
	if(v->cpu >= 0)
		s = seprint(s, e, " cpu%d", v->cpu);
	if(BUSTYPE(v->tbdf) == BusPCI && v->lapic >= 0)
		s = seprint(s, e, " lapic %d", v->lapic);
	return seprint(s, e, "\n");
}

static long
irqallocread(Chan*, void *vbuf, long n, vlong offset)
{
	char *buf, *p, *e, str[90];
	int ns, vno;
	long oldn;
	Vctl *v;

	if(n < 0 || offset < 0)
		error(Ebadarg);

	oldn = n;
	buf = vbuf;
	e = str + sizeof str;
	for(vno=0; vno<nelem(vctl); vno++)
		for(v=vctl[vno]; v; v=v->next){
			/* v is a trap, not yet seen?  adjust to taste */
			if (v->isintr == 0 && v->count == 0)
				continue;
			ns = vctlseprint(str, e, v, vno) - str;
			if(ns <= offset) {  /* if do not want this, skip entry */
				offset -= ns;
				continue;
			}
			/* skip offset bytes */
			ns -= offset;
			p = str+offset;
			offset = 0;

			/* write at most min(n,ns) bytes */
			if(ns > n)
				ns = n;
			memmove(buf, p, ns);
			n -= ns;
			buf += ns;

			if(n == 0)
				return oldn;
		}
	return oldn - n;
}

void
trapenable(int vno, int (*f)(Ureg*, void*), void* a, char *name)
{
	Vctl *v;

	if(vno < 0 || vno >= VectorPIC)
		panic("trapenable: vno %d", vno);
	v = newvect(f, a, name, BUSUNKNOWN);

	ilock(&vctllock);
	v->next = vctl[vno];
	vctl[vno] = v;
	iunlock(&vctllock);
}

static void
nmienable(void)
{
	int x;

	/*
	 * Hack: should be locked with NVRAM access.
	 */
	outb(Paddr, Paddrnonmi);	/* NMI latch clear */
	outb(Paddr, 0);

	x = inb(KBDCTLB) & 0x07;	/* Enable NMI */
	outb(KBDCTLB, Ctlbchanchk|x);
	outb(KBDCTLB, x);
}

/*
 * Minimal trap setup.  Just enough so that we can panic
 * on traps (bugs) during kernel initialization.
 * Called very early - malloc is not yet available.
 */
void
trapinit0(void)
{
	int d1, v;
	ulong vaddr;
	Segdesc *idt;

	idt = (Segdesc*)IDTADDR;
	vaddr = (ulong)vectortable;
	for(v = 0; v < 256; v++){
		d1 = (vaddr & 0xFFFF0000)|SEGP;
		switch(v){

		case VectorBPT:
		case VectorSYSCALL:
			d1 |= SEGPL(3)|SEGIG;
			break;

		default:
			d1 |= SEGPL(0)|SEGIG;
			break;
		}
		idt[v].d0 = (vaddr & 0xFFFF)|(KESEL<<16);
		idt[v].d1 = d1;
		vaddr += 6;		/* size of CALL _strayintr; BYTE $n */
	}
}

void
trapinit(void)
{
	/*
	 * Special traps.
	 * Syscall() is called directly without going through trap().
	 */
	trapenable(VectorBPT, debugbpt, 0, "debugpt");
	trapenable(VectorPF, fault386, 0, "fault386");
	trapenable(Vector2F, doublefault, 0, "doublefault");
	trapenable(Vector15, unexpected, 0, "unexpected");
	trapenable(VectorGPF, gpf, 0, "GPF");
	nmienable();

	addarchfile("irqalloc", 0444, irqallocread, nil);
	trapinited = 1;
}

static char* excname[32] = {
	"divide error",
	"debug exception",
	"nonmaskable interrupt",
	"breakpoint",
	"overflow",
	"bounds check",
	"invalid opcode",
	"coprocessor not available",
	"double fault",
	"coprocessor segment overrun",
	"invalid TSS",
	"segment not present",
	"stack exception",
	"general protection violation",
	"page fault",
	"15 (reserved)",
	"coprocessor error",
	"alignment check",
	"machine check",
	"19 (reserved)",
	"20 (reserved)",
	"21 (reserved)",
	"22 (reserved)",
	"23 (reserved)",
	"24 (reserved)",
	"25 (reserved)",
	"26 (reserved)",
	"27 (reserved)",
	"28 (reserved)",
	"29 (reserved)",
	"30 (reserved)",
	"31 (reserved)",
};

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
	diff /= m->cpumhz*100;		/* quantum = 100µsec */
	if(diff >= Ntimevec)
		diff = Ntimevec-1;
	intrtimes[vno][diff]++;
#else
	USED(vno);
#endif
}

/* prepare to go to user space */
void
kexit(Ureg*)
{
	uvlong t;
	Tos *tos;

	/* precise time accounting, kernel exit */
	tos = (Tos*)(USTKTOP-sizeof(Tos));
	cycles(&t);
	tos->kcycles += t - up->kentry;
	tos->pcycles = up->pcycles;
	tos->pid = up->pid;
}

static void
posttrapnote(uint vno, char *name)
{
	char buf[ERRMAX];

	spllo();
	snprint(buf, sizeof buf, "sys: trap: %s",
		vno < nelem(excname)? excname[vno]: name);
	postnote(up, 1, buf, NDebug);
}

static void
badtrap(Ureg *ureg, uint vno, int user)
{
	char *name;
	char buf[32];

	if(vno < nelem(excname))
		name = excname[vno];
	else if (vno < nelem(vctl) && vctl[vno])
		name = vctl[vno]->name;
	else {
		snprint(buf, sizeof buf, "vector #%d", vno);
		name = buf;
	}
	if (user) {
		iprint("pid %ld %s: unexpected user trap %s @ %#p\n",
			up->pid, up->text, name, ureg->pc);
		posttrapnote(vno, name);
	} else
		panic("unexpected kernel trap %s @ %#p", name, ureg->pc);
}

static int
gpf(Ureg* ureg, void*)
{
	uchar *inst;

	/*
	 * on a fault, pc points to the faulting instruction.
	 * rdmsr is 0x0f, 0x32.  wrmsr is 0x0f, 0x30.
	 */
	inst = (uchar *)ureg->pc;
	if (inst[0] == 0x0f && (inst[1] == 0x30 || inst[1] == 0x32)) {
		iprint("gpf: bad msr #%#lux in rd/wrmsr at %#p\n",
			ureg->cx, ureg->pc);
		if (inst[1] == 0x32)
			ureg->ax = ureg->dx = 0;
		ureg->pc += 2;			/* pretend it succeeded */
		return 0;
	}
	badtrap(ureg, VectorGPF, userureg(ureg));
	return 0;
}

static void
trapunknown(Ureg *ureg, int vno, int user)
{
	if(vno == VectorNMI){
		/*
		 * Don't re-enable, it confuses the crash dumps.
		nmienable();
		 */
		iprint("cpu%d: NMI PC %#8.8lux\n", m->machno, ureg->pc);
		while(m->machno != 0)
			;
	}
	dumpregs(ureg);
	if(!user){
		ureg->sp = (ulong)&ureg->sp;
		_dumpstack(ureg);
	}
	if(vno < nelem(excname))
		panic("%s", excname[vno]);
	panic("unknown trap/intr: %d", vno);
}

/* mainly for interrupt routing debugging */
static void
checkcpu(Vctl *v)
{
	static int whined;

	if (v->cpu != m->machno && v->isintr && !isclockirq(v->irq) &&
	    v->cpu >= 0 && ++whined <= 4)
		/*
		 * not fatal.  we could be polling to find an
		 * isr for an unclaimed interrupt.
		 */
		iprint("%s: intr on cpu%d, expected on cpu%d\n",
			v->name, m->machno, v->cpu);
}

/*
 * interrupt service routines now return non-zero to indicate `interrupt was
 * for me'.  it might not be for me if devices share an irq (e.g., orig. pci).
 *
 * call at splhi; returns number of isrs claiming the interrupt.
 */
static int
callvectisrs(Ureg *ureg, Vctl *ctl)
{
	int accted, forme;
	Vctl *v;
	int (*isr)(Ureg*, void*);

	accted = 0;
	for(v = ctl; v != nil; v = v->next) {
		isr = v->f;
		if (isr == nil)
			continue;
		forme = isr(ureg, v->a);
		/*
		 * in case isr erroneously lowered PL.  happens during shutdown,
		 * at least.
		 */
		splhi();
		if (forme) {			/* precise acct'ng */
//			checkcpu(v);
			accted++;
			ainc((long *)&v->count);
		}
	}
	return accted;
}

/*
 * call all interrupt routines with same irq, just in case.
 * caller must call eoi.
 */
static int
intrsameirq(Ureg *ureg, int irq)
{
	int i, accted;
	Vctl *v;

	accted = 0;
	for(i = VectorPIC; i <= MaxVectorAPIC; i++){
		v = vctl[i];
		if(v && v->isintr && v->irq == irq)
			accted += callvectisrs(ureg, v);
			/* don't call v->eoi; irq doesn't matter to lapic */
	}
	return accted;
}

static void
incintrunk(Vctl *v)
{
	if (v && ainc((long *)&v->unclaimed) == Lotsintrs)
		iprint("%s: %ud unclaimed intrs\n", v->name, Lotsintrs);
}

/*
 * Called on an unknown interrupt.
 * This can happen when
 * the IRQ input goes away before the acknowledge.
 * In this case, a 'default IRQ7' is generated, but
 * the corresponding bit in the ISR isn't set.
 * In fact, just ignore all such interrupts.
 */
static void
intrunknown(Ureg *ureg, int vno)
{
	int i, isrs, accted, machno;
	Vctl *v, *vcpu;
	Mach *mach;

	/* call all interrupt routines, just in case */
	accted = 0;
	machno = m->machno;
	for(i = VectorPIC; i <= MaxVectorAPIC; i++){
		v = vctl[i];
		if(v == nil || !v->isintr)
			continue;
		if (i == vno && v->name)
			iprint("intrunknown %d %s\n", vno, v->name);
		isrs = callvectisrs(ureg, v);
		accted += isrs;
		if(isrs)
			/* could it have been for this vector & isr? */
			if (v->cpu == machno)
				incintrunk(v);
	}
	/* clear the interrupt */
	if (vno >= VectorPIC && vno <= VectorPIC+MaxIrqPIC)
		i8259isr(vno);	/* harmless if vno can't be a 8259 intr. */
	else
		lapiceoi(vno);

	if (accted)
		return;		/* claimed, so not truly spurious */

	/* this cpu must be the source of this interrupt */
	vcpu = nil;
	for(i = VectorPIC; i <= MaxIrqLAPIC; i++)
		for (vcpu = vctl[i]; vcpu != nil; vcpu = vcpu->next)
			if (vcpu->cpu == machno)
				break;		/* could have been vcpu */
	if (vcpu)
		incintrunk(vcpu);

	m->spuriousintr++;
	iprint("cpu%d: spurious interrupt %d, last %d\n",
		machno, vno, m->lastintr);
	if(conf.nmach > 1) {
		for(i = 0; i < MAXMACH; i++){
			if(!iscpuactive(i))
				continue;
			mach = MACHP(i);
			if(machno == mach->machno)
				continue;
			iprint("  cpu%d: last %d", mach->machno, mach->lastintr);
		}
		print("\n");
	}
}

/*
 * current interrupt is unclaimed, but this cpu must be its source.
 * charge it to an isr on this cpu.  return chosen Vctl*.
 */
static Vctl *
intrunclaimacct(Vctl *ctl, int vno)
{
	Vctl *vcpu;

	for (vcpu = vctl[vno]; vcpu != nil; vcpu = vcpu->next)
		if (vcpu->cpu == m->machno)
			break;		/* could have been vcpu */
	if (vcpu == nil)
		vcpu = ctl;	/* no idea, fall back to default */
	if (ainc((long *)&vcpu->unclaimed) == Lotsintrs)
		iprint("%s: %ud unclaimed intrs\n", vcpu->name, Lotsintrs);
	return vcpu;
}

static void
trapisrs(Ureg *ureg, int vno)
{
	Vctl *v;

	/* dismiss intr early if appropriate (e.g., edge-triggered) */
	v = vctl[vno];
	if(v->isr)
		v->isr(vno);
	callvectisrs(ureg, v);
	if(vno != IrqSPURIOUS && v->eoi)
		v->eoi(vno);			/* dismiss intr late instead */
}

/*
 * service known traps, including interrupts.
 * return true for clock interrupts only.
 */
static int
trapknown(Ureg *ureg, int vno)
{
	int clockintr, irq;
	Vctl *v, *vcpu;

	v = vctl[vno];
	if (!v->isintr) {
		trapisrs(ureg, vno);
		return 0;
	}

	/* it's an interrupt, not some other sort of trap */
	m->intr++;
	irq = v->irq;
	clockintr = isclockirq(irq);
	if(vno >= VectorPIC && vno != VectorSYSCALL && !clockintr)
		m->lastintr = irq;

	/* dismiss intr early if appropriate (e.g., edge-triggered) */
	if(v->isr)
		v->isr(vno);
	if (callvectisrs(ureg, v) == 0) {
		vcpu = intrunclaimacct(v, vno);
		if (!clockintr && intrsameirq(ureg, irq) == 0)
			incintrunk(vcpu);
	}
	if(vno != IrqSPURIOUS && v->eoi)
		v->eoi(vno);			/* dismiss intr late instead */

	intrtime(m, vno);
	if(!clockintr) {
		if(up)
			preempted();
		/*
		 * may be other runnable procs now, wake mwaiting cpus.
		 */
		lockwake();
	}
	return clockintr;
}

static void
traptooearly(Ureg *ureg)
{
	/* fault386 can give a better error message */
	if(ureg->trap == VectorPF)
		fault386(ureg, nil);
	panic("trap %lud: not ready", ureg->trap);
}

/*
 *  All traps come here.  It is slower to have all traps call trap()
 *  rather than directly vectoring the handler.  However, this avoids a
 *  lot of code duplication and possible bugs.  The only exception is
 *  VectorSYSCALL.
 *  Trap is called with interrupts disabled via interrupt-gates.
 */
void
trap(Ureg* ureg)
{
	int user, clockintr, vno;

	if(!trapinited)
		traptooearly(ureg);

	m->perf.intrts = perfticks();		/* for intrtime in trapknown */
	user = (ureg->cs & 0xFFFF) == UESEL;
	if(user){
		up->dbgreg = ureg;
		cycles(&up->kentry);
	}
	vno = (uchar)ureg->trap;
	if(vctl[vno] != nil)		/* vector allocated for trap vno? */
		clockintr = trapknown(ureg, vno);	/* common case */
	else if(vno < nelem(excname) && user){
		char buf[ERRMAX];

		clockintr = 0;
		spllo();
		snprint(buf, sizeof buf, "sys: trap: %s", excname[vno]);
		postnote(up, 1, buf, NDebug);
	} else if(vno >= VectorPIC && vno != VectorSYSCALL){
		intrunknown(ureg, vno);		/* no isr registered */
		if(user)
			kexit(ureg);
		return;
	} else {
		trapunknown(ureg, vno, user);		/* no return */
		return;					/* convince compiler */
	}
	splhi();

	/* to do: shouldn't notify be called first, so we won't spin? */

	/* delaysched set because we held a lock or because our quantum ended */
	if(up && up->delaysched && clockintr){
		sched();
		splhi();
	}

	if(user){
		if(up->procctl || up->nnote)
			notify(ureg);
		kexit(ureg);
	}
}

/*
 *  dump registers
 */
void
dumpregs2(Ureg* ureg)
{
	if(up)
		iprint("cpu%d: registers for %s %lud\n",
			m->machno, up->text, up->pid);
	else
		iprint("cpu%d: registers for kernel\n", m->machno);
	iprint("FLAGS=%luX TRAP=%luX ECODE=%luX PC=%luX",
		ureg->flags, ureg->trap, ureg->ecode, ureg->pc);
	iprint(" SS=%4.4luX USP=%luX\n", ureg->ss & 0xFFFF, ureg->usp);
	iprint("  AX %8.8luX  BX %8.8luX  CX %8.8luX  DX %8.8luX\n",
		ureg->ax, ureg->bx, ureg->cx, ureg->dx);
	iprint("  SI %8.8luX  DI %8.8luX  BP %8.8luX\n",
		ureg->si, ureg->di, ureg->bp);
	iprint("  CS %4.4luX  DS %4.4luX  ES %4.4luX  FS %4.4luX  GS %4.4luX\n",
		ureg->cs & 0xFFFF, ureg->ds & 0xFFFF, ureg->es & 0xFFFF,
		ureg->fs & 0xFFFF, ureg->gs & 0xFFFF);
}

void
dumpregs(Ureg* ureg)
{
	vlong mca, mct;

	dumpregs2(ureg);

	/*
	 * Processor control registers.
	 * If machine check exception, time stamp counter, page size extensions
	 * or enhanced virtual 8086 mode extensions are supported, there is a
	 * CR4. If there is a CR4 and machine check extensions, read the machine
	 * check address and machine check type registers if RDMSR supported.
	 */
	iprint("  CR0 %8.8lux CR2 %8.8lux CR3 %8.8lux",
		getcr0(), getcr2(), getcr3());
	if(m->cpuiddx & (Mce|Tsc|Pse|Vmex)){
		iprint(" CR4 %8.8lux", getcr4());
		if((m->cpuiddx & (Mce|Cpumsr)) == (Mce|Cpumsr)){
			rdmsr(Msrmcaddr, &mca);
			rdmsr(Msrmctype, &mct);
			iprint("\n  MCA %8.8llux MCT %8.8llux", mca, mct);
		}
	}
	iprint("\n  ur %#p up %#p\n", ureg, up);
}


/*
 * Fill in enough of Ureg to get a stack trace, and call a function.
 * Used by debugging interface rdb.
 */
void
callwithureg(void (*fn)(Ureg*))
{
	Ureg ureg;
	ureg.pc = getcallerpc(&fn);
	ureg.sp = (ulong)&fn;
	fn(&ureg);
}

static void
_dumpstack(Ureg *ureg)
{
	uintptr l, v, i, estack;
	extern ulong etext;
	int x;
	char *s;

	if((s = getconf("*nodumpstack")) != nil && strcmp(s, "0") != 0){
		iprint("dumpstack disabled\n");
		return;
	}
	iprint("dumpstack\n");

	x = 0;
	x += iprint("ktrace /kernel/path %.8lux %.8lux <<EOF\n", ureg->pc, ureg->sp);
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
		if((KTZERO < v && v < (uintptr)&etext) || estack-l < 32){
			/*
			 * Could Pick off general CALL (((uchar*)v)[-5] == 0xE8)
			 * and CALL indirect through AX
			 * (((uchar*)v)[-2] == 0xFF && ((uchar*)v)[-2] == 0xD0),
			 * but this is too clever and misses faulting address.
			 */
			x += iprint("%.8p=%.8p ", l, v);
			i++;
		}
		if(i == 4){
			i = 0;
			x += iprint("\n");
		}
	}
	if(i)
		iprint("\n");
	iprint("EOF\n");

	if(ureg->trap != VectorNMI)
		return;

	i = 0;
	for(l = (uintptr)&l; l < estack; l += sizeof(uintptr)){
		iprint("%.8p ", *(uintptr*)l);
		if(++i == 8){
			i = 0;
			iprint("\n");
		}
	}
	if(i)
		iprint("\n");
}

void
dumpstack(void)
{
	callwithureg(_dumpstack);
}

static int
debugbpt(Ureg* ureg, void*)
{
	char buf[ERRMAX];

	if(up == 0)
		panic("kernel bpt");
	/* restore pc to instruction that caused the trap */
	ureg->pc--;
	snprint(buf, sizeof buf, "sys: breakpoint");
	postnote(up, 1, buf, NDebug);
	return Intrtrap;
}

static int
doublefault(Ureg*, void*)
{
	panic("double fault");
	return Intrtrap;
}

static int
unexpected(Ureg* ureg, void*)
{
	iprint("unexpected trap %lud; ignoring\n", ureg->trap);
	return Intrtrap;
}

extern void checkpages(void);
extern void checkfault(ulong, ulong);

static int
fault386(Ureg* ureg, void*)
{
	ulong addr;
	int read, user, n, insyscall;

	addr = getcr2();
	read = !(ureg->ecode & 2);

	user = (ureg->cs & 0xFFFF) == UESEL;
	if(!user){
		if(vmapsync(addr))
			return Intrtrap;
		if(addr >= USTKTOP)
			panic("kernel fault: bad address pc=%#.8lux addr=%#.8lux", ureg->pc, addr);
		if(up == nil)
			panic("kernel fault: no user process pc=%#.8lux addr=%#.8lux", ureg->pc, addr);
	}
	if(up == nil)
		panic("user fault: up=0 pc=%#.8lux addr=%#.8lux", ureg->pc, addr);

	insyscall = up->insyscall;
	up->insyscall = 1;
	n = fault(addr, read);
	if(n < 0){
		char buf[ERRMAX];

		if(!user){
			dumpregs(ureg);
			panic("fault: kernel addr %#lux", addr);
		}
		checkpages();
		checkfault(addr, ureg->pc);
		snprint(buf, sizeof buf, "sys: trap: fault %s addr=%#lux",
			read ? "read" : "write", addr);
		postnote(up, 1, buf, NDebug);
	}
	up->insyscall = insyscall;
	return Intrtrap;
}

long
execregs(ulong entry, ulong ssize, ulong nargs)
{
	ulong *sp;
	Ureg *ureg;

	up->fpstate = FPinit;
	fpoff();

	sp = (ulong*)(USTKTOP - ssize);
	*--sp = nargs;

	ureg = up->dbgreg;
	ureg->usp = (ulong)sp;
	ureg->pc = entry;
	return USTKTOP-sizeof(Tos);	/* address of kernel/user shared data */
}

/*
 *  return the userpc the last exception happened at
 */
ulong
userpc(void)
{
	Ureg *ureg;

	ureg = (Ureg*)up->dbgreg;
	return ureg->pc;
}

/* This routine must save the values of registers the user is not permitted
 * to write from devproc and then restore the saved values before returning.
 */
void
setregisters(Ureg* ureg, char* pureg, char* uva, int n)
{
	ulong cs, ds, es, flags, fs, gs, ss;

	ss = ureg->ss;
	flags = ureg->flags;
	cs = ureg->cs;
	ds = ureg->ds;
	es = ureg->es;
	fs = ureg->fs;
	gs = ureg->gs;
	memmove(pureg, uva, n);
	ureg->gs = gs;
	ureg->fs = fs;
	ureg->es = es;
	ureg->ds = ds;
	ureg->cs = cs;
	/* 0xff00 is nested task, iopl, overflow, dir'n, intr enable, trap */
	ureg->flags = (ureg->flags & 0x00FF) | (flags & 0xFF00);
	ureg->ss = ss;
}

static void
linkproc(void)
{
	spllo();
	up->kpfun(up->kparg);
	pexit("kproc dying", 0);
}

void
kprocchild(Proc* p, void (*func)(void*), void* arg)
{
	/*
	 * gotolabel() needs a word on the stack in
	 * which to place the return PC used to jump
	 * to linkproc().
	 */
	p->sched.pc = (ulong)linkproc;
	p->sched.sp = (ulong)p->kstack+KSTACK-BY2WD;

	p->kpfun = func;
	p->kparg = arg;
}

void
forkchild(Proc *p, Ureg *ureg)
{
	Ureg *cureg;

	/*
	 * Add 2*BY2WD to the stack to account for
	 *  - the return PC
	 *  - trap's argument (ur)
	 */
	p->sched.sp = (ulong)p->kstack+KSTACK-(sizeof(Ureg)+2*BY2WD);
	p->sched.pc = (ulong)forkret;

	cureg = (Ureg*)(p->sched.sp+2*BY2WD);
	memmove(cureg, ureg, sizeof(Ureg));
	/* return value of syscall in child */
	cureg->ax = 0;

	/* Things from bottom of syscall which were never executed */
	p->psstate = 0;
	p->insyscall = 0;
}

/* Give enough context in the ureg to produce a kernel stack for
 * a sleeping process
 */
void
setkernur(Ureg* ureg, Proc* p)
{
	ureg->pc = p->sched.pc;
	ureg->sp = p->sched.sp+BY2WD;
}

ulong
dbgpc(Proc *p)
{
	Ureg *ureg;

	ureg = p->dbgreg;
	if(ureg == 0)
		return 0;

	return ureg->pc;
}
