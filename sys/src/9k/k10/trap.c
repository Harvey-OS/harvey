/*
 * traps, faults, system call entry
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	<tos.h>
#include	"ureg.h"
#include	"amd64.h"
#include	"io.h"

enum {				/* Ureg->err for page faults: causes */
	Pfp	= 1<<0,		/* page table entry Present bit */
	Pfwr	= 1<<1,		/* write (not read) access */
	Pfuser	= 1<<2,		/* user mode access */
	Pfrsvd	= 1<<3,		/* reserved page table bit set */
	Pfinst	= 1<<4,		/* instruction access */
};

/* syscall stack frame (on amd64 at least); used for notes */
typedef struct NFrame NFrame;
struct NFrame
{
	uintptr	ip;
	Ureg*	arg0;
	char*	arg1;
	char	msg[ERRMAX];
	Ureg*	old;
	Ureg	ureg;
};

void	lapicerror(Ureg*, void*);
void	lapicspurious(Ureg*, void*);

static void debugbpt(Ureg*, void*);
static void faultamd64(Ureg*, void*);
static void doublefault(Ureg*, void*);
static void gpf(Ureg *, void *);
static void unexpected(Ureg*, void*);
static void unwanted(Ureg*, void*);
static void dumpstackwithureg(Ureg*);

static Lock vctllock;
static Vctl *vctl[IdtMAX+1];

enum
{
	Pmc	= 0,		/* flag: call pmcupdate */
	Ntimevec = 20,		/* number of time buckets for each intr */
};
ulong intrtimes[IdtMAX+1][Ntimevec];

void (*pmcupdate)(void) = nop;

int
inop(int)
{
	return 0;
}

int
vecinuse(uint vec)
{
	return vec >= nelem(vctl) || vctl[vec] != 0;
}

Vctl *
newvec(void (*f)(Ureg*, void*), void* a, int tbdf, char *name)
{
	Vctl *v;

	v = malloc(sizeof(Vctl));
	v->tbdf = tbdf;
	v->f = f;
	v->a = a;
	v->cpu = -1;			/* could be any cpu until overridden */
	v->eoi = v->isr = inop;		/* always safe to call through these */
	strncpy(v->name, name, KNAMELEN);
	v->name[KNAMELEN-1] = 0;
	return v;
}

/* old 9k interface */
void*
intrenable(int irq, void (*f)(Ureg*, void*), void* a, int tbdf, char *name)
{
	int vno;
	Vctl *v;
	extern int ioapicintrenable(Vctl*);

	if(f == nil){
		print("intrenable: nil handler for %d, tbdf %#ux for %s\n",
			irq, tbdf, name);
		return nil;
	}

	v = newvec(f, a, tbdf, name);
	v->isintr = 1;
	v->irq = irq;

	ilock(&vctllock);
	vno = ioapicintrenable(v);
	if(vno == -1){
		iunlock(&vctllock);
		print("intrenable: couldn't enable irq %d, tbdf %#ux for %s\n",
			irq, tbdf, v->name);
		free(v);
		return nil;
	}
	if(vctl[vno]){
		iunlock(&vctllock);
		panic("vno %d for %s already allocated by %s",
			vno, v->name, vctl[vno]->name);
	}
	/* we assert that vectors are unshared, though irqs may be shared */
	v->vno = vno;
	vctl[vno] = v;
	iunlock(&vctllock);
	DBG("intrenable %s irq %d vector %d\n", name, irq, vno);

	/*
	 * Return the assigned vector so intrdisable can find
	 * the handler; the IRQ is useless in the wonderful world
	 * of the IOAPIC.
	 */
	return v;
}

/* new, portable interface (between 9 and 9k) */
int
enableintr(Intrcommon *ic, Intrsvcret (*f)(Ureg*, void*), void *ctlr, char *name)
{
	Pcidev *pcidev;

	pcidev = ic->pcidev;
	ic->vector = intrenable(ic->irq, f, ctlr,
		(pcidev? pcidev->tbdf: BUSUNKNOWN), name);
	if (ic->vector)
		ic->intrenabled = 1;
	return ic->vector? 0: -1;
}

int
intrdisable(void* vector)
{
	Vctl *v;
	extern int ioapicintrdisable(uint);

	ilock(&vctllock);
	v = vector;
	if(v == nil || vctl[v->vno] != v)
		panic("intrdisable: v %#p", v);
	ioapicintrdisable(v->vno);
	vctl[v->vno] = nil;
	iunlock(&vctllock);

	free(v);

	return 0;
}

/* new, portable interface (between 9 and 9k) */
int
disableintr(Intrcommon *ic, Intrsvcret (*)(Ureg*, void*), void *, char *)
{
	if (ic->vector == nil || ic->intrenabled == 0)
		return 0;
	if (intrdisable(ic->vector) < 0)
		return -1;
	ic->intrenabled = 0;
	ic->vector = nil;
	return 0;
}

static char *
vctlseprint(char *s, char *e, Vctl *v, int vno)
{
	s = seprint(s, e, "%3d %3d %10llud %3s %.*s", vno, v->irq, v->count,
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
			if(ns <= offset) { /* if do not want this, skip entry */
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
trapenable(int vno, void (*f)(Ureg*, void*), void* a, char *name)
{
	Vctl *v;

	if(vno < 0 || vno > IdtMAX)
		panic("trapenable: vno %d", vno);
	v = newvec(f, a, BUSUNKNOWN, name);
	v->isintr = 0;

	ilock(&vctllock);
	v->next = vctl[vno];
	vctl[vno] = v;
	iunlock(&vctllock);
}

void
trapinit(void)
{
	/*
	 * Special traps.
	 * Syscall() is called directly without going through trap().
	 */
	/*
	 * Need to set BPT interrupt gate - here or in vsvminit?
	 */
	trapenable(IdtBP, debugbpt, 0, "#BP");
	trapenable(IdtPF, faultamd64, 0, "#PF");
	trapenable(IdtDF, doublefault, 0, "#DF");
	trapenable(Idt0F, unexpected, 0, "#15");
	trapenable(IdtMC, unexpected, 0, "machine check");
	trapenable(IdtGP, gpf, 0, "GPF");
	trapenable(IdtSPURIOUS, lapicspurious, 0, "lapic spurious");
	trapenable(IdtERROR, lapicerror, 0, "lapic error");
	nmienable();

	addarchfile("irqalloc", 0444, irqallocread, nil);
}

static char* excname[32] = {
	"#DE",					/* Divide-by-Zero Error */
	"#DB",					/* Debug */
	"#NMI",					/* Non-Maskable-Interrupt */
	"#BP",					/* Breakpoint */
	"#OF",					/* Overflow */
	"#BR",					/* Bound-Range */
	"#UD",					/* Invalid-Opcode */
	"#NM",					/* Device-Not-Available */
	"#DF",					/* Double-Fault */
	"#9 (reserved)",
	"#TS",					/* Invalid-TSS */
	"#NP",					/* Segment-Not-Present */
	"#SS",					/* Stack */
	"#GP",					/* General-Protection */
	"#PF",					/* Page-Fault */
	"#15 (reserved)",
	"#MF",					/* x87 FPE-Pending */
	"#AC",					/* Alignment-Check */
	"#MC",					/* Machine-Check */
	"#XF",					/* SIMD Floating-Point */
	"#20 (reserved)",
	"#21 (reserved)",
	"#22 (reserved)",
	"#23 (reserved)",
	"#24 (reserved)",
	"#25 (reserved)",
	"#26 (reserved)",
	"#27 (reserved)",
	"#28 (reserved)",
	"#29 (reserved)",
	"#30 (reserved)",
	"#31 (reserved)",
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

	diff /= m->cpumhz*100;		/* quantum = 100µsec */
	if(diff >= Ntimevec)
		diff = Ntimevec-1;
	intrtimes[vno][diff]++;
}

/* prepare to go to user space by updating profiling counts */
void
kexit(Ureg *)
{
	uvlong t;
	Tos *tos;

	/* performance counters */
	if(Pmc)
		pmcupdate();

	if (up == nil)
		return;

	if(0 && up->nlocks)	/* TODO? extra debugging: see qlock, syscall */
		print("kexit: nlocks %d\n", up->nlocks);

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

/* ignore a few early traps */
static void
trapunassigned(Ureg *ureg, int vno)
{
	static int traps;

	apicisr(vno);
	ioapicintrtbloff(vno);
	if ((aadd(&traps, 1) > sys->nonline || sys->ticks > 60*HZ) &&
	    traps < 12) {
		iprint("cpu%d: intr for unassigned vector %d @ %#p, disabled\n",
			m->machno, vno, ureg->ip);
		vctl[vno] = nil;
	}
	apiceoi(vno);
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
		iprint("pid %d %s: unexpected user trap %s @ %#p\n",
			up->pid, up->text, name, ureg->ip);
		posttrapnote(vno, name);
	} else
		panic("unexpected kernel trap %s @ %#p", name, ureg->ip);
}

static void
unexpected(Ureg* ureg, void*)
{
	iprint("unexpected trap %llud; ignoring\n", ureg->type);
}

static void
unwanted(Ureg* ureg, void*)
{
	badtrap(ureg, ureg->type, userureg(ureg));
}

static void
gpf(Ureg* ureg, void*)
{
	uchar *inst;

	/*
	 * on a fault, pc points to the faulting instruction.
	 * rdmsr is 0x0f, 0x32.  wrmsr is 0x0f, 0x30.
	 */
	inst = (uchar *)ureg->pc;
	if (inst[0] == 0x0f && (inst[1] == 0x30 || inst[1] == 0x32)) {
		iprint("gpf: bad msr #%#llux in rd/wrmsr at %#p\n",
			ureg->cx, ureg->pc);
		if (inst[1] == 0x32)
			ureg->ax = ureg->dx = 0;
		ureg->pc += 2;			/* pretend it succeeded */
		return;
	}
	badtrap(ureg, ureg->type, userureg(ureg));
}

static void
trapunexpected(Ureg *ureg, uint vno, int user)
{
	dumpregs(ureg);
	if(!user){
		ureg->sp = (uintptr)&ureg->sp;
		dumpstackwithureg(ureg);
	}
	badtrap(ureg, vno, user);
}

static void
trapunknown(Ureg *ureg, int vno, int user)
{
	if (vno >= IdtIOAPIC && vno <= IdtMAX)
		/*
		 * In the range of vectors that we assign.
		 * We shouldn't get these.  Perhaps they are latched
		 * interrupts from the bootstrap, not cleared by disabling
		 * all PCI interrupts, legacy and MSI?  Maybe need to reset
		 * I/O APICs' redirection tables?
		 */
		trapunassigned(ureg, vno);
	else if(vno == IdtNMI){
		nmienable();
		if(m->machno == 0)
			panic("NMI @ %#p", ureg->ip);
		else {
			iprint("nmi: cpu%d: PC %#p\n", m->machno, ureg->ip);
			for(;;)
				idlehands();
		}
		notreached();
	} else if (1)				/* for apu, at least */
		trapunassigned(ureg, vno);
	else
		trapunexpected(ureg, vno, user);
}

/*
 *  All traps come here.  It is slower to have all traps call trap()
 *  rather than directly vectoring the handler.  However, this avoids a
 *  lot of code duplication and possible bugs.  The only exception is
 *  for a system call.
 *  Trap is called with interrupts disabled via interrupt-gates.
 */
void
trap(Ureg* ureg)
{
	int clockintr, vno, user, isintr, irq;
	void (*isr)(Ureg *, void *);
	Vctl *ctl, *v;

	m->perf.intrts = perfticks();
	user = userureg(ureg);
	if(user){
		up->dbgreg = ureg;
		cycles(&up->kentry);
	}
	/* performance counters */
	if(Pmc)
		pmcupdate();

	clockintr = 0;
	vno = ureg->type;
	if(ctl = vctl[vno]){
		irq = ctl->irq;
		if((isintr = ctl->isintr) != 0){
			m->intr++;
			if(vno >= IdtPIC && vno != IdtSYSCALL)
				m->lastintr = irq;
		}

		ctl->isr(vno);
		for(v = ctl; v != nil; v = v->next)
			if((isr = v->f) != nil) {
				isr(ureg, v->a);
				splhi();	/* in case v->f dropped PL */
				v->count++;
			} else
				iprint("trap: no isr for vector %d\n", vno);
		ctl->eoi(vno);
		if(isintr){
			intrtime(m, vno);

			/* in case the cpus all raced into mwait, always wake */
			if(irq == IdtPIC+IrqCLOCK || irq == IdtTIMER)
				clockintr = 1;
			else
				if(up)
					preempted();
			/*
			 * procs waiting for this interrupt could be on
			 * any cpu, so wake any mwaiting cpus.
			 */
			idlewake();
		}
	} else if (user)
		posttrapnote(vno, "GOK");
	else
		trapunknown(ureg, vno, user);
	splhi();

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
 * Dump general registers.
 * try to fit it on a cga screen (80x25).
 */
static void
dumpgpr(Ureg* ureg)
{
	if(up != nil)
		iprint("cpu%d: registers for %s %d\n",
			m->machno, up->text, up->pid);
	else
		iprint("cpu%d: registers for kernel\n", m->machno);

	iprint("ax\t%#16.16llux\t", ureg->ax);
	iprint("bx\t%#16.16llux\n", ureg->bx);
	iprint("cx\t%#16.16llux\t", ureg->cx);
	iprint("dx\t%#16.16llux\n", ureg->dx);
	iprint("di\t%#16.16llux\t", ureg->di);
	iprint("si\t%#16.16llux\n", ureg->si);
	iprint("bp\t%#16.16llux\t", ureg->bp);
	iprint("r8\t%#16.16llux\n", ureg->r8);
	iprint("r9\t%#16.16llux\t", ureg->r9);
	iprint("r10\t%#16.16llux\n", ureg->r10);
	iprint("r11\t%#16.16llux\t", ureg->r11);
	iprint("r12\t%#16.16llux\n", ureg->r12);
	iprint("r13\t%#16.16llux\t", ureg->r13);
	iprint("r14\t%#16.16llux\n", ureg->r14);
	iprint("r15\t%#16.16llux\n", ureg->r15);
	iprint("ds  %#4.4ux   es  %#4.4ux   fs  %#4.4ux   gs  %#4.4ux\n",
		ureg->ds, ureg->es, ureg->fs, ureg->gs);
	iprint("type\t%#llux\t", ureg->type);
	iprint("error\t%#llux\n", ureg->err);
	iprint("pc\t%#llux\t", ureg->ip);
	iprint("cs\t%#llux\n", ureg->cs);
	iprint("flags\t%#llux\t", ureg->flags);
	iprint("sp\t%#llux\n", ureg->sp);
	iprint("ss\t%#llux\t", ureg->ss);
	iprint("type\t%#llux\n", ureg->type);

	iprint("m\t%#16.16p\nup\t%#16.16p\n", m, up);
}

void
dumpregs(Ureg* ureg)
{
	if(getconf("*nodumpregs")){
		iprint("dumpregs disabled\n");
		return;
	}
	dumpgpr(ureg);

	/*
	 * Processor control registers.
	 * If machine check exception, time stamp counter, page size extensions
	 * or enhanced virtual 8086 mode extensions are supported, there is a
	 * CR4. If there is a CR4 and machine check extensions, read the machine
	 * check address and machine check type registers if RDMSR supported.
	 */
	iprint("cr0\t%#16.16llux\t", cr0get());
	iprint("cr2\t%#16.16llux\n", cr2get());
	iprint("cr3\t%#16.16llux\n", cr3get());

//	archdumpregs();
}

/*
 * Fill in enough of Ureg to get a stack trace, and call a function.
 * Used by debugging interface rdb.
 */
void
callwithureg(void (*fn)(Ureg*))
{
	Ureg ureg;

	ureg.ip = getcallerpc(&fn);
	ureg.sp = (uintptr)&fn;
	fn(&ureg);
}

static void
dumpstackwithureg(Ureg* ureg)
{
	uintptr l, v, i, estack;
	extern ulong etext;
	int x;

	if(ureg != nil)
		dumpregs(ureg);
	if(getconf("*nodumpstack")){
		iprint("dumpstack disabled\n");
		return;
	}
	iprint("dumpstack\n");

	x = 0;
	x += iprint("ktrace /kernel/path %#p %#p\n", ureg->ip, ureg->sp);
	i = 0;
	if(up != nil
//	&& (uintptr)&l >= (uintptr)up->kstack
	&& (uintptr)&l <= (uintptr)up->kstack+KSTACK)
		estack = (uintptr)up->kstack+KSTACK;
	else if((uintptr)&l >= m->stack && (uintptr)&l <= m->stack+MACHSTKSZ)
		estack = m->stack+MACHSTKSZ;
	else{
		if(up != nil)
			iprint("&up->kstack %#p &l %#p\n", up->kstack, &l);
		else
			iprint("&m %#p &l %#p\n", m, &l);
		return;
	}
	x += iprint("estackx %#p\n", estack);

	for(l = (uintptr)&l; l < estack; l += sizeof(uintptr)){
		v = *(uintptr*)l;
		if((KTZERO < v && v < (uintptr)&etext)
		|| ((uintptr)&l < v && v < estack) || estack-l < 256){
			x += iprint("%#16.16p=%#16.16p ", l, v);
			i++;
		}
		if(i == 2){
			i = 0;
			x += iprint("\n");
		}
	}
	if(i)
		iprint("\n");
}

void
dumpstack(void)
{
	callwithureg(dumpstackwithureg);
}

static void
debugbpt(Ureg* ureg, void*)
{
	char buf[ERRMAX];

	if(up == 0)
		panic("kernel bpt");
	/* restore pc to instruction that caused the trap */
	ureg->ip--;
	snprint(buf, sizeof buf, "sys: breakpoint");
	postnote(up, 1, buf, NDebug);
}

static void
doublefault(Ureg *ureg, void*)
{
	panic("double fault; pc %#llux", ureg->ip);
}

static void
faultamd64(Ureg* ureg, void*)
{
	uintptr addr;
	int read, user, insyscall;

	addr = cr2get();
	user = userureg(ureg);
//	if(!user && mmukmapsync(addr))
//		return;

	/*
	 * There must be a user context.
	 * If not, the usual problem is causing a fault during
	 * initialisation before the system is fully up.
	 */
	if(up == nil)
		panic("fault with up == nil; pc %#p addr %#p", ureg->ip, addr);
	read = !(ureg->err & Pfwr);

	insyscall = up->insyscall;
	up->insyscall = 1;
	if(fault(addr, read) < 0){
		char buf[ERRMAX];

		/*
		 * It is possible to get here with !user if, for example,
		 * a process was in a system call accessing a shared
		 * segment but was preempted by another process which shrunk
		 * or deallocated the shared segment; when the original
		 * process resumes it may fault while in kernel mode.
		 * No need to panic this case, post a note to the process
		 * and unwind the error stack. There must be an error stack
		 * (up->nerrlab != 0) if this is a system call, if not then
		 * the game's a bogey.
		 */
		if(!user && (!insyscall || up->nerrlab == 0)){
			dumpregs(ureg);
			panic("fault: addr %#p pc %#p", addr, ureg->ip);
		}
		snprint(buf, sizeof buf, "sys: trap: fault %s addr=%#p",
			read? "read": "write", addr);
		postnote(up, 1, buf, NDebug);
		if(insyscall)
			error(buf);
	}
	up->insyscall = insyscall;
}

/*
 *  return the userpc the last exception happened at
 */
uintptr
userpc(Ureg* ureg)
{
	if(ureg == nil)
		ureg = up->dbgreg;
	return ureg->ip;
}

/* This routine must save the values of registers the user is not permitted
 * to write from devproc and then restore the saved values before returning.
 */
void
setregisters(Ureg* ureg, char* pureg, char* uva, int n)
{
	u64int cs, flags, ss;
	u16int ds, es, fs, gs;

	ss = ureg->ss;
	flags = ureg->flags;
	cs = ureg->cs;
	gs = ureg->cs;
	fs = ureg->cs;
	es = ureg->cs;
	ds = ureg->cs;
	memmove(pureg, uva, n);
	ureg->ds = ds;
	ureg->es = es;
	ureg->fs = fs;
	ureg->gs = gs;
	ureg->cs = cs;
	ureg->flags = (ureg->flags & 0x00ff) | (flags & 0xff00);
	ureg->ss = ss;
}

/* Give enough context in the ureg to produce a kernel stack for
 * a sleeping process
 */
void
setkernur(Ureg* ureg, Proc* p)
{
	ureg->ip = p->sched.pc;
	ureg->sp = p->sched.sp+BY2SE;
}

uintptr
dbgpc(Proc *p)
{
	Ureg *ureg;

	ureg = p->dbgreg;
	return ureg? ureg->ip: 0;
}

/*
 * machine-specific system call and note code
 */

static void
vrfyuregaddrs(Ureg *nur)
{
	if(!okaddr(nur->ip, sizeof(ulong), 0) ||
	   !okaddr(nur->sp, sizeof(uintptr), 0)){
		qunlock(&up->debug);
		pprint("suicide: trap in noted pc=%#p sp=%#p\n",
			nur->ip, nur->sp);
		pexit("Suicide", 0);
	}
}

/*
 *   Return user to state before notify()
 */
void
noted(Ureg* cur, uintptr arg0)
{
	NFrame *nf;
	Note note;
	Ureg *nur;

	qlock(&up->debug);
	if(arg0 != NRSTR && !up->notified){
		qunlock(&up->debug);
		pprint("suicide: call to noted when not notified\n");
		pexit("Suicide", 0);
	}
	up->notified = 0;
	fpunoted();

	/* construct NFrame over ureg on stack */
	nf = up->ureg;

	/* sanity clause */
	if(!okaddr(PTR2UINT(nf), sizeof(NFrame), 0)){
		qunlock(&up->debug);
		pprint("suicide: bad ureg %#p in noted\n", nf);
		pexit("Suicide", 0);
	}

	/*
	 * Check the segment selectors are all valid.
	 * Only real machine dependency.
	 */
	nur = &nf->ureg;
	if(nur->cs != SSEL(SiUCS, SsRPL3) || nur->ss != SSEL(SiUDS, SsRPL3)
	|| nur->ds != SSEL(SiUDS, SsRPL3) || nur->es != SSEL(SiUDS, SsRPL3)
	|| nur->fs != SSEL(SiUDS, SsRPL3) || nur->gs != SSEL(SiUDS, SsRPL3)){
		qunlock(&up->debug);
		pprint("suicide: bad segment selector in noted\n");
		pexit("Suicide", 0);
	}

	/* don't let user change system flags */
	nur->flags &= (Of|Df|Sf|Zf|Af|Pf|Cf);
	nur->flags |= cur->flags & ~(Of|Df|Sf|Zf|Af|Pf|Cf);
	memmove(cur, nur, sizeof(Ureg));

	switch((int)arg0){
	case NCONT:
	case NRSTR:
		vrfyuregaddrs(cur);
		up->ureg = nf->old;
		qunlock(&up->debug);
		break;
	case NSAVE:
		vrfyuregaddrs(cur);
		qunlock(&up->debug);

		splhi();
		nf->arg1 = nf->msg;
		nf->arg0 = &nf->ureg;
		cur->bp = PTR2UINT(nf->arg0);
		nf->ip = 0;
		cur->sp = PTR2UINT(nf);
		break;
	default:
		memmove(&note, &up->lastnote, sizeof(Note));
		qunlock(&up->debug);
		pprint("suicide: bad arg %#p in noted: %s\n", arg0, note.msg);
		pexit(note.msg, 0);
		break;
	case NDFLT:
		memmove(&note, &up->lastnote, sizeof(Note));
		qunlock(&up->debug);
		if(note.flag == NDebug)
			pprint("suicide: %s\n", note.msg);
		pexit(note.msg, note.flag != NDebug);
		break;
	}
}

/*
 *  Call user, if necessary, with note.
 *  Pass user the Ureg struct and the note on his stack.
 */
int
notify(Ureg* ureg)
{
	int l;
	Mreg s;
	Note note;
	uintptr sp;
	NFrame *nf;

	if(up->procctl)
		procctl(up);
	if(up->nnote == 0)
		return 0;

	fpunotify(ureg);

	s = spllo();
	qlock(&up->debug);

	up->notepending = 0;
	memmove(&note, &up->note[0], sizeof(Note));
	if(strncmp(note.msg, "sys:", 4) == 0){
		l = strlen(note.msg);
		if(l > ERRMAX-sizeof(" pc=0x0123456789abcdef"))
			l = ERRMAX-sizeof(" pc=0x0123456789abcdef");
		seprint(note.msg+l, note.msg + ERRMAX, " pc=%#p", ureg->ip);
	}

	if(note.flag != NUser && (up->notified || up->notify == nil)){
		qunlock(&up->debug);
		if(note.flag == NDebug)
			pprint("suicide: %s\n", note.msg);
		pexit(note.msg, note.flag != NDebug);
	}

	if(up->notified){
		qunlock(&up->debug);
		splhi();
		return 0;
	}

	if(up->notify == nil){
		qunlock(&up->debug);
		pexit(note.msg, note.flag != NDebug);
	}
	if(!okaddr(PTR2UINT(up->notify), sizeof(ureg->ip), 0)){
		qunlock(&up->debug);
		pprint("suicide: bad function address %#p in notify\n",
			up->notify);
		pexit("Suicide", 0);
	}

	sp = ureg->sp - sizeof(NFrame);
	if(!okaddr(sp, sizeof(NFrame), 1)){
		qunlock(&up->debug);
		pprint("suicide: bad stack address %#p in notify\n", sp);
		pexit("Suicide", 0);
	}

	nf = UINT2PTR(sp);
	memmove(&nf->ureg, ureg, sizeof(Ureg));
	nf->old = up->ureg;
	up->ureg = nf;
	memmove(nf->msg, note.msg, ERRMAX);
	nf->arg1 = nf->msg;
	nf->arg0 = &nf->ureg;
	ureg->bp = PTR2UINT(nf->arg0);
	nf->ip = 0;

	ureg->sp = sp;
	ureg->ip = PTR2UINT(up->notify);
	up->notified = 1;
	up->nnote--;
	memmove(&up->lastnote, &note, sizeof(Note));
	memmove(&up->note[0], &up->note[1], up->nnote*sizeof(Note));

	qunlock(&up->debug);
	splx(s);

	return 1;
}

void
syscallamd(uint scallnr, Ureg* ureg)
{
	/*
	 * Last argument is location of return value in frame.
	 */
	syscall(scallnr, ureg, (Ar0 *)&ureg->ax);
}

void
sysrforkchild(Proc* child, Proc* parent)
{
	Ureg *cureg;

	cureg = (Ureg*)(child->kstack+KSTACK - sizeof(Ureg));
	/*
	 * Subtract 3*BY2SE from the stack to account for
	 *  - the return PC
	 *  - syscallamd's arguments (syscallnr, ureg)
	 * below Ureg at the top of kstack.
	 */
	child->sched.sp = PTR2UINT(cureg) - 3*BY2SE;
	child->sched.pc = PTR2UINT(sysrforkret);

	memmove(cureg, parent->dbgreg, sizeof(Ureg));

	/* Things from bottom of syscall which were never executed */
	child->psstate = 0;
	child->insyscall = 0;

	fpusysrforkchild(child, parent);
}
