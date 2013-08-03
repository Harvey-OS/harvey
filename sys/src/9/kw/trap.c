/*
 * sheevaplug traps, exceptions, interrupts, system calls.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"

#include "arm.h"

enum {
	Debug = 0,

	Ntimevec = 20,			/* # of time buckets for each intr */
	Nvecs = 256,
};

extern int notify(Ureg*);

extern int ldrexvalid;

typedef struct Vctl Vctl;
typedef struct Vctl {
	Vctl*	next;		/* handlers on this vector */
	char	*name;		/* of driver, xallocated */
	void	(*f)(Ureg*, void*);	/* handler to call */
	void*	a;		/* argument to call it with */
} Vctl;

static Lock vctllock;
static Vctl* vctl[32];

uvlong ninterrupt;
uvlong ninterruptticks;
ulong intrtimes[Nvecs][Ntimevec];

typedef struct Handler Handler;
struct Handler {
	void	(*r)(Ureg*, void*);
	void	*a;
	char	name[KNAMELEN];
};

static Handler irqlo[32];
static Handler irqhi[32];
static Handler irqbridge[32];
static Lock irqlock;
static int probing, trapped;

typedef struct Irq Irq;
struct Irq {
	ulong	*irq;
	ulong	*irqmask;
	Handler	*irqvec;
	int	nirqvec;
	char	*name;
};
/* irq and irqmask are filled in by trapinit */
static Irq irqs[] = {
[Irqlo]		{nil, nil, irqlo,	nelem(irqlo),	"lo"},
[Irqhi]		{nil, nil, irqhi,	nelem(irqhi),	"hi"},
[Irqbridge]	{nil, nil, irqbridge,	nelem(irqbridge), "bridge"},
};

/*
 *  keep histogram of interrupt service times
 */
void
intrtime(Mach*, int vno)
{
	ulong diff, x;

	if (m == nil)
		return;
	x = perfticks();
	diff = x - m->perf.intrts;
	m->perf.intrts = x;

	m->perf.inintr += diff;
	if(up == nil && m->perf.inidle > diff)
		m->perf.inidle -= diff;

	if (m->cpuhz == 0)			/* not set yet? */
		return;
	diff /= (m->cpuhz/1000000)*100;		/* quantum = 100µsec */
	if(diff >= Ntimevec)
		diff = Ntimevec-1;
	assert(vno >= 0 && vno < Nvecs);
	intrtimes[vno][diff]++;
}

void
intrfmtcounts(char *s, char *se)
{
	USED(s, se);
}

static void
dumpcounts(void)
{
}

void
intrclear(int sort, int v)
{
	*irqs[sort].irq = ~(1 << v);
}

void
intrmask(int sort, int v)
{
	*irqs[sort].irqmask &= ~(1 << v);
}

void
intrunmask(int sort, int v)
{
	*irqs[sort].irqmask |= 1 << v;
}

static void
maskallints(void)
{
	CpucsReg *cpu = (CpucsReg *)soc.cpu;
	IntrReg *intr;

	/* no fiq or ep in use */
	intr = (IntrReg *)soc.intr;
	intr->lo.irqmask = 0;
	intr->hi.irqmask = 0;
	cpu->irqmask = 0;
	coherence();
}

void
intrset(Handler *h, void (*f)(Ureg*, void*), void *a, char *name)
{
	if(h->r != nil) {
//		iprint("duplicate irq: %s (%#p)\n", h->name, h->r);
		return;
	}
	h->r = f;
	h->a = a;
	strncpy(h->name, name, KNAMELEN-1);
	h->name[KNAMELEN-1] = 0;
}

void
intrunset(Handler *h)
{
	h->r = nil;
	h->a = nil;
	h->name[0] = 0;
}

void
intrdel(Handler *h, void (*f)(Ureg*, void*), void *a, char *name)
{
	if(h->r != f || h->a != a || strcmp(h->name, name) != 0)
		return;
	intrunset(h);
}

void
intrenable(int sort, int v, void (*f)(Ureg*, void*), void *a, char *name)
{
//iprint("enabling intr %d vec %d for %s\n", sort, v, name);
	ilock(&irqlock);
	intrset(&irqs[sort].irqvec[v], f, a, name);
	intrunmask(sort, v);
	iunlock(&irqlock);
}

void
intrdisable(int sort, int v, void (*f)(Ureg*, void*), void* a, char *name)
{
	ilock(&irqlock);
	intrdel(&irqs[sort].irqvec[v], f, a, name);
	intrmask(sort, v);
	iunlock(&irqlock);
}

/*
 *  called by trap to handle interrupts
 */
static void
intrs(Ureg *ur, int sort)
{
	int i, s;
	ulong ibits;
	Handler *h;
	Irq irq;

	assert(sort >= 0 && sort < nelem(irqs));
	irq = irqs[sort];
	ibits = *irq.irq;
	ibits &= *irq.irqmask;

	for(i = 0; i < irq.nirqvec && ibits; i++)
		if(ibits & (1<<i)){
			h = &irq.irqvec[i];
			if(h->r != nil){
				h->r(ur, h->a);
				splhi();
				intrtime(m, sort*32 + i);
				if (sort == Irqbridge && i == IRQcputimer0)
					m->inclockintr = 1;
				ibits &= ~(1<<i);
			}
		}
	if(ibits != 0) {
		iprint("spurious irq%s interrupt: %8.8lux\n", irq.name, ibits);
		s = splfhi();
		*irq.irq &= ibits;
		*irq.irqmask &= ~ibits;
		splx(s);
	}
}

void
intrhi(Ureg *ureg, void*)
{
	intrs(ureg, Irqhi);
}

void
intrbridge(Ureg *ureg, void*)
{
	intrs(ureg, Irqbridge);
	intrclear(Irqlo, IRQ0bridge);
}

void
trapinit(void)
{
	int i;
	CpucsReg *cpu;
	IntrReg *intr;
	Vectorpage *page0 = (Vectorpage*)HVECTORS;

	intr = (IntrReg *)soc.intr;
	cpu = (CpucsReg *)soc.cpu;
	irqs[Irqlo].irq = &intr->lo.irq;
	irqs[Irqlo].irqmask = &intr->lo.irqmask;
	irqs[Irqhi].irq = &intr->hi.irq;
	irqs[Irqhi].irqmask = &intr->hi.irqmask;
	irqs[Irqbridge].irq = &cpu->irq;
	irqs[Irqbridge].irqmask = &cpu->irqmask;
	coherence();

	setr13(PsrMfiq, m->fiqstack + nelem(m->fiqstack));
	setr13(PsrMirq, m->irqstack + nelem(m->irqstack));
	setr13(PsrMabt, m->abtstack + nelem(m->abtstack));
	setr13(PsrMund, m->undstack + nelem(m->undstack));

	memmove(page0->vectors, vectors, sizeof page0->vectors);
	memmove(page0->vtable,  vtable,  sizeof page0->vtable);
	cacheuwbinv();
	l2cacheuwbinv();

	cpu->cpucfg &= ~Cfgvecinithi;

	for(i = 0; i < nelem(irqlo); i++)
		intrunset(&irqlo[i]);
	for(i = 0; i < nelem(irqhi); i++)
		intrunset(&irqhi[i]);
	for(i = 0; i < nelem(irqbridge); i++)
		intrunset(&irqbridge[i]);

	/* disable all interrupts */
	intr->lo.fiqmask = intr->hi.fiqmask = 0;
	intr->lo.irqmask = intr->hi.irqmask = 0;
	intr->lo.epmask =  intr->hi.epmask = 0;
	cpu->irqmask = 0;
	coherence();

	/* clear interrupts */
	intr->lo.irq = intr->hi.irq = ~0;
	cpu->irq = ~0;
	coherence();

	intrenable(Irqlo, IRQ0hisum, intrhi, nil, "hi");
	intrenable(Irqlo, IRQ0bridge, intrbridge, nil, "bridge");

	/* enable watchdog & access-error interrupts */
	cpu->irqmask |= 1 << IRQcputimerwd | 1 << IRQaccesserr;
	coherence();
}

static char *trapnames[PsrMask+1] = {
	[ PsrMusr ] "user mode",
	[ PsrMfiq ] "fiq interrupt",
	[ PsrMirq ] "irq interrupt",
	[ PsrMsvc ] "svc/swi exception",
	[ PsrMabt ] "prefetch abort/data abort",
	[ PsrMabt+1 ] "data abort",
	[ PsrMund ] "undefined instruction",
	[ PsrMsys ] "sys trap",
};

static char *
trapname(int psr)
{
	char *s;

	s = trapnames[psr & PsrMask];
	if(s == nil)
		s = "unknown trap number in psr";
	return s;
}

/* this is quite helpful during mmu and cache debugging */
static void
ckfaultstuck(uintptr va)
{
	static int cnt, lastpid;
	static uintptr lastva;

	if (va == lastva && up->pid == lastpid) {
		++cnt;
		if (cnt >= 2)
			/* fault() isn't fixing the underlying cause */
			panic("fault: %d consecutive faults for va %#p",
				cnt+1, va);
	} else {
		cnt = 0;
		lastva = va;
		lastpid = up->pid;
	}
}

/*
 *  called by trap to handle access faults
 */
static void
faultarm(Ureg *ureg, uintptr va, int user, int read)
{
	int n, insyscall;
	char buf[ERRMAX];
	static int cnt, lastpid;
	static ulong lastva;

	if(up == nil) {
		dumpregs(ureg);
		panic("fault: nil up in faultarm, accessing %#p", va);
	}
	insyscall = up->insyscall;
	up->insyscall = 1;
	if (Debug)
		ckfaultstuck(va);

	n = fault(va, read);
	if(n < 0){
		if(!user){
			dumpregs(ureg);
			panic("fault: kernel accessing %#p", va);
		}
		/* don't dump registers; programs suicide all the time */
		snprint(buf, sizeof buf, "sys: trap: fault %s va=%#p",
			read? "read": "write", va);
		postnote(up, 1, buf, NDebug);
	}
	up->insyscall = insyscall;
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

void
trap(Ureg *ureg)
{
	int user, x, rv, rem;
	ulong inst;
	u32int fsr;
	uintptr va;
	char buf[ERRMAX];

	if(up != nil)
		rem = (char*)ureg - up->kstack;
	else
		rem = (char*)ureg - ((char*)m + sizeof(Mach));
	if(rem < 256) {
		dumpstack();
		panic("trap %d bytes remaining, up %#p ureg %#p at pc %#lux",
			rem, up, ureg, ureg->pc);
	}

	user = (ureg->psr & PsrMask) == PsrMusr;
	if(user){
		up->dbgreg = ureg;
		cycles(&up->kentry);
	}

	if(ureg->type == PsrMabt+1)
		ureg->pc -= 8;
	else
		ureg->pc -= 4;

	m->inclockintr = 0;
	switch(ureg->type) {
	default:
		panic("unknown trap %ld", ureg->type);
		break;
	case PsrMirq:
		ldrexvalid = 0;
		// splflo();		/* allow fast interrupts */
		intrs(ureg, Irqlo);
		m->intr++;
		break;
	case PsrMabt:			/* prefetch fault */
		ldrexvalid = 0;
		faultarm(ureg, ureg->pc, user, 1);
		if(up->nnote == 0 &&
		   (*(u32int*)ureg->pc & ~(0xF<<28)) == 0x01200070)
			postnote(up, 1, "sys: breakpoint", NDebug);
		break;
	case PsrMabt+1:			/* data fault */
		ldrexvalid = 0;
		va = farget();
		inst = *(ulong*)(ureg->pc);
		fsr = fsrget() & 0xf;
		if (probing && !user) {
			if (trapped++ > 0)
				panic("trap: recursive probe %#lux", va);
			ureg->pc += 4;	/* continue at next instruction */
			break;
		}
		switch(fsr){
		case 0x0:
			panic("vector exception at %#lux", ureg->pc);
			break;
		case 0x1:
		case 0x3:
			if(user){
				snprint(buf, sizeof buf,
					"sys: alignment: pc %#lux va %#p\n",
					ureg->pc, va);
				postnote(up, 1, buf, NDebug);
			} else
				panic("kernel alignment: pc %#lux va %#p", ureg->pc, va);
			break;
		case 0x2:
			panic("terminal exception at %#lux", ureg->pc);
			break;
		case 0x4:
		case 0x6:
		case 0x8:
		case 0xa:
		case 0xc:
		case 0xe:
			panic("external abort %#ux pc %#lux addr %#px",
				fsr, ureg->pc, va);
			break;
		case 0x5:		/* translation fault, no section entry */
		case 0x7:		/* translation fault, no page entry */
			faultarm(ureg, va, user, !writetomem(inst));
			break;
		case 0x9:
		case 0xb:
			/* domain fault, accessing something we shouldn't */
			if(user){
				snprint(buf, sizeof buf,
					"sys: access violation: pc %#lux va %#p\n",
					ureg->pc, va);
				postnote(up, 1, buf, NDebug);
			} else
				panic("kernel access violation: pc %#lux va %#p",
					ureg->pc, va);
			break;
		case 0xd:
		case 0xf:
			/* permission error, copy on write or real permission error */
			faultarm(ureg, va, user, !writetomem(inst));
			break;
		}
		break;
	case PsrMund:	/* undefined instruction */
		if(user){
			if(seg(up, ureg->pc, 0) != nil &&
			   (*(u32int*)ureg->pc & ~(0xF<<28)) == 0x01200070)
				postnote(up, 1, "sys: breakpoint", NDebug);
			else{
				/* look for floating point instructions to interpret */
				x = spllo();
				rv = fpiarm(ureg);
				splx(x);
				if(rv == 0){
					ldrexvalid = 0;
					snprint(buf, sizeof buf,
						"undefined instruction: pc %#lux",
						ureg->pc);
					postnote(up, 1, buf, NDebug);
				}
			}
		}else{
			iprint("undefined instruction: pc %#lux inst %#ux\n",
				ureg->pc, ((u32int*)ureg->pc)[-2]);
			panic("undefined instruction");
		}
		break;
	}
	splhi();

	/* delaysched set because we held a lock or because our quantum ended */
	if(up && up->delaysched && m->inclockintr){
		ldrexvalid = 0;
		sched();
		splhi();
	}

	if(user){
		if(up->procctl || up->nnote)
			notify(ureg);
		kexit(ureg);
	}
}

int
isvalidaddr(void *v)
{
	return (uintptr)v >= KZERO;
}

void
dumplongs(char *msg, ulong *v, int n)
{
	int i, l;

	l = 0;
	iprint("%s at %.8p: ", msg, v);
	for(i=0; i<n; i++){
		if(l >= 4){
			iprint("\n    %.8p: ", v);
			l = 0;
		}
		if(isvalidaddr(v)){
			iprint(" %.8lux", *v++);
			l++;
		}else{
			iprint(" invalid");
			break;
		}
	}
	iprint("\n");
}

static void
dumpstackwithureg(Ureg *ureg)
{
	uintptr l, i, v, estack;
	u32int *p;

	iprint("ktrace /kernel/path %#.8lux %#.8lux %#.8lux # pc, sp, link\n",
		ureg->pc, ureg->sp, ureg->r14);
	delay(2000);
	i = 0;
	if(up != nil && (uintptr)&l <= (uintptr)up->kstack+KSTACK)
		estack = (uintptr)up->kstack+KSTACK;
	else if((uintptr)&l >= (uintptr)m->stack
	     && (uintptr)&l <= (uintptr)m+MACHSIZE)
		estack = (uintptr)m+MACHSIZE;
	else{
		if(up != nil)
			iprint("&up->kstack %#p &l %#p\n", up->kstack, &l);
		else
			iprint("&m %#p &l %#p\n", m, &l);
		return;
	}
	for(l = (uintptr)&l; l < estack; l += sizeof(uintptr)){
		v = *(uintptr*)l;
		if(KTZERO < v && v < (uintptr)etext && !(v & 3)){
			v -= sizeof(u32int);		/* back up an instr */
			p = (u32int*)v;
			if((*p & 0x0f000000) == 0x0b000000){	/* BL instr? */
				iprint("%#8.8lux=%#8.8lux ", l, v);
				i++;
			}
		}
		if(i == 4){
			i = 0;
			iprint("\n");
		}
	}
	if(i)
		iprint("\n");
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
	ureg.sp = PTR2UINT(&fn);
	fn(&ureg);
}

void
dumpstack(void)
{
	callwithureg(dumpstackwithureg);
}

void
dumpregs(Ureg* ureg)
{
	int s;

	if (ureg == nil) {
		iprint("trap: no user process\n");
		return;
	}
	s = splhi();
	iprint("trap: %s", trapname(ureg->type));
	if(ureg != nil && (ureg->psr & PsrMask) != PsrMsvc)
		iprint(" in %s", trapname(ureg->psr));
	iprint("\n");
	iprint("psr %8.8lux type %2.2lux pc %8.8lux link %8.8lux\n",
		ureg->psr, ureg->type, ureg->pc, ureg->link);
	iprint("R14 %8.8lux R13 %8.8lux R12 %8.8lux R11 %8.8lux R10 %8.8lux\n",
		ureg->r14, ureg->r13, ureg->r12, ureg->r11, ureg->r10);
	iprint("R9  %8.8lux R8  %8.8lux R7  %8.8lux R6  %8.8lux R5  %8.8lux\n",
		ureg->r9, ureg->r8, ureg->r7, ureg->r6, ureg->r5);
	iprint("R4  %8.8lux R3  %8.8lux R2  %8.8lux R1  %8.8lux R0  %8.8lux\n",
		ureg->r4, ureg->r3, ureg->r2, ureg->r1, ureg->r0);
	iprint("stack is at %#p\n", ureg);
	iprint("pc %#lux link %#lux\n", ureg->pc, ureg->link);

	if(up)
		iprint("user stack: %#p-%#p\n", up->kstack, up->kstack+KSTACK-4);
	else
		iprint("kernel stack: %8.8lux-%8.8lux\n",
			(ulong)(m+1), (ulong)m+BY2PG-4);
	dumplongs("stack", (ulong *)(ureg + 1), 16);
	delay(2000);
	dumpstack();
	splx(s);
}

void
idlehands(void)
{
	extern void _idlehands(void);

	_idlehands();
}

/* assumes that addr is already mapped suitable (e.g., by mmuidmap) */
vlong
probeaddr(uintptr addr)
{
	vlong v;
	static Lock fltlck;

	ilock(&fltlck);
	trapped = 0;
	probing = 1;
	coherence();

	v = *(ulong *)addr;	/* this may cause a fault (okay under ilock) */
	USED(probing);
	coherence();

	probing = 0;
	coherence();
	if (trapped)
		v = -1;
	iunlock(&fltlck);
	return v;
}
