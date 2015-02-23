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

extern int notify(Ureg*);

static void debugbpt(Ureg*, void*);
static void faultamd64(Ureg*, void*);
static void doublefault(Ureg*, void*);
static void unexpected(Ureg*, void*);
static void expected(Ureg*, void*);
static void dumpstackwithureg(Ureg*);

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

	if(v->mask)
		v->mask(v, 0);

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
		v->mask(v, 1);
	v->f(nil, v->a);
	*ll = v->next;
	ioapicintrdisable(v->vno);
	iunlock(&vctllock);

	free(v);
	return 0;
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
			m = snprint(str, sizeof str, "%11d %11d %20llud %20llud %-*.*s %.*s\n",
				vno, v->irq, t->count, t->cycles, 8, 8, v->type, KNAMELEN, v->name);
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
	trapenable(VectorBPT, debugbpt, 0, "#BP");
	trapenable(VectorPF, faultamd64, 0, "#PF");
	trapenable(Vector2F, doublefault, 0, "#DF");
	intrenable(IdtIPI, expected, 0, BUSUNKNOWN, "#IPI");
	trapenable(Vector15, unexpected, 0, "#15");
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
 *  keep interrupt service times and counts
 */
void
intrtime(int vno)
{
	uint32_t diff, x;		/* should be uint64_t */

	x = perfticks();
	diff = x - m->perf.intrts;
	m->perf.intrts = x;

	m->perf.inintr += diff;
	if(up == nil && m->perf.inidle > diff)
		m->perf.inidle -= diff;

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
		mp = m;
	tos->core = mp->machno;	
	tos->nixtype = mp->nixtype;
	_pmcupdate(m);	
	/*
	 * The process may change its core.
	 * Be sure it has the right cyclefreq.
	 */
	tos->cyclefreq = mp->cyclefreq;
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
		m->cr2 = cr2get();
	trap(ureg);
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
	int clockintr, vno, user;
	char buf[ERRMAX];
	Vctl *ctl, *v;

	vno = ureg->type;

	m->perf.intrts = perfticks();
	user = userureg(ureg);
	if(user && (m->nixtype == NIXTC)){
		up->dbgreg = ureg;
		cycles(&up->kentry);
	}

	clockintr = 0;

	_pmcupdate(m);

	if(ctl = vctl[vno]){
		if(ctl->isintr){
			m->intr++;
			if(vno >= VectorPIC && vno != VectorSYSCALL)
				m->lastintr = ctl->irq;
		}else
			if(up)
				up->nqtrap++;

		if(ctl->isr)
			ctl->isr(vno);
		for(v = ctl; v != nil; v = v->next){
			if(v->f)
				v->f(ureg, v->a);
		}
		if(ctl->eoi)
			ctl->eoi(vno);
		intrtime(vno);
		if(ctl->isintr){
			if(ctl->irq == IrqCLOCK || ctl->irq == IrqTIMER)
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
			m->machno, vno, m->lastintr);
		intrtime(vno);
		if(user)
			kexit(ureg);
		return;
	}
	else{
		if(vno == VectorNMI){
			nmienable();
			if(m->machno != 0){
				iprint("cpu%d: PC %#llux\n",
					m->machno, ureg->ip);
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
	iprint("ureg fs\t%#ux\n", *(unsigned int *)&ureg->ds);
	iprint("type\t%#llux\n", ureg->type);
	iprint("error\t%#llux\n", ureg->error);
	iprint("pc\t%#llux\n", ureg->ip);
	iprint("cs\t%#llux\n", ureg->cs);
	iprint("flags\t%#llux\n", ureg->flags);
	iprint("sp\t%#llux\n", ureg->sp);
	iprint("ss\t%#llux\n", ureg->ss);
	iprint("type\t%#llux\n", ureg->type);
	iprint("FS\t%#llux\n", rdmsr(FSbase));
	iprint("GS\t%#llux\n", rdmsr(GSbase));

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
	iprint("cr2\t%#16.16llux\n", m->cr2);
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
	char *s;
	uintptr_t l, v, i, estack;
//	extern char etext;
	int x;

	if((s = getconf("*nodumpstack")) != nil && atoi(s) != 0){
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
	else if((uintptr_t)&l >= m->stack && (uintptr_t)&l <= m->stack+MACHSTKSZ)
		estack = m->stack+MACHSTKSZ;
	else{
		if(up != nil)
			iprint("&up->kstack %#p &l %#p\n", up->kstack, &l);
		else
			iprint("&m %#p &l %#p\n", m, &l);
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
	panic("double fault");
}

static void
unexpected(Ureg* ureg, void* v)
{
	iprint("unexpected trap %llud; ignoring\n", ureg->type);
}

static void
expected(Ureg* ureg, void* v)
{
}

static void
faultamd64(Ureg* ureg, void* v)
{
	uint64_t addr;
	int read, user, insyscall;
	char buf[ERRMAX];

	addr = m->cr2;
	user = userureg(ureg);
//	if(!user && mmukmapsync(addr))
//		return;

	/*
	 * There must be a user context.
	 * If not, the usual problem is causing a fault during
	 * initialisation before the system is fully up.
	 */
	if(up == nil){
		panic("fault with up == nil; pc %#llux addr %#llux\n",
			ureg->ip, addr);
	}
	read = !(ureg->error & 2);

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
		if(!user && (!insyscall || up->nerrlab == 0))
			panic("fault: %#llux\n", addr);
		sprint(buf, "sys: trap: fault %s addr=%#llux",
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
uintptr_t
userpc(Ureg* ureg)
{
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
	uint16_t ds, es, fs, gs;

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

uintptr_t
dbgpc(Proc *p)
{
	Ureg *ureg;

	ureg = p->dbgreg;
	if(ureg == 0)
		return 0;

	return ureg->ip;
}
