#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	<tos.h>
#include	<ptrace.h>
#include	"ureg.h"

#include	"amd64.h"
#include	"io.h"

extern int notify(Ureg*);

static void debugbpt(Ureg*, void*);
static void faultgpf(Ureg*, void*);
static void faultamd64(Ureg*, void*);
static void doublefault(Ureg*, void*);
static void unexpected(Ureg*, void*);
static void dumpstackwithureg(Ureg*);

static Lock vctllock;
static Vctl *vctl[IdtMAX+1];

enum
{
	Ntimevec = 20		/* number of time buckets for each intr */
};
ulong intrtimes[IdtMAX+1][Ntimevec];

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

	v = malloc(sizeof(Vctl));
	v->isintr = 1;
	v->irq = irq;
	v->tbdf = tbdf;
	v->f = f;
	v->a = a;
	strncpy(v->name, name, KNAMELEN-1);
	v->name[KNAMELEN-1] = 0;

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
		if(vctl[v->vno]->isr != v->isr || vctl[v->vno]->eoi != v->eoi)
			panic("intrenable: handler: %s %s %#p %#p %#p %#p",
				vctl[v->vno]->name, v->name,
				vctl[v->vno]->isr, v->isr, vctl[v->vno]->eoi, v->eoi);
	}
	v->vno = vno;
	v->next = vctl[vno];
	vctl[vno] = v;
	iunlock(&vctllock);

	/*
	 * Return the assigned vector so intrdisable can find
	 * the handler; the IRQ is useless in the wonderful world
	 * of the IOAPIC.
	 */
	return v;
}

int
intrdisable(void* vector)
{
	Vctl *v, **vl;
	extern int ioapicintrdisable(int);

	ilock(&vctllock);
	v = vector;
	for(vl = &vctl[v->vno]; *vl != nil; vl = &(*vl)->next)
		if(*vl == v)
			break;
	if(*vl == nil)
		panic("intrdisable: v %#p", v);
	v->f(nil, v->a);
	*vl = v->next;
	ioapicintrdisable(v->vno);
	iunlock(&vctllock);

	free(v);

	return 0;
}

static long
irqallocread(Chan*, void *vbuf, long n, vlong offset)
{
	char *buf, *p, str[2*(11+1)+(8+1)+KNAMELEN+1+1];
	int ns, vno;
	long oldn;
	Vctl *v;

	if(n < 0 || offset < 0)
		error(Ebadarg);

	oldn = n;
	buf = vbuf;
	for(vno=0; vno<nelem(vctl); vno++){
		for(v=vctl[vno]; v; v=v->next){
			ns = snprint(str, sizeof str, "%11d %11d %-*.*s %.*s\n",
				vno, v->irq, 8, 8, v->type, KNAMELEN, v->name);
			if(ns <= offset)	/* if do not want this, skip entry */
				offset -= ns;
			else{
				/* skip offset bytes */
				ns -= offset;
				p = str+offset;
				offset = 0;

				/* write at most max(n,ns) bytes */
				if(ns > n)
					ns = n;
				memmove(buf, p, ns);
				n -= ns;
				buf += ns;

				if(n == 0)
					return oldn;
			}
		}
	}
	return oldn - n;
}

void
trapenable(int vno, void (*f)(Ureg*, void*), void* a, char *name)
{
	Vctl *v;

	if(vno < 0 || vno > IdtMAX)
		panic("trapenable: vno %d", vno);
	v = malloc(sizeof(Vctl));
	v->type = "trap";
	v->tbdf = BUSUNKNOWN;
	v->f = f;
	v->a = a;
	strncpy(v->name, name, KNAMELEN);
	v->name[KNAMELEN-1] = 0;

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
	outb(0x70, 0x80);		/* NMI latch clear */
	outb(0x70, 0);

	x = inb(0x61) & 0x07;		/* Enable NMI */
	outb(0x61, 0x08|x);
	outb(0x61, x);
}

void
trapinit(void)
{
	/*
	 * Need to set BPT interrupt gate - here or in vsvminit?
	 */
	/*
	 * Special traps.
	 * Syscall() is called directly without going through trap().
	 */
	trapenable(IdtBP, debugbpt, 0, "#BP");
	trapenable(IdtGP, faultgpf, 0, "#GP");
	trapenable(IdtPF, faultamd64, 0, "#PF");
	trapenable(IdtDF, doublefault, 0, "#DF");
	trapenable(Idt0F, unexpected, 0, "#15");
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

void (*pmcupdate)(void);

/* go to user space */
void
kexit(Ureg*)
{
	uvlong t;
	Tos *tos;

	/* performance counters */
	if(pmcupdate != nil)
		pmcupdate();

	/* precise time accounting, kernel exit */
	tos = (Tos*)(USTKTOP-sizeof(Tos));
	cycles(&t);
	tos->kcycles += t - up->kentry;
	tos->pcycles = up->pcycles;
	tos->pid = up->pid;
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
	int clockintr, vno, user;
	char buf[ERRMAX];
	Vctl *ctl, *v;
	void (*pt)(Proc*, int, vlong, vlong);

	m->perf.intrts = perfticks();
	user = userureg(ureg);
	if(user){
		up->dbgreg = ureg;
		cycles(&up->kentry);
	}
	/* performance counters */
	if(pmcupdate != nil)
		pmcupdate();

	clockintr = 0;

	vno = ureg->type;
	if(ctl = vctl[vno]){
		if(ctl->isintr){
			m->intr++;
			if(vno >= IdtPIC && vno != IdtSYSCALL)
				m->lastintr = ctl->irq;
		}else if(user && up->trace && (pt = proctrace) != nil)
			if(vno != IdtPF)
				pt(up, STrap, 0, vno);

		if(ctl->isr)
			ctl->isr(vno);
		for(v = ctl; v != nil; v = v->next){
			if(v->f)
				v->f(ureg, v->a);
		}
		if(ctl->eoi)
			ctl->eoi(vno);
		if(ctl->isintr){
			intrtime(m, vno);

			if(ctl->irq == IdtPIC+IrqCLOCK || ctl->irq == IdtTIMER)
				clockintr = 1;

			if(up && !clockintr)
				preempted();
		}
	}
	else if(vno < nelem(excname) && user){
		spllo();
		snprint(buf, sizeof buf, "sys: trap: %s", excname[vno]);
		postnote(up, 1, buf, NDebug);
	}
	else if(vno != 39){
		if(vno == IdtNMI){
			/*
			 * Don't re-enable, it confuses the crash dumps.
			nmienable();
			 */
			iprint("cpu%d: NMI PC %#llux\n", m->machno, ureg->ip);
			while(m->machno != 0)
				;
		}
		dumpregs(ureg);
#ifdef notdef
		if(!user){
			ureg->sp = PTR2UINT(&ureg->sp);
			dumpstackwithureg(ureg);
		}
		if(vno < nelem(excname))
			panic("%s", excname[vno]);
		panic("unknown trap/intr: %d", vno);
#else
		iprint("vno %d: buggeration @ %#p...\n", vno, ureg->ip);
		i8042reset();
#endif /* notdef */
	}
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
 */
static void
dumpgpr(Ureg* ureg)
{
	if(up != nil)
		iprint("cpu%d: registers for %s %d\n",
			m->machno, up->text, up->pid);
	else
		iprint("cpu%d: registers for kernel\n", m->machno);

	iprint("ax\t%#16.16llux\n", ureg->ax);
	iprint("bx\t%#16.16llux\n", ureg->bx);
	iprint("cx\t%#16.16llux\n", ureg->cx);
	iprint("dx\t%#16.16llux\n", ureg->dx);
	iprint("di\t%#16.16llux\n", ureg->di);
	iprint("si\t%#16.16llux\n", ureg->si);
	iprint("bp\t%#16.16llux\n", ureg->bp);
	iprint("r8\t%#16.16llux\n", ureg->r8);
	iprint("r9\t%#16.16llux\n", ureg->r9);
	iprint("r10\t%#16.16llux\n", ureg->r10);
	iprint("r11\t%#16.16llux\n", ureg->r11);
	iprint("r12\t%#16.16llux\n", ureg->r12);
	iprint("r13\t%#16.16llux\n", ureg->r13);
	iprint("r14\t%#16.16llux\n", ureg->r14);
	iprint("r15\t%#16.16llux\n", ureg->r15);
	iprint("ds  %#4.4ux   es  %#4.4ux   fs  %#4.4ux   gs  %#4.4ux\n",
		ureg->ds, ureg->es, ureg->fs, ureg->gs);
	iprint("type\t%#llux\n", ureg->type);
	iprint("error\t%#llux\n", ureg->error);
	iprint("pc\t%#llux\n", ureg->ip);
	iprint("cs\t%#llux\n", ureg->cs);
	iprint("flags\t%#llux\n", ureg->flags);
	iprint("sp\t%#llux\n", ureg->sp);
	iprint("ss\t%#llux\n", ureg->ss);
	iprint("type\t%#llux\n", ureg->type);

	iprint("m\t%#16.16p\nup\t%#16.16p\n", m, up);
}

void
dumpregs(Ureg* ureg)
{
	dumpgpr(ureg);

	/*
	 * Processor control registers.
	 * If machine check exception, time stamp counter, page size extensions
	 * or enhanced virtual 8086 mode extensions are supported, there is a
	 * CR4. If there is a CR4 and machine check extensions, read the machine
	 * check address and machine check type registers if RDMSR supported.
	 */
	iprint("cr0\t%#16.16llux\n", cr0get());
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
	ureg.sp = PTR2UINT(&fn);
	fn(&ureg);
}

static void
dumpstackwithureg(Ureg* ureg)
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
doublefault(Ureg*, void*)
{
	panic("double fault");
}

static void
unexpected(Ureg* ureg, void*)
{
	iprint("unexpected trap %llud; ignoring\n", ureg->type);
}

extern void mayberdmsr(void);
extern void rdmsrfail(void);

static void
faultgpf(Ureg* ureg, void*)
{
	char buf[ERRMAX];

	if (userureg(ureg)) {
		spllo();
		snprint(buf, sizeof buf, "sys: trap: %s", excname[IdtGP]);
		postnote(up, 1, buf, NDebug);
		return;
	}
	if(ureg->ip != (u64int)mayberdmsr)
		panic("unhandled GPF at 0x%016llux", ureg->ip);
	ureg->ip = (u64int)rdmsrfail;
}

static void
faultamd64(Ureg* ureg, void*)
{
	u64int addr, arg;
	int read, user, insyscall;
	char buf[ERRMAX];
	void (*pt)(Proc*, int, vlong, vlong);

	addr = cr2get();
	user = userureg(ureg);
//	if(!user && mmukmapsync(addr))
//		return;

	/*
	 * There must be a user context.
	 * If not, the usual problem is causing a fault during
	 * initialisation before the system is fully up.
	 */
	if(up == nil){
		panic("fault with up == nil; pc %#llux addr %#llux",
			ureg->ip, addr);
	}
	read = !(ureg->error & 2);

	if(up->trace && (pt = proctrace) != nil){
		if(read)
			arg = STrapRPF | (addr&STrapMask);
		else
			arg = STrapWPF | (addr&STrapMask);
		pt(up, STrap, 0, arg);
	}

	insyscall = up->insyscall;
	up->insyscall = 1;
	if(fault(addr, read) < 0){
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
			panic("fault: %#llux", addr);
		}
		snprint(buf, sizeof buf, "sys: trap: fault %s addr=%#llux",
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
	if(ureg == 0)
		return 0;

	return ureg->ip;
}
