/*
 * SIMD Floating Point.
 * Assembler support to get at the individual instructions
 * is in l64fpu.s.
 * There are opportunities to be lazier about saving and
 * restoring the state and allocating the storage needed.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "amd64.h"
#include "ureg.h"

enum {						/* FCW, FSW and MXCSR */
	I		= 0x0001,		/* Invalid-Operation */
	D		= 0x0002,		/* Denormalized-Operand */
	Z		= 0x0004,		/* Zero-Divide */
	O		= 0x0008,		/* Overflow */
	U		= 0x0010,		/* Underflow */
	P		= 0x0020,		/* Precision */
};

enum {						/* FCW */
	PCs		= 0x0000,		/* Precision Control -Single */
	PCd		= 0x0200,		/* -Double */
	PCde		= 0x0300,		/* -Double Extended */
	RCn		= 0x0000,		/* Rounding Control -Nearest */
	RCd		= 0x0400,		/* -Down */
	RCu		= 0x0800,		/* -Up */
	RCz		= 0x0C00,		/* -Toward Zero */
};

enum {						/* FSW */
	Sff		= 0x0040,		/* Stack Fault Flag */
	Es		= 0x0080,		/* Error Summary Status */
	C0		= 0x0100,		/* ZF - Condition Code Bits */
	C1		= 0x0200,		/* O/U# */
	C2		= 0x0400,		/* PF */
	C3		= 0x4000,		/* ZF */
	B		= 0x8000,		/* Busy */
};

enum {						/* MXCSR */
	Daz		= 0x0040,		/* Denormals are Zeros */
	Im		= 0x0080,		/* I Mask */
	Dm		= 0x0100,		/* D Mask */
	Zm		= 0x0200,		/* Z Mask */
	Om		= 0x0400,		/* O Mask */
	Um		= 0x0800,		/* U Mask */
	Pm		= 0x1000,		/* P Mask */
	Rn		= 0x0000,		/* Round to Nearest */
	Rd		= 0x2000,		/* Round Down */
	Ru		= 0x4000,		/* Round Up */
	Rz		= 0x6000,		/* Round toward Zero */
	Fz		= 0x8000,		/* Flush to Zero for Um */
};

enum {						/* PFPU.state */
	Init		= 0,			/* The FPU has not been used */
	Busy		= 1,			/* The FPU is being used */
	Idle		= 2,			/* The FPU has been used */

	Hold		= 1<<2,			/* Handling an FPU note */
};

extern void _clts(void);
extern void _fldcw(u16int);
extern void _fnclex(void);
extern void _fninit(void);
extern void _fxrstor(Fxsave*);
extern void _fxsave(Fxsave*);
extern void _fwait(void);
extern void _ldmxcsr(u32int);
extern void _stts(void);

static Fxsave *
fpalign(void *buf)
{
	return (Fxsave *)(((uintptr)(buf) + FPalign - 1) &
		~((uintptr)FPalign - 1));
}

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
	if(offset >= sizeof(Fxsave))
		return 0;
	if((p = proc->fpusave) == nil)
		return 0;
	if(offset+n > sizeof(Fxsave))
		n = sizeof(Fxsave) - offset;
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
	_fxsave(p->fpusave);	/* relatively costly; dumps much state */
	m->fpsaves++;
	_stts();
	p->fpustate = Idle;
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
	child->fpusave = fpalign(up->fxsave);
	if(child->fpustate != Init)
		memmove(child->fpusave, parent->fpusave, sizeof(Fxsave));
}

static void
initfpustate(Proc *p)
{
	Mpl pl;

	pl = splhi();  /* don't let an interrupt set Ts until after _fnclex */
	_clts();
	/*
	 * _fnclex got #NM in troff or hangs about every 2 weeks only on cpu.
	 * Xeon E3 bug?  use _fninit+(load state) instead of _fnclex?
	 */
	_fnclex();		/* will fault if Ts (or Em) set */
	_fwait();		/* ensure completion before _stts */
	_stts();
	p->fpustate = Init;
	splx(pl);
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
	Fxsave *fpusave;
	char *m, n[ERRMAX];

	/*
	 * The Sff bit is sticky, meaning it should be explicitly
	 * cleared or there's no way to tell if the exception was an
	 * invalid operation or a stack fault.
	 */
	fpusave = up->fpusave;
	fsw = (fpusave->fsw & ~fpusave->fcw) & (Sff|P|U|O|Z|D|I);
	if(fsw & I)
		if(fsw & Sff)
			if(fsw & C1)
				m = "Stack Overflow";
			else
				m = "Stack Underflow";
		else
			m = "Invalid Operation";
	else if(fsw & D)
		m = "Denormal Operand";
	else if(fsw & Z)
		m = "Divide-By-Zero";
	else if(fsw & O)
		m = "Numeric Overflow";
	else if(fsw & U)
		m = "Numeric Underflow";
	else if(fsw & P)
		m = "Precision";
	else
		m =  "Unknown";

	snprint(n, sizeof(n), "sys: fp: %s Exception ipo=%#llux fsw=%#ux",
		m, fpusave->rip, fsw);
	postnote(up, 1, n, NDebug);
}

static void
fpuxf(Ureg* ureg, void*)
{
	u32int mxcsr;
	char *m, n[ERRMAX];

	/*
	 * #XF - SIMD Floating Point Exception (Vector 18).
	 */

	/*
	 * Save FPU state to check out the error.
	 */
	fpuidle(up);

	if(ureg->ip & KZERO)
		panic("#XF: kernel ip=%#p", ureg->ip);

	/*
	 * Notify the user process.
	 * The path here is similar to the x87 path described
	 * in fpupostnote above but without the fpupostnote()
	 * call.
	 */
	mxcsr = ((Fxsave *)up->fpusave)->mxcsr;
	if((mxcsr & (Im|I)) == I)
		m = "Invalid Operation";
	else if((mxcsr & (Dm|D)) == D)
		m = "Denormal Operand";
	else if((mxcsr & (Zm|Z)) == Z)
		m = "Divide-By-Zero";
	else if((mxcsr & (Om|O)) == O)
		m = "Numeric Overflow";
	else if((mxcsr & (Um|U)) == U)
		m = "Numeric Underflow";
	else if((mxcsr & (Pm|P)) == P)
		m = "Precision";
	else
		m =  "Unknown";

	snprint(n, sizeof(n), "sys: fp: %s Exception mxcsr=%#ux", m, mxcsr);
	postnote(up, 1, n, NDebug);
}

static void
fpumf(Ureg* ureg, void*)
{
	/*
	 * #MF - x87 Floating Point Exception Pending (Vector 16).
	 */

	/*
	 * Save FPU state to check out the error.
	 */
	fpuidle(up);

	if(ureg->ip & KZERO)
		panic("#MF: kernel ip=%#p rip=%#p",
			ureg->ip, ((Fxsave *)up->fpusave)->rip);

	/*
	 * Notify the user process.
	 * The path here is
	 *	call trap->fpumf->fpupostnote->postnote
	 *	return ->fpupostnote->fpumf->trap
	 *	call notify->fpunotify
	 *	return ->notify
	 * then either
	 *	call pexit
	 * or
	 *	return ->trap
	 *	return ->user note handler
	 */
	fpupostnote();
}

static void
fpunm(Ureg* ureg, void*)
{
	Fxsave *fpusave;

	/*
	 * #NM - Device Not Available (Vector 7).
	 */
	if(up == nil)
		panic("#NM: fpu in kernel: ip %#p", ureg->ip);

	/*
	 * Someone tried to use the FPU in a note handler.
	 * That's a no-no.
	 */
	if(up->fpustate & Hold){
		postnote(up, 1, "sys: floating point in note handler", NDebug);
		return;
	}

	/*
	 * was getting here from _fnclex from initfpustate with fpustate == 2
	 * (Idle) so presumably from fpuprocsave, only on cpu, and only after
	 * about two weeks up.  Xeon E3 bug?  NB: up->fpustate is logical
	 * state, not physical m->fcw.
	 */
	if(ureg->ip & KZERO) {
		/* we hope it's spurious */
		iprint("#NM: kernel ip %#p proc %d %s up->fpustate %d cr0 %#llux\n",
			ureg->ip, up->pid, up->text, up->fpustate,
			(uvlong)cr0get());
		return;
	}

	switch(up->fpustate){
	case Busy:
	default:
		panic("#NM: bad fpu state %d ip %#p", up->fpustate, ureg->ip);
		break;
	case Init:
		/*
		 * A process tries to use the FPU for the
		 * first time and generates a 'device not available'
		 * exception.
		 * Turn the FPU on and initialise it for use.
		 * Set the precision and mask the exceptions
		 * we don't care about from the generic Mach value.
		 */
		_clts();
		_fninit();
		_fwait();
		_fldcw(m->fcw);			/* x87 control */
		_ldmxcsr(m->mxcsr);		/* sse control */
		up->fpusave = fpalign(up->fxsave);
		up->fpustate = Busy;
		break;
	case Idle:
		/*
		 * Before restoring the state, check for any pending
		 * exceptions, there's no way to restore the state without
		 * generating an unmasked exception.
		 */
		fpusave = up->fpusave;
		if((fpusave->fsw & ~fpusave->fcw) & (Sff|P|U|O|Z|D|I)){
			fpupostnote();
			break;
		}

		/*
		 * Sff is sticky.
		 */
		fpusave->fcw &= ~Sff;
		_clts();
		_fxrstor(fpusave);  /* relatively costly; inhales much state */
		m->fprestores++;
		up->fpustate = Busy;
		break;
	}
}

void
fpuinit(void)
{
	u64int r;
	Fxsave *fxsave;
	uchar buf[sizeof(Fxsave)+FPalign-1];

	/*
	 * It's assumed there is an integrated FPU, so Em is cleared;
	 */
	r = cr0get() & ~(Ts|Em);
	cr0put(r | Ne | Mp);

	cr4put(cr4get() | Osxmmexcpt | Osfxsr);

	_fninit();
	fxsave = fpalign(buf);
	memset(fxsave, 0, sizeof(Fxsave));
	_fxsave(fxsave);
	m->fpsaves = m->fprestores = 0;
	m->fcw = RCn|PCd|P|U|D;			/* x87: signal I|Z|O */
	if(fxsave->mxcsrmask == 0)
		m->mxcsrmask = MASK(16) & ~FPDAZ;
	else
		m->mxcsrmask = fxsave->mxcsrmask;
	m->mxcsr = (Rn|Pm|Um|Dm) & m->mxcsrmask; /* sse: signal I|Z|O */
	_stts();

	/*
	 * Set up the exception handlers.
	 */
	if(m->machno == 0) {
		trapenable(IdtNM, fpunm, 0, "#NM");
		trapenable(IdtMF, fpumf, 0, "#MF");
		trapenable(IdtXF, fpuxf, 0, "#XF");
	}
}

void
fpsts2ureg(Ureg *)
{
}
