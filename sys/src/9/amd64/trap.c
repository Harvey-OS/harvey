/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	<tos.h>
#include	"ureg.h"
#include	"../port/pmc.h"

#include	"io.h"
#include	"amd64.h"


// counters. Set by assembly code.
// interrupt enter and exit, systecm call enter and exit.
unsigned long ire, irx, sce, scx;
// Did we start doing an exit for the interrupts?
// ir exit entry :-)
unsigned long irxe;

extern int notify(Ureg*);

static void debugbpt(Ureg*, void*);
static void faultamd64(Ureg*, void*);
static void doublefault(Ureg*, void*);
static void unexpected(Ureg*, void*);
static void expected(Ureg*, void*);
static void dumpstackwithureg(Ureg*);
extern int bus_irq_setup(Vctl*);

static Lock vctllock;
static Vctl *vctl[256];

typedef struct Intrtime Intrtime;
struct Intrtime {
	uint64_t	count;
	uint64_t	cycles;
};
static Intrtime intrtimes[256];

void*
intrenable(int irq, void (*f)(Ureg*, void*), void* a, int tbdf, char *name)
{
	int vno;
	Vctl *v;
	if(f == nil){
		print("intrenable: nil handler for %d, tbdf %#x for %s\n",
			irq, tbdf, name);
		return nil;
	}

	v = malloc(sizeof(Vctl));
	v->isintr = 1;
	v->Vkey.irq = irq;
	v->Vkey.tbdf = tbdf;
	v->f = f;
	v->a = a;
	strncpy(v->name, name, KNAMELEN-1);
	v->name[KNAMELEN-1] = 0;

	ilock(&vctllock);
	vno = bus_irq_setup(v);
	if(vno == -1){
		iunlock(&vctllock);
		print("intrenable: couldn't enable irq %d, tbdf %#x for %s\n",
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

	if(v->mask)
		v->mask(&v->Vkey, 0);

	/*
	 * Return the assigned vector so intrdisable can find
	 * the handler; the IRQ is useless in the wonderful world
	 * of the IOAPIC.
	 */
	return v;
}

int acpiintrenable(Vctl *v)
{
	int vno;
	ilock(&vctllock);
	vno = bus_irq_setup(v);
	if(vno == -1){
		iunlock(&vctllock);
		print("intrenable: couldn't enable irq %d, tbdf %#x for %s\n",
			v->Vkey.irq, v->Vkey.tbdf, v->name);
		free(v);
		return -1;
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

	if(v->mask)
		v->mask(&v->Vkey, 0);

	return -1;
}

int
intrdisable(void* vector)
{
	Vctl *v, *x, **ll;
	extern int ioapicintrdisable(int);

	ilock(&vctllock);
	v = vector;
	if(v == nil || vctl[v->vno] != v)
		panic("intrdisable: v %#p", v);
	for(ll = vctl+v->vno; x = *ll; ll = &x->next)
		if(v == x)
			break;
	if(x != v)
		panic("intrdisable: v %#p", v);
	if(v->mask)
		v->mask(&v->Vkey, 1);
	v->f(nil, v->a);
	*ll = v->next;
	ioapicintrdisable(v->vno);
	iunlock(&vctllock);

	free(v);
	return 0;
}

static int32_t
irqmapread(Chan* c, void *vbuf, int32_t n, int64_t offset)
{
	char *readtodo(void);
	return readstr(offset, vbuf, n, readtodo());
}

static int32_t
irqmapwrite(Chan* c, void *buf, int32_t n, int64_t offset)
{
	int acpiirq(uint32_t tbdf, int irq);
	int t, b, d, f, irq;
	int *p[] = {&t, &b, &d, &f, &irq};
	Cmdbuf *cb;

	cb = parsecmd(buf, n);
	if (cb->nf < nelem(p))
		error("iprqmapwrite t b d f irq");
	for(int i = 0; i < nelem(p); i++)
		*p[i] = strtoul(cb->f[i], 0, 0);

	acpiirq(MKBUS(t, b, d, f), irq);
	return -1;
}

static int32_t
irqenablewrite(Chan* c, void *vbuf, int32_t n, int64_t offset)
{
	void irqenable(void);
	irqenable();
	return n;
}

static int32_t
irqallocread(Chan* c, void *vbuf, int32_t n, int64_t offset)
{
	char *buf, *p, str[2*(11+1)+2*(20+1)+(KNAMELEN+1)+(8+1)+1];
	int m, vno;
	int32_t oldn;
	Intrtime *t;
	Vctl *v;

	if(n < 0 || offset < 0)
		error(Ebadarg);

	oldn = n;
	buf = vbuf;
	for(vno=0; vno<nelem(vctl); vno++){
		for(v=vctl[vno]; v; v=v->next){
			t = intrtimes + vno;
			m = snprint(str, sizeof str, "%11d %11d %20llu %20llu %-*.*s %.*s\n",
				vno, v->Vkey.irq, t->count, t->cycles, 8, 8, v->type, KNAMELEN, v->name);
			if(m <= offset)	/* if do not want this, skip entry */
				offset -= m;
			else{
				/* skip offset bytes */
				m -= offset;
				p = str+offset;
				offset = 0;

				/* write at most max(n,m) bytes */
				if(m > n)
					m = n;
				memmove(buf, p, m);
				n -= m;
				buf += m;

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

	if(vno < 0 || vno >= 256)
		panic("trapenable: vno %d\n", vno);
	v = malloc(sizeof(Vctl));
	v->type = "trap";
	v->Vkey.tbdf = BUSUNKNOWN;
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
	trapenable(VectorBPT, debugbpt, 0, "#BP");
	trapenable(VectorPF, faultamd64, 0, "#PF");
	trapenable(Vector2F, doublefault, 0, "#DF");
	intrenable(IdtIPI, expected, 0, BUSUNKNOWN, "#IPI");
	trapenable(Vector15, unexpected, 0, "#15");
	nmienable();

	addarchfile("irqalloc", 0444, irqallocread, nil);
	addarchfile("irqmap", 0666, irqmapread, irqmapwrite);
	addarchfile("irqenable", 0222, nil, irqenablewrite);
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
 *  keep interrupt service times and counts
 */
void
intrtime(int vno)
{
	Proc *up = externup();
	uint64_t diff, x;

	x = perfticks();
	diff = x - machp()->perf.intrts;
	machp()->perf.intrts = x;

	machp()->perf.inintr += diff;
	if(up == nil && machp()->perf.inidle > diff)
		machp()->perf.inidle -= diff;

	intrtimes[vno].cycles += diff;
	intrtimes[vno].count++;
}

static void
pmcnop(Mach *m)
{
}

void (*_pmcupdate)(Mach *m) = pmcnop;

/* go to user space */
void
kexit(Ureg* u)
{
 	Proc *up = externup();
 	uint64_t t;
	Tos *tos;
	Mach *mp;

	/*
	 * precise time accounting, kernel exit
	 * initialized in exec, sysproc.c
	 */
	tos = (Tos*)(USTKTOP-sizeof(Tos));
	cycles(&t);
	tos->kcycles += t - up->kentry;
	tos->pcycles = up->pcycles;
	tos->pid = up->pid;
	if (up->ac != nil)
		mp = up->ac;
	else
		mp = machp();
	tos->core = mp->machno;
	tos->nixtype = mp->NIX.nixtype;
	//_pmcupdate(m);
	/*
	 * The process may change its core.
	 * Be sure it has the right cyclefreq.
	 */
	tos->cyclefreq = mp->cyclefreq;
	/* thread local storage */
	wrmsr(FSbase, up->tls);
}

void
kstackok(void)
{
	Proc *up = externup();

	if(up == nil){
		uintptr_t *stk = (uintptr_t*)machp()->stack;
		if(*stk != STACKGUARD)
			panic("trap: mach %d machstk went through bottom %p\n", machp()->machno, machp()->stack);
	} else {
		uintptr_t *stk = (uintptr_t*)up->kstack;
		if(*stk != STACKGUARD)
			panic("trap: proc %d kstack went through bottom %p\n", up->pid, up->kstack);
	}
}

void
_trap(Ureg *ureg)
{

	/*
	 * If it's a real trap in this core, then we want to
	 * use the hardware cr2 register.
	 * We cannot do this in trap() because application cores
	 * would update m->cr2 with their cr2 values upon page faults,
	 * and then call trap().
	 * If we do this in trap(), we would overwrite that with our own cr2.
	 */
	if(ureg->type == VectorPF)
		machp()->MMU.cr2 = cr2get();
	trap(ureg);
}

static int lastvno;
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
	int clockintr, vno, user;
	// cache the previous vno to see what might be causing
	// trouble
	vno = ureg->type;
	uint64_t gsbase = rdmsr(GSbase);
	//if (sce > scx) iprint("====================");
	lastvno = vno;
	if (gsbase < 1ULL<<63)
		die("bogus gsbase");
	Proc *up = externup();
	char buf[ERRMAX];
	Vctl *ctl, *v;

	machp()->perf.intrts = perfticks();
	user = userureg(ureg);
	if(user && (machp()->NIX.nixtype == NIXTC)){
		up->dbgreg = ureg;
		cycles(&up->kentry);
	}

	clockintr = 0;

	//_pmcupdate(machp());

	if(ctl = vctl[vno]){
		if(ctl->isintr){
			machp()->intr++;
			if(vno >= VectorPIC && vno != VectorSYSCALL)
				machp()->lastintr = ctl->Vkey.irq;
		}else
			if(up)
				up->nqtrap++;

		if(ctl->isr){
			ctl->isr(vno);
			if(islo())print("trap %d: isr %p enabled interrupts\n", vno, ctl->isr);
		}
		for(v = ctl; v != nil; v = v->next){
			if(v->f){
				v->f(ureg, v->a);
				if(islo())print("trap %d: ctlf %p enabled interrupts\n", vno, v->f);
			}
		}
		if(ctl->eoi){
			ctl->eoi(vno);
			if(islo())print("trap %d: eoi %p enabled interrupts\n", vno, ctl->eoi);
		}

		intrtime(vno);
		if(ctl->isintr){
			if(ctl->Vkey.irq == IrqCLOCK || ctl->Vkey.irq == IrqTIMER)
				clockintr = 1;

			if (ctl->Vkey.irq == IrqTIMER)
				oprof_alarm_handler(ureg);

			if(up && !clockintr)
				preempted();
		}
	}
	else if(vno < nelem(excname) && user){
		spllo();
		snprint(buf, sizeof buf, "sys: trap: %s", excname[vno]);
		postnote(up, 1, buf, NDebug);
	}
	else if(vno >= VectorPIC && vno != VectorSYSCALL){
		/*
		 * An unknown interrupt.
		 * Check for a default IRQ7. This can happen when
		 * the IRQ input goes away before the acknowledge.
		 * In this case, a 'default IRQ7' is generated, but
		 * the corresponding bit in the ISR isn't set.
		 * In fact, just ignore all such interrupts.
		 */

		/* clear the interrupt */
		i8259isr(vno);

		iprint("cpu%d: spurious interrupt %d, last %d\n",
			machp()->machno, vno, machp()->lastintr);
		intrtime(vno);
		if(user)
			kexit(ureg);
		return;
	}
	else{
		if(vno == VectorNMI){
			nmienable();
			if(machp()->machno != 0){
				iprint("cpu%d: PC %#llx\n",
					machp()->machno, ureg->ip);
				for(;;);
			}
		}
		dumpregs(ureg);
		if(!user){
			ureg->sp = PTR2UINT(&ureg->sp);
			dumpstackwithureg(ureg);
		}
		if(vno < nelem(excname))
			panic("%s", excname[vno]);
		panic("unknown trap/intr: %d\n", vno);
	}
	splhi();

	/* delaysched set because we held a lock or because our quantum ended */
	if(up && up->delaysched && clockintr){
		if(0)
		if(user && up->ac == nil && up->nqtrap == 0 && up->nqsyscall == 0){
			if(!waserror()){
				up->ac = getac(up, -1);
				poperror();
				runacore();
				return;
			}
		}
		sched();
		splhi();
	}


	if(user){
		if(up && up->procctl || up->nnote)
			notify(ureg);
		kexit(ureg);
	}
}

/*
 * Dump general registers.
 */
void
dumpgpr(Ureg* ureg)
{
	Proc *up = externup();
	if(up != nil)
		print("cpu%d: registers for %s %d\n",
			machp()->machno, up->text, up->pid);
	else
		print("cpu%d: registers for kernel\n", machp()->machno);

	print("ax\t%#16.16llx\n", ureg->ax);
	print("bx\t%#16.16llx\n", ureg->bx);
	print("cx\t%#16.16llx\n", ureg->cx);
	print("dx\t%#16.16llx\n", ureg->dx);
	print("di\t%#16.16llx\n", ureg->di);
	print("si\t%#16.16llx\n", ureg->si);
	print("bp\t%#16.16llx\n", ureg->bp);
	print("r8\t%#16.16llx\n", ureg->r8);
	print("r9\t%#16.16llx\n", ureg->r9);
	print("r10\t%#16.16llx\n", ureg->r10);
	print("r11\t%#16.16llx\n", ureg->r11);
	print("r12\t%#16.16llx\n", ureg->r12);
	print("r13\t%#16.16llx\n", ureg->r13);
	print("r14\t%#16.16llx\n", ureg->r14);
	print("r15\t%#16.16llx\n", ureg->r15);
	print("type\t%#llx\n", ureg->type);
	print("error\t%#llx\n", ureg->error);
	print("pc\t%#llx\n", ureg->ip);
	print("cs\t%#llx\n", ureg->cs);
	print("flags\t%#llx\n", ureg->flags);
	print("sp\t%#llx\n", ureg->sp);
	print("ss\t%#llx\n", ureg->ss);
	print("type\t%#llx\n", ureg->type);
	print("FS\t%#llx\n", rdmsr(FSbase));
	print("GS\t%#llx\n", rdmsr(GSbase));

	print("m\t%#16.16p\nup\t%#16.16p\n", machp(), up);
}

void
dumpregs(Ureg* ureg)
{
die("dumpregs");

	dumpgpr(ureg);

	/*
	 * Processor control registers.
	 * If machine check exception, time stamp counter, page size extensions
	 * or enhanced virtual 8086 mode extensions are supported, there is a
	 * CR4. If there is a CR4 and machine check extensions, read the machine
	 * check address and machine check type registers if RDMSR supported.
	 */
	print("cr0\t%#16.16llx\n", cr0get());
	print("cr2\t%#16.16llx\n", machp()->MMU.cr2);
	print("cr3\t%#16.16llx\n", cr3get());
die("dumpregs");
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
	ureg.ip = getcallerpc();
	ureg.sp = PTR2UINT(&fn);
	fn(&ureg);
}

static void
dumpstackwithureg(Ureg* ureg)
{
	Proc *up = externup();
	uintptr_t l, v, i, estack;
//	extern char etext;
	int x;

	if (0) { //if((s = getconf("*nodumpstack")) != nil && atoi(s) != 0){
		iprint("dumpstack disabled\n");
		return;
	}
	iprint("dumpstack\n");

	x = 0;
	x += iprint("ktrace 9%s %#p %#p\n", strrchr(conffile, '/')+1, ureg->ip, ureg->sp);
	i = 0;
	if(up != nil
//	&& (uintptr)&l >= (uintptr)up->kstack
	&& (uintptr_t)&l <= (uintptr_t)up->kstack+KSTACK)
		estack = (uintptr_t)up->kstack+KSTACK;
	else if((uintptr_t)&l >= machp()->stack && (uintptr_t)&l <= machp()->stack+MACHSTKSZ)
		estack = machp()->stack+MACHSTKSZ;
	else{
		if(up != nil)
			iprint("&up->kstack %#p &l %#p\n", up->kstack, &l);
		else
			iprint("&m %#p &l %#p\n", machp(), &l);
		return;
	}
	x += iprint("estackx %#p\n", estack);

	for(l = (uintptr_t)&l; l < estack; l += sizeof(uintptr_t)){
		v = *(uintptr_t*)l;
		if((KTZERO < v && v < (uintptr_t)&etext)
		|| ((uintptr_t)&l < v && v < estack) || estack-l < 256){
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
debugbpt(Ureg* ureg, void* v)
{
	Proc *up = externup();
	char buf[ERRMAX];

	if(up == 0)
		panic("kernel bpt");
	/* restore pc to instruction that caused the trap */
	ureg->ip--;
	sprint(buf, "sys: breakpoint");
	postnote(up, 1, buf, NDebug);
}

static void
doublefault(Ureg* ureg, void* v)
{
	iprint("cr2 %p\n", (void *)cr2get());
	panic("double fault");
}

static void
unexpected(Ureg* ureg, void* v)
{
	iprint("unexpected trap %llu; ignoring\n", ureg->type);
}

static void
expected(Ureg* ureg, void* v)
{
}

static void
faultamd64(Ureg* ureg, void* v)
{
	Proc *up = externup();
	uint64_t addr;
	int ftype, user, insyscall;
	char buf[ERRMAX];

	addr = machp()->MMU.cr2;
	user = userureg(ureg);
	if(!user && mmukmapsync(addr))
		return;

	/*
	 * There must be a user context.
	 * If not, the usual problem is causing a fault during
	 * initialisation before the system is fully up.
	 */
	if(up == nil){
		panic("fault with up == nil; pc %#llx addr %#llx\n",
			ureg->ip, addr);
	}

	ftype = (ureg->error&2) ? FT_WRITE : (ureg->error&16) ? FT_EXEC : FT_READ;
/*
if (read) hi("read fault\n"); else hi("write fault\n");
hi("addr "); put64(addr); hi("\n");
 */

	insyscall = up->insyscall;
	up->insyscall = 1;
	if (0)hi("call fault\n");

	if(fault(addr, ureg->ip, ftype) < 0){
iprint("could not %s fault %p\n", faulttypes[ftype], addr);
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
		if(!user && (!insyscall || up->nerrlab == 0))
			panic("fault: %#llx\n", addr);
		sprint(buf, "sys: trap: fault %s addr=%#llx",
			faulttypes[ftype], addr);
		postnote(up, 1, buf, NDebug);
		if(insyscall)
			error(buf);
	}
	up->insyscall = insyscall;
}

/*
 *  return the userpc the last exception happened at
 */
uintptr_t
userpc(Ureg* ureg)
{
	Proc *up = externup();
	if(ureg == nil)
		ureg = up->dbgreg;
	return ureg->ip;
}

/* This routine must save the values of registers the user is not permitted
 * to write from devproc and then restore the saved values before returning.
 * TODO: fix this because the segment registers are wrong for 64-bit mode.
 */
void
setregisters(Ureg* ureg, char* pureg, char* uva, int n)
{
	uint64_t cs, flags, ss;

	ss = ureg->ss;
	flags = ureg->flags;
	cs = ureg->cs;
	memmove(pureg, uva, n);
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

uintptr_t
dbgpc(Proc *p)
{
	Ureg *ureg;

	ureg = p->dbgreg;
	if(ureg == 0)
		return 0;

	return ureg->ip;
}
