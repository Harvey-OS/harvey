/*
 * RV32FD and RV64FD Floating Point.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "riscv.h"
#include "ureg.h"

enum {						/* FCSR */
	I		= FPAINEX,
	D		= FPAINVAL,		/* Denormalized-Operand */
	Z		= FPAZDIV,
	O		= FPAOVFL,		/* Overflow */
	U		= FPAUNFL,		/* Underflow */
};

enum {						/* PFPU.state */
	Init		= 0,			/* The FPU has not been used */
	Busy		= 1,			/* The FPU is being used */
	Idle		= 2,			/* The FPU has been used */

	Hold		= 1<<2,			/* Handling an FPU note */
};

static char *fpstnames[] = {
	"Init", "Busy", "Idle",
};

int
fpudevprocio(Proc* proc, void* a, long n, uintptr offset, int write)
{
	uchar *p;

	/*
	 * Called from procdevtab.read and procdevtab.write
	 * allow user process access to the FPU registers.
	 * This is the only FPU routine which is called directly
	 * from the port code; it would be nice to have dynamic
	 * creation of entries in the device file trees...
	 */
	if(offset >= sizeof(proc->fpusave))
		return 0;
	if((p = (uchar *)proc->fpusave) == nil)
		return 0;
	if(offset+n > sizeof(proc->fpusave))
		n = sizeof(proc->fpusave) - offset;
	switch(write){
	default:
		memmove(p+offset, a, n);
		break;
	case 0:
		memmove(a, p+offset, n);
		break;
	}

	return n;
}

static void
fpuidle(Proc *p)
{
	/* user may have changed fcsr since we last restored it */
	if (p->fpustate != Init) {
		fpon();
		p->fcsr = getfcsr();
	}
	if (p->fpustate == Idle)
		return;
	fpusave((uchar *)p->fpusave);	/* and disables fpu */
	m->turnedfpoff = 1;	/* propagate Fsst change back to user mode */
	p->fpustate = Idle;
	m->fpsaves++;
}

void
fpunotify(Ureg*)
{
	/*
	 * Called when a note is about to be delivered to a
	 * user process, usually at the end of a system call.
	 * Note handlers are not allowed to use the FPU so
	 * the state is marked (after saving if necessary) and
	 * checked in the Device Not Available handler.
	 */
	if(up->fpustate == Busy)
		fpuidle(up);
	up->fpustate |= Hold;
}

void
fpunoted(void)
{
	/*
	 * Called from sysnoted() via the machine-dependent
	 * noted() routine.
	 * Clear the flag set above in fpunotify().
	 */
	up->fpustate &= ~Hold;
}

void
fpusysrfork(Ureg*)
{
	/*
	 * Called early in the non-interruptible path of
	 * sysrfork() via the machine-dependent syscall() routine.
	 * Save the state so that it can be easily copied
	 * to the child process later.
	 */
	if(up->fpustate == Busy)
		fpuidle(up);
}

void
fpusysrforkchild(Proc* child, Proc* parent)
{
	/*
	 * Called later in sysrfork() via the machine-dependent
	 * sysrforkchild() routine.
	 * Copy the parent FPU state to the child.
	 */
	child->fpustate = parent->fpustate;
	if(child->fpustate != Init) {
		memmove(child->fpusave, parent->fpusave, sizeof(child->fpusave));
		child->fcsr = parent->fcsr;
	}
}

static void
initfpustate(Proc *p)
{
	_fpuinit();	/* inits last few regs, also leaves fpu Initial */
	fpusave((uchar *)p->fpusave);	/* and disables fpu */
	p->fpustate = Init;
	m->turnedfpoff = 1;	/* propagate Fsst change back to user mode */
}

void
fpuprocsave(Proc* p)
{
	/*
	 * Called from sched() and sleep() via the machine-dependent
	 * procsave() routine.
	 * About to go in to the scheduler.
	 * If the process wasn't using the FPU
	 * there's nothing to do.
	 */
	if(p->fpustate != Busy)
		return;

	/*
	 * The process is dead so clear and disable the FPU
	 * and set the state for whoever gets this proc struct
	 * next.
	 */
	if(p->state == Moribund){
		initfpustate(p);
		return;
	}

	/*
	 * Save the FPU state without handling pending
	 * unmasked exceptions and disable. Postnote() can't
	 * be called here as sleep() already has up->rlock,
	 * so the handling of pending exceptions is delayed
	 * until the process runs again and generates a
	 * Device Not Available exception fault to activate
	 * the FPU.
	 */
	fpuidle(p);
}

void
fpuprocrestore(Proc* p)
{
	/*
	 * The process has been rescheduled and is about to run.
	 * Nothing to do here right now. If the process tries to use
	 * the FPU again it will cause a Device Not Available
	 * exception and the state will then be restored.
	 */
	USED(p);
}

void
fpusysprocsetup(Proc* p)
{
	/*
	 * Disable the FPU.
	 * Called from sysexec() via sysprocsetup() to
	 * set the FPU for the new process.
	 */
	if(p->fpustate != Init)
		initfpustate(p);
}

static void
fpupostnote(void)
{
	ushort fsw;
	char *m, n[ERRMAX];

	fsw = up->fcsr & (U|O|Z|D|I);
	if(fsw & I)
		m = "Invalid Operation";
	else if(fsw & D)
		m = "Denormal Operand";
	else if(fsw & Z)
		m = "Divide-By-Zero";
	else if(fsw & O)
		m = "Numeric Overflow";
	else if(fsw & U)
		m = "Numeric Underflow";
	else
		m =  "Unknown";

	snprint(n, sizeof(n), "sys: fp: %s Exception fsw=%#ux", m, fsw);
	postnote(up, 1, n, NDebug);
}

/*
 * called on an illegal FP instruction, usually because the FPU is disabled.
 * if so, enable FPU, load saved state if needed, and return to retry the
 * user-mode instruction.
 */
void
fptrap(Ureg* ureg, void*)
{
	int rnd, fpison;
	Mreg fpsts;

	if(up == nil)
		panic("fptrap: fpu in kernel: ip %#p", ureg->pc);

	/*
	 * Someone tried to use the FPU in a note handler.
	 * That's a no-no.
	 */
	if(up->fpustate & Hold){
		postnote(up, 1, "sys: floating point in note handler", NDebug);
		return;
	}

	/*
	 * NB: up->fpustate is logical state, not physical CSR(FCSR).
	 */
	if(ureg->pc & KZERO) {
		/* we hope it's spurious */
		iprint("fptrap: kernel ip %#p proc %d %s up->fpustate %d\n",
			ureg->pc, up->pid, up->text, up->fpustate);
		return;
	}

	fpsts = ureg->status & Fsst;
	fpison = fpsts != (Off<<Fsshft);

	/*
	 * before we assume that the trap is due to the fpu being off, check
	 * for any other possible fp trap causes.
	 */
	if (fpison)
		up->fcsr = getfcsr();
	rnd = (up->fcsr & FPRMASK) >> 5;
	if (rnd >= 5) {
		iprint("fptrap: clearing bad fpu rounding mode %d at pc %#p in %s\n",
			rnd, ureg->pc, up->text);
		up->fcsr &= ~FPRMASK;
		up->fcsr |= FPRNR;
		if (fpison)
			setfcsr(up->fcsr);
		return;
	}

	if (fpison) {
		iprint("trap: ill fp inst %#lux with enabled fp status %lld\n",
			(ulong)ureg->tval, fpsts);
		postnote(up, 1, "sys: illegal instruction", NDebug);
		return;
	}

	/* we now know that FPU is off. */
	switch(up->fpustate){
	case Busy:
	default:
		panic("fptrap: bad fpu state %s at pc %#p",
			fpstnames[up->fpustate & ~Hold], ureg->pc);
	case Init:
		/*
		 * A process tries to use the FPU for the
		 * first time and generates a 'device not available'
		 * exception.
		 * Turn the FPU on and initialise it for use.
		 * Set the precision and mask the exceptions.
		 */
		memset(up->fpusave, 0, sizeof up->fpusave);
		fpurestore((uchar *)up->fpusave);
		_fpuinit();	/* inits last few regs, leaves fpu Initial */
		up->fcsr = getfcsr();
		up->fpustate = Busy;
		break;
	case Idle:
		/*
		 * Note that risc-v doesn't trap on fp exceptions, so skip this.
		 * Programs that care must check explicitly.
		 *
		 * Before restoring the state, check for any pending exceptions,
		 * in case there's no way to restore the state without
		 * generating an unmasked exception.
		 */
		if(0 && up->fcsr & (U|O|Z|D|I)){
			fpupostnote();
			break;
		}

		fpurestore((uchar *)up->fpusave);
		setfcsr(up->fcsr);
		m->fprestores++;
		up->fpustate = Busy;
		break;
	}
	/* propagate Fsst changes back to user mode */
	ureg->status = (ureg->status & ~Fsst) | (getsts() & Fsst);
}

/*
 * if fp is disabled (in Off state), update ureg->status to match,
 * keeping the fpu off after the transition to user mode.   on the
 * next FP use, we'll trap to fptrap and reload and enable it.
 * this makes lazy FP state restoral work, and restoring should only be
 * necessary after switching processes.
 */
void
fpsts2ureg(Ureg *ureg)
{
	if (m->turnedfpoff) {
		/* make sure it's still off */
		if ((getsts() & Fsst) == (Off<<Fsshft))
			ureg->status = (ureg->status & ~Fsst) | (Off<<Fsshft);
		m->turnedfpoff = 0;
	}
}

void
fpuinit(void)
{
	_fpuinit();		/* inits last few regs, leaves fpu Initial */
	fpoff();
	m->fpsaves = 0;
	m->fprestores = 0;
}
