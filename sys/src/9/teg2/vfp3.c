/*
 * ARM VFPv2 or VFPv3 floating point unit
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "ureg.h"
#include "arm.h"

enum {
	Debug	= 0,
	Pretendnovfp = 0, /* non-0: debug vfp emulation despite having vfp hw */
};

/* subarchitecture code in m->havefp */
enum {
	VFPv2	= 2,
	VFPv3	= 3,
};

enum {
	/* Fpexc bits */
	Fpex =		1u << 31,
	Fpenabled =	1 << 30,
	Fpdex =		1 << 29,	/* defined synch exception */
//	Fp2v =		1 << 28,	/* Fpexinst2 reg is valid */
//	Fpvv =		1 << 27,	/* if Fpdex, vecitr is valid */
//	Fptfv = 	1 << 26,	/* trapped fault is valid */
//	Fpvecitr =	MASK(3) << 8,
	/* FSR bits appear here */
	Fpmbc =		Fpdex,		/* bits exception handler must clear */

	/* Fpscr bits; see u.h for more */
	Stride =	MASK(2) << 20,
	Len =		MASK(3) << 16,
	Dn=		1 << 25,
	Fz=		1 << 24,
	/* trap exception enables (not allowed in vfp3) */
	FPIDNRM =	1 << 15,	/* input denormal */
	Alltraps = FPIDNRM | FPINEX | FPUNFL | FPOVFL | FPZDIV | FPINVAL,
	/* pending exceptions */
	FPAIDNRM =	1 << 7,		/* input denormal */
	Allexc = FPAIDNRM | FPAINEX | FPAUNFL | FPAOVFL | FPAZDIV | FPAINVAL,
	/* condition codes */
	Allcc =		MASK(4) << 28,
};
enum {
	/* CpCPaccess bits */
	Cpaccnosimd =	1u << 31,
	Cpaccd16 =	1 << 30,	/* have only 16 fp regs, not 32 */
};

static void fpsave(FPsave *fps);

static char *
subarch(int impl, uint sa)
{
	static char *armarchs[] = {
		"VFPv1 (unsupported)",
		"VFPv2",
		"VFPv3+ with common VFP subarch v2",
		"VFPv3+ with null subarch",
		"VFPv3+ with common VFP subarch v3",
	};

	if (impl != 'A' || sa >= nelem(armarchs))
		return "GOK";
	else
		return armarchs[sa];
}

static char *
implement(uchar impl)
{
	if (impl == 'A')
		return "arm";
	else
		return "unknown";
}

int
havefp(void)
{
	int gotfp;
	ulong acc, sid;

	if (Pretendnovfp)
		return 0;
	if (m->havefpvalid)
		return m->havefp;

	m->havefp = 0;
	gotfp = 1 << CpFP | 1 << CpDFP;
	cpwrsc(0, CpCONTROL, 0, CpCPaccess, MASK(28));
	acc = cprdsc(0, CpCONTROL, 0, CpCPaccess);
	if ((acc & (MASK(2) << (2*CpFP))) == 0) {
		gotfp &= ~(1 << CpFP);
		print("fpon: no single FP coprocessor\n");
	}
	if ((acc & (MASK(2) << (2*CpDFP))) == 0) {
		gotfp &= ~(1 << CpDFP);
		print("fpon: no double FP coprocessor\n");
	}
	if (!gotfp) {
		print("fpon: no FP coprocessors\n");
		m->havefpvalid = 1;
		return 0;
	}

	m->fpon = 1;			/* lie to avoid panic in fprd */
	sid = fprd(Fpsid);
	m->fpon = 0;
	switch((sid >> 16) & MASK(7)){
	case 0:				/* VFPv1 */
		break;
	case 1:				/* VFPv2 */
		m->havefp = VFPv2;
		m->fpnregs = 16;
		break;
	default:			/* VFPv3 or later */
		m->havefp = VFPv3;
		m->fpnregs = (acc & Cpaccd16) ? 16 : 32;
		break;
	}
	if (m->machno == 0)
		print("fp: %d registers,%s simd\n",
			m->fpnregs, (acc & Cpaccnosimd? " no": ""));
	m->havefpvalid = 1;
	return 1;
}

/*
 * these can be called to turn the fpu on or off for user procs,
 * not just at system start up or shutdown.
 */

void
fpoff(void)
{
	if (m->fpon) {
		fpwr(Fpexc, 0);
		m->fpon = 0;
	}
}

void
fpononly(void)
{
	if (!m->fpon && havefp()) {
		/* enable fp.  must be first operation on the FPUs. */
		fpwr(Fpexc, Fpenabled);
		m->fpon = 1;
	}
}

static void
fpcfg(void)
{
	int impl;
	ulong sid;
	static int printed;

	if (!havefp())
		return;
	/* clear pending exceptions; no traps in vfp3; all v7 ops are scalar */
	m->fpscr = Dn | Fz | FPRNR | (FPINVAL | FPZDIV | FPOVFL) & ~Alltraps;
	fpwr(Fpscr, m->fpscr);
	m->fpconfiged = 1;

	if (printed)
		return;
	sid = fprd(Fpsid);
	impl = sid >> 24;
	print("fp: %s arch %s; rev %ld\n", implement(impl),
		subarch(impl, (sid >> 16) & MASK(7)), sid & MASK(4));
	printed = 1;
}

void
fpinit(void)
{
	if (havefp()) {
		fpononly();
		fpcfg();
	}
}

void
fpon(void)
{
	if (havefp()) {
	 	fpononly();
		if (m->fpconfiged)
			fpwr(Fpscr, (fprd(Fpscr) & Allcc) | m->fpscr);
		else
			fpcfg();	/* 1st time on this fpu; configure it */
	}
}

void
fpclear(void)
{
	if (!havefp())
		return;
	fpon();
	if (!m->fpon)
		return;
	if (0) {
		ulong scr;

		scr = fprd(Fpscr);
		m->fpscr = scr & ~Allexc;
		fpwr(Fpscr, m->fpscr);
	}
	fpwr(Fpexc, fprd(Fpexc) & ~Fpmbc);
}


/*
 * Called when a note is about to be delivered to a
 * user process, usually at the end of a system call.
 * Note handlers are not allowed to use the FPU so
 * the state is marked (after saving if necessary) and
 * checked in the Device Not Available handler.
 */
void
fpunotify(Ureg*)
{
	if(up->fpstate == FPactive){
		fpsave(&up->fpsave);
		up->fpstate = FPinactive;
	}
	up->fpstate |= FPillegal;
}

/*
 * Called from sysnoted() via the machine-dependent
 * noted() routine.
 * Clear the flag set above in fpunotify().
 */
void
fpunoted(void)
{
	up->fpstate &= ~FPillegal;
}

/*
 * Called early in the non-interruptible path of
 * sysrfork() via the machine-dependent syscall() routine.
 * Save the state so that it can be easily copied
 * to the child process later.
 */
void
fpusysrfork(Ureg*)
{
	if(up->fpstate == FPactive){
		fpsave(&up->fpsave);
		up->fpstate = FPinactive;
	}
}

/*
 * Called later in sysrfork() via the machine-dependent
 * sysrforkchild() routine.
 * Copy the parent FPU state to the child.
 */
void
fpusysrforkchild(Proc *p, Ureg *, Proc *up)
{
	/* don't penalize the child, it hasn't done FP in a note handler. */
	p->fpstate = up->fpstate & ~FPillegal;
}

/* should only be called if p->fpstate == FPactive */
static void
fpsave(FPsave *fps)
{
	int n;

	fpon();
	fps->control = fps->status = fprd(Fpscr);
	assert(m->fpnregs);
	for (n = 0; n < m->fpnregs; n++)
		fpsavereg(n, &fps->regs[n].dbl);
	fpoff();
}

static void
fprestore(Proc *p)
{
	int n;

	fpon();
	fpwr(Fpscr, p->fpsave.control);
	m->fpscr = fprd(Fpscr) & ~Allcc;
	assert(m->fpnregs);
	for (n = 0; n < m->fpnregs; n++)
		fprestreg(n, p->fpsave.regs[n].dbl);
}

/*
 * Called from sched() and sleep() via the machine-dependent
 * procsave() routine.
 * About to go in to the scheduler.
 * If the process wasn't using the FPU
 * there's nothing to do.
 */
void
fpuprocsave(Proc *p)
{
	if(p->fpstate == FPactive){
		if(p->state == Moribund)
			fpclear();
		else{
			/*
			 * Fpsave() stores without handling pending
			 * unmasked exeptions. Postnote() can't be called
			 * here as sleep() already has up->rlock, so
			 * the handling of pending exceptions is delayed
			 * until the process runs again and generates an
			 * emulation fault to activate the FPU.
			 */
			fpsave(&p->fpsave);
		}
		p->fpstate = FPinactive;
	}
}

/*
 * The process has been rescheduled and is about to run.
 * Nothing to do here right now. If the process tries to use
 * the FPU again it will cause a Device Not Available
 * exception and the state will then be restored.
 */
void
fpuprocrestore(Proc *)
{
}

/*
 * Disable the FPU.
 * Called from sysexec() via sysprocsetup() to
 * set the FPU for the new process.
 */
void
fpusysprocsetup(Proc *p)
{
	p->fpstate = FPinit;
	fpoff();
}

static void
mathnote(void)
{
	ulong status;
	char *msg, note[ERRMAX];

	status = up->fpsave.status;

	/*
	 * Some attention should probably be paid here to the
	 * exception masks and error summary.
	 */
	if (status & FPAINEX)
		msg = "inexact";
	else if (status & FPAOVFL)
		msg = "overflow";
	else if (status & FPAUNFL)
		msg = "underflow";
	else if (status & FPAZDIV)
		msg = "divide by zero";
	else if (status & FPAINVAL)
		msg = "bad operation";
	else
		msg = "spurious";
	snprint(note, sizeof note, "sys: fp: %s fppc=%#p status=%#lux",
		msg, up->fpsave.pc, status);
	postnote(up, 1, note, NDebug);
}

static void
mathemu(Ureg *)
{
	switch(up->fpstate){
	case FPemu:
		if (!m->fpon && havefp())
			error("sys: illegal instruction: "
				"VFP opcode in emulated mode");
		break;
	case FPinit:
		fpinit();
		up->fpstate = FPactive;
		break;
	case FPinactive:
		/*
		 * Before restoring the state, check for any pending
		 * exceptions.  There's no way to restore the state without
		 * generating an unmasked exception.
		 * More attention should probably be paid here to the
		 * exception masks and error summary.
		 */
		if(up->fpsave.status & (FPAINEX|FPAUNFL|FPAOVFL|FPAZDIV|FPAINVAL)){
			mathnote();
			break;
		}
		fprestore(up);
		up->fpstate = FPactive;
		break;
	case FPactive:
		if (!m->fpon && havefp())
			error("sys: illegal instruction: bad vfp fpu opcode");
		break;
	}
	fpclear();
}

void
fpstuck(uintptr pc)
{
	if (!Debug) {
		USED(pc);
		return;
	}
	if (m->fppc == pc && m->fppid == up->pid) {
		m->fpcnt++;
		if (m->fpcnt > 4)
			panic("fpuemu: cpu%d stuck at pid %ld %s pc %#p "
				"instr %#8.8lux", m->machno, up->pid, up->text,
				pc, *(ulong *)pc);
	} else {
		m->fppid = up->pid;
		m->fppc = pc;
		m->fpcnt = 0;
	}
}

/* will call error() if given a bad pc */
int
getfpinst(uintptr pc, Fpinst *fpin)
{
	Inst inst;

	validaddr(pc, sizeof(Inst), 0);	/* verify legal mapped address */
	validalign(pc, sizeof(Inst));
	fpin->inst = inst = *(Inst *)pc;
	fpin->op = OPCODE(inst);
	fpin->coproc = COPROC(inst);
	fpin->vd = VFPREGD(inst);
	return ISCPOP(fpin->op) && ISFPOP(fpin->coproc);
}

/*
 * only called from trap() to deal with user-mode instruction faults.
 * returns number of consecutive FP instructions seen (emulated or restarted).
 */
int
fpuemu(Ureg* ureg)
{
	int s, nfp;
	Fpinst fpin;

	if(waserror()){
		postnote(up, 1, up->errstr, NDebug);
		return 1;
	}

	if(up == nil)
		panic("fpu trap from kernel mode");
	if(up->fpstate & FPillegal)
		error("sys: floating point in note handler");

	if (!getfpinst(ureg->pc, &fpin)) {
		iprint("fpuemu: invoked on non-coproc instr %#8.8lux at %#p\n",
			fpin.inst, ureg->pc);
		error("sys: fpuemu: invoked on non-coproc instruction");
	}
	if(!condok(ureg->psr, CONDITION(fpin.inst)))
		iprint("fpuemu: instruction condition false\n");

	if(Debug && m->fpon)
		fpstuck(ureg->pc);
	nfp = 0;
	switch (fpin.coproc) {
	case CpDFP:				/* vfp */
	case CpFP:
		if (!m->fpon && havefp()) {
			mathemu(ureg);	/* enable fpu; will retry the instr */
			nfp = 1;		/* saw one fp instr */
			break;
		}
		/* else fall through to emulate */
	case CpOFPA:				/* old arm 7500 fpa */
		s = spllo();
		if(waserror()){
			splx(s);
			nexterror();
		}
		nfp = fpiarm(ureg, &fpin); /* advances pc past emulated inst's */
		if (Debug && nfp > 1)		/* made progress, not stuck? */
			m->fppc = m->fpcnt = 0;
		splx(s);
		poperror();
		break;
	default:
		iprint("fpuemu: invoked on unknown coproc %ld; instr %#8.8lux at %#p\n",
			fpin.coproc, fpin.inst, ureg->pc);
		error("sys: fpuemu: invoked on unknown coproc");
	}

	poperror();
	return nfp;
}
