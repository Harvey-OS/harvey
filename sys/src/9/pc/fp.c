/*
 * floating point.  both x87 and sse.
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"

void
fpsavealloc(void)
{
	m->fpsavalign = mallocalign(sizeof(FPssestate), FPalign, 0, 0);
	if (m->fpsavalign == nil)
		panic("cpu%d: can't allocate fpsavalign", m->machno);
}

static char* mathmsg[] =
{
	nil,	/* handled below */
	"denormalized operand",
	"division by zero",
	"numeric overflow",
	"numeric underflow",
	"precision loss",
};

static void
mathstate(ulong *stsp, ulong *pcp, ulong *ctlp)
{
	ulong sts, fpc, ctl;
	FPsave *f = &up->fpsave;

	if (up == nil)
		panic("mathstate: nil up");
	if(fpsave == fpx87save){
		sts = f->status;
		fpc = f->pc;
		ctl = f->control;
	} else {
		sts = f->fsw;
		fpc = f->fpuip;
		ctl = f->fcw;
	}
	if(stsp)
		*stsp = sts;
	if(pcp)
		*pcp = fpc;
	if(ctlp)
		*ctlp = ctl;
}

static void
mathnote(void)
{
	int i;
	ulong status, pc;
	char *msg, note[ERRMAX];

	mathstate(&status, &pc, nil);

	/*
	 * Some attention should probably be paid here to the
	 * exception masks and error summary.
	 */
	msg = "unknown exception";
	for(i = 1; i <= 5; i++){
		if(!((1<<i) & status))
			continue;
		msg = mathmsg[i];
		break;
	}
	if(status & 0x01){
		if(status & 0x40){
			if(status & 0x200)
				msg = "stack overflow";
			else
				msg = "stack underflow";
		}else
			msg = "invalid operation";
	}
	snprint(note, sizeof note, "sys: fp: %s fppc=%#lux status=%#lux",
		msg, pc, status);
	postnote(up, 1, note, NDebug);
}

/*
 * sse fp save and restore buffers have to be 16-byte (FPalign) aligned,
 * so we shuffle the data down as needed or make copies.
 */

void
fpssesave(FPsave *fps)
{
	FPsave *afps;

	fps->magic = 0x1234;
	afps = (FPsave *)ROUND(((uintptr)fps), FPalign);
	fpssesave0(afps);
	if (fps != afps)  /* not aligned? shuffle down from aligned buffer */
		memmove(fps, afps, sizeof(FPssestate));
	if (fps->magic != 0x1234)
		print("fpssesave: magic corrupted\n");
}

void
fpsserestore(FPsave *fps)
{
	FPsave *afps;

	fps->magic = 0x4321;
	afps = (FPsave *)ROUND(((uintptr)fps), FPalign);
	if (fps != afps) {
		afps = m->fpsavalign;
		memmove(afps, fps, sizeof(FPssestate));	/* make aligned copy */
	}
	fpsserestore0(afps);
	if (fps->magic != 0x4321)
		print("fpsserestore: magic corrupted\n");
}

/*
 *  math coprocessor error
 */
static int
matherror(Ureg *ur, void*)
{
	ulong status, pc;

	/*
	 *  a write cycle to port 0xF0 clears the interrupt latch attached
	 *  to the error# line from the 387
	 */
	if(!(m->cpuiddx & Fpuonchip))		/* probably not true any more */
		outb(FPCLRLATCH, 0xFF);

	/*
	 *  save floating point state to check out error
	 */
	fpenv(&up->fpsave);	/* result ignored, but masks fp exceptions */
	fpsave(&up->fpsave);		/* also turns fpu off */
	fpon();
	mathnote();

	if((ur->pc & KSEGM) == KZERO){
		mathstate(&status, &pc, nil);
		panic("fp: status %#lux fppc=%#lux pc=%#lux", status, pc, ur->pc);
	}
	return Intrtrap;
}

/*
 *  math coprocessor emulation fault
 */
static int
mathemu(Ureg *ureg, void*)
{
	ulong status, control;

	if (up == nil)
		panic("mathemu: nil up");
	if(up->fpstate & FPillegal){
		postnote(up, 1, "sys: floating point in note handler", NDebug);
		return Intrtrap;
	}
	switch(up->fpstate){
	case FPinit:
		fpinit();
		up->fpstate = FPactive;
		break;
	case FPinactive:
		/*
		 * Before restoring the state, check for any pending
		 * exceptions, there's no way to restore the state without
		 * generating an unmasked exception.
		 * More attention should probably be paid here to the
		 * exception masks and error summary.
		 */
		mathstate(&status, nil, &control);
		if((status & ~control) & 0x07F){
			mathnote();
			break;
		}
		fprestore(&up->fpsave);
		up->fpstate = FPactive;
		break;
	case FPactive:
		panic("math emu pid %ld %s pc %#lux",
			up->pid, up->text, ureg->pc);
		break;
	}
	return Intrtrap;
}

/*
 *  math coprocessor segment overrun
 */
static int
mathover(Ureg*, void*)
{
	pexit("math overrun", 0);
	return Intrtrap;
}

void
mathinit(void)
{
	trapenable(VectorCERR, matherror, 0, "matherror");
	if(cpuisa386())			/* never true; too old */
		intrenable(IrqIRQ13, matherror, 0, BUSUNKNOWN, "matherror");
	trapenable(VectorCNA, mathemu, 0, "mathemu");
	trapenable(VectorCSO, mathover, 0, "mathover");
}

void
procfpusave(Proc *p)
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
