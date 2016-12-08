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
#include        "encoding.h"

void msg(char *);
// counters. Set by assembly code.
// interrupt enter and exit, systecm call enter and exit.
unsigned long ire, irx, sce, scx;
// Did we start doing an exit for the interrupts?
// ir exit entry :-)
unsigned long irxe;

extern int notify(Ureg*);

//static void debugbpt(Ureg*, void*);
static void faultarch(Ureg*);
//static void doublefault(Ureg*, void*);
//static void unexpected(Ureg*, void*);
//static void expected(Ureg*, void*);
static void dumpstackwithureg(Ureg*);
//extern int bus_irq_setup(Vctl*);

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
	panic("bus_irq_setup");
	//vno = bus_irq_setup(v);
	vno = -1;
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

static const char *const excname[] = {
	"Instruction address misaligned",
	"Instruction access fault",
	"Illegal instruction",
	"Breakpoint",
	"Load address misaligned",
	"Load access fault",
	"Store address misaligned",
	"Store access fault",
	"Environment call from U-mode",
	"Environment call from S-mode",
	"Environment call from H-mode",
	"Environment call from M-mode"
};

static void print_trap_information(const Ureg *ureg)
{
	const char *previous_mode;
	int status = ureg->status;
//	bool mprv = !!(ureg->status & MSTATUS_MPRV);

	/* Leave some space around the trap message */
	print("\n");

	if (ureg->cause < nelem(excname))
		print("Exception:          %s\n",
				excname[ureg->cause]);
	else
		print("Trap:               Unknown cause %p\n",
				(void *)ureg->cause);

	previous_mode = status & 0x100 ? "Supervisor" : "User";
	print("Previous mode:      %s\n", previous_mode);
	print("Bad instruction pc: %p\n", (void *)ureg->epc);
	print("Bad address:        %p\n", (void *)ureg->badaddr);
	print("Stored ip:          %p\n", (void*) ureg->ip);
	print("Stored sp:          %p\n", (void*) ureg->sp);
}

void trap_handler(Ureg *ureg) {
	switch(ureg->cause) {
		case CAUSE_MISALIGNED_FETCH:
			print_trap_information(ureg);
			panic("misaligned fetch, firmware is supposed to do this");
			break;
		case CAUSE_ILLEGAL_INSTRUCTION:
			print_trap_information(ureg);
			panic("illegal instruction, going to die");
			break;
		case CAUSE_BREAKPOINT:
			print_trap_information(ureg);
			panic("can't handle breakpoints yet\n");
			break;
		case CAUSE_FAULT_FETCH:
		case CAUSE_FAULT_LOAD:
		case CAUSE_FAULT_STORE:
			print_trap_information(ureg);
			faultarch(ureg);
	hexdump((void *)ureg->ip, 16);
	hexdump((void *)ureg->epc, 16);
	hexdump((void *)ureg->badaddr, 16);
			break;
		case CAUSE_USER_ECALL:
		case CAUSE_HYPERVISOR_ECALL:
		case CAUSE_MACHINE_ECALL:
			print_trap_information(ureg);
			panic("Can't do ecalls here");
			break;
		case CAUSE_MISALIGNED_LOAD:
			print("hgroup 2\n");
			print_trap_information(ureg);
			panic("misaligned LOAD, we don't do these");
			break;
		case CAUSE_MISALIGNED_STORE:
			print_trap_information(ureg);
			panic("misaligned STORE, we don't do these");
			break;
		default:
			print_trap_information(ureg);
			panic("WTF\n");
			break;
	}
}

int
intrdisable(void* vector)
{
	Vctl *v, *x, **ll;
	//extern int ioapicintrdisable(int);

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
	panic("ioapicintrdisable");
	//ioapicintrdisable(v->vno);
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
	v->Vkey.tbdf = -1;
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
	panic("nmienable");
}

void
trapinit(void)
{
	// basically done in firmware. 
	addarchfile("irqalloc", 0444, irqallocread, nil);
}

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
	if (0) print("USTKTOP %p sizeof(Tos) %d tos %p\n", (void *)USTKTOP, sizeof(Tos), tos);
	cycles(&t);
	if (1) {
		if (0) print("tos is %p, &tos->kcycles is %p, up is %p\n", tos, &tos->kcycles, up);
		tos->kcycles += t - up->kentry;
		tos->pcycles = up->pcycles;
		tos->pid = up->pid;
	
		if (up->ac != nil)
			mp = up->ac;
		else
			mp = machp();
		if (0) print("kexit: mp is %p\n", mp);
		tos->core = mp->machno;
		if (0) print("kexit: mp is %p\n", mp);
		tos->nixtype = mp->NIX.nixtype;
		if (0) print("kexit: mp is %p\n", mp);
		//_pmcupdate(m);
		/*
	 	* The process may change its core.
	 	* Be sure it has the right cyclefreq.
	 	*/
		tos->cyclefreq = mp->cyclefreq;
		if (0) print("kexit: mp is %p\n", mp);
	}
	if (0) print("kexit: done\n");
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

enum traps {
	InstructionAlignment = 0,
	InstructionAccessFault,
	IllegalInstruction,
	Breakpoint,
	Trap4Reserved,
	LoadAccessFault,
	AMOAddressMisaligned,
	Store_AMOAccessFault,
	EnvironmentCall,
	LastTrap = EnvironmentCall,
	InterruptMask = 0x8000000000000000ULL
};

enum interrupts {
	UserSoftware,
	SupervisorSoftware,
	Interrupt2Reserved,
	Interrupt3eserved,
	UserTimer,	
	SupervisorTimer,
	LastInterrupt = SupervisorTimer
};

void
_trap(Ureg *ureg)
{
	if (0) msg("+trap\n");
	if (0) print("_trap\n");
	/*
	 * If it's a real trap in this core, then we want to
	 * use the hardware cr2 register.
	 * We cannot do this in trap() because application cores
	 * would update m->cr2 with their cr2 values upon page faults,
	 * and then call trap().
	 * If we do this in trap(), we would overwrite that with our own cr2.
	 */
	switch(ureg->cause){
	case CAUSE_FAULT_FETCH:
		ureg->ftype = FT_EXEC;
		machp()->MMU.badaddr = ureg->badaddr;
		break;
	case CAUSE_FAULT_LOAD:
		ureg->ftype = FT_READ;
		machp()->MMU.badaddr = ureg->badaddr;
		break;
	case CAUSE_FAULT_STORE:
		ureg->ftype = FT_WRITE;
		machp()->MMU.badaddr = ureg->badaddr;
		break;
	}
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
trap(Ureg *ureg)
{
	int clockintr, vno, user, interrupt;
	// cache the previous vno to see what might be causing
	// trouble
	print("T");
	vno = ureg->cause & ~InterruptMask;
	interrupt = ureg->cause & InterruptMask;
	Mach *m =machp();
	//if (sce > scx) iprint("====================");
	lastvno = vno;
	if (m < (Mach *)(1ULL<<63))
		die("bogus mach");
	Proc *up = externup();
	char buf[ERRMAX];
	Vctl *ctl, *v;

	machp()->perf.intrts = perfticks();
	user = userureg(ureg);
	if(user && (machp()->NIX.nixtype == NIXTC)){
		print("call cycles\n");
		up->dbgreg = ureg;
		cycles(&up->kentry);
		print("done\n");
	}

	clockintr = interrupt && vno == SupervisorTimer;
	print("clockintr %d\n", clockintr);

	if (clockintr) panic("clockintr finallyi set");
	//_pmcupdate(machp());

	if (!interrupt){
		print("trap_handler\n");
		trap_handler(ureg);
	} else {
panic("real interrupt!");

	if(ctl = vctl[vno]){
		panic("we don't know what to do with this interrupt yet");
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
	} else if(vno < nelem(excname) && user){
print("OOR\n");
		spllo();
		snprint(buf, sizeof buf, "sys: trap: %s", excname[vno]);
		postnote(up, 1, buf, NDebug);
	} else if ((interrupt && vno > LastInterrupt) || (vno > LastTrap)) {
print("UNK\n");
		/*
		 * An unknown interrupt.
		 */

		iprint("cpu%d: spurious interrupt %d, last %d\n",
			machp()->machno, vno, machp()->lastintr);
		intrtime(vno);
		if(user)
			kexit(ureg);
		return;
	} else {
print("wut\n");
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
	}
	splhi();

	/* delaysched set because we held a lock or because our quantum ended */
	if(up && up->delaysched && clockintr){
#if 0
		if(0)
		if(user && up->ac == nil && up->nqtrap == 0 && up->nqsyscall == 0){
			if(!waserror()){
				up->ac = getac(up, -1);
				poperror();
				runacore();
				return;
			}
		}
#endif
		sched();
		splhi();
	}
print("DUN\n");

	if(user){
		if(up && up->procctl || up->nnote)
			notify(ureg);
		kexit(ureg);
	}
print("ALL DONE TRAP\n");
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

	print("ip %#llx\n", ureg->ip);
	print("sp %#llx\n", ureg->sp);
	print("gp %#llx\n", ureg->gp);
	print("tp %#llx\n", ureg->tp);
	print("t0 %#llx\n", ureg->t0);
	print("t1 %#llx\n", ureg->t1);
	print("t2 %#llx\n", ureg->t2);
	print("s0 %#llx\n", ureg->s0);
	print("s1 %#llx\n", ureg->s1);
	print("a0 %#llx\n", ureg->a0);
	print("a1 %#llx\n", ureg->a1);
	print("a2 %#llx\n", ureg->a2);
	print("a3 %#llx\n", ureg->a3);
	print("a4 %#llx\n", ureg->a4);
	print("a5 %#llx\n", ureg->a5);
	print("a6 %#llx\n", ureg->a6);
	print("a7 %#llx\n", ureg->a7);
	print("s2 %#llx\n", ureg->s2);
	print("s3 %#llx\n", ureg->s3);
	print("s4 %#llx\n", ureg->s4);
	print("s5 %#llx\n", ureg->s5);
	print("s6 %#llx\n", ureg->s6);
	print("s7 %#llx\n", ureg->s7);
	print("s8 %#llx\n", ureg->s8);
	print("s9 %#llx\n", ureg->s9);
	print("s10 %#llx\n", ureg->s10);
	print("s11 %#llx\n", ureg->s11);
	print("t3 %#llx\n", ureg->t3);
	print("t4 %#llx\n", ureg->t4);
	print("t5 %#llx\n", ureg->t5);
	print("t6 %#llx\n", ureg->t6);
	print("status %#llx\n", ureg->status);
	print("epc %#llx\n", ureg->epc);
	print("badaddr %#llx\n", ureg->badaddr);
	print("cause %#llx\n", ureg->cause);
	print("insnn %#llx\n", ureg->insnn);
	print("bp %#llx\n", ureg->bp);
	print("ftype %#llx\n", ureg->ftype);

	print("m\t%#16.16p\nup\t%#16.16p\n", machp(), up);
}

void
dumpregs(Ureg* ureg)
{
panic("dumpregs");

	dumpgpr(ureg);
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
	//x += iprint("ktrace 9%s %#p %#p\n", strrchr(conffile, '/')+1, ureg->ip, ureg->sp);
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

#if 0
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
	iprint("badaddr %p\n", read_csr(sbadaddr));
	panic("double fault");
}

static void
unexpected(Ureg *ureg, void* v)
{
	iprint("unexpected trap %llu; ignoring\n", ureg->cause);
}

static void
expected(Ureg* ureg, void* v)
{
}
#endif

/*static*/
void
faultarch(Ureg* ureg)
{
	Proc *up = externup();
	uint64_t addr;
	int ftype = ureg->ftype, user, insyscall;
	char buf[ERRMAX];

	addr = ureg->badaddr;
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

	insyscall = up->insyscall;
	up->insyscall = 1;
	msg("call fault\n");

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
#if 0
	uint64_t cs, flags, ss;

	ss = ureg->ss;
	flags = ureg->flags;
	cs = ureg->cs;
	memmove(pureg, uva, n);
	ureg->cs = cs;
	ureg->flags = (ureg->flags & 0x00ff) | (flags & 0xff00);
	ureg->ss = ss;
#endif
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
