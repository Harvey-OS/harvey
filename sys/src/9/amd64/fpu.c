/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * SIMD Floating Point.
 * Assembler support to get at the individual instructions is in l64fpu.s.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "amd64.h"
#include "ureg.h"

enum {						/* FCW, FSW and MXCSR */
	I		= 0x00000001,		/* Invalid-Operation */
	D		= 0x00000002,		/* Denormalized-Operand */
	Z		= 0x00000004,		/* Zero-Divide */
	O		= 0x00000008,		/* Overflow */
	U		= 0x00000010,		/* Underflow */
	P		= 0x00000020,		/* Precision */
};

enum {						/* FCW */
	PCs		= 0x00000000,		/* Precision Control -Single */
	PCd		= 0x00000200,		/* -Double */
	PCde		= 0x00000300,		/* -Double Extended */
	RCn		= 0x00000000,		/* Rounding Control -Nearest */
	RCd		= 0x00000400,		/* -Down */
	RCu		= 0x00000800,		/* -Up */
	RCz		= 0x00000C00,		/* -Toward Zero */
};

enum {						/* FSW */
	Sff		= 0x00000040,		/* Stack Fault Flag */
	Es		= 0x00000080,		/* Error Summary Status */
	C0		= 0x00000100,		/* ZF - Condition Code Bits */
	C1		= 0x00000200,		/* O/U# */
	C2		= 0x00000400,		/* PF */
	C3		= 0x00004000,		/* ZF */
	B		= 0x00008000,		/* Busy */
};

enum {						/* MXCSR */
	Daz		= 0x00000040,		/* Denormals are Zeros */
	Im		= 0x00000080,		/* I Mask */
	Dm		= 0x00000100,		/* D Mask */
	Zm		= 0x00000200,		/* Z Mask */
	Om		= 0x00000400,		/* O Mask */
	Um		= 0x00000800,		/* U Mask */
	Pm		= 0x00001000,		/* P Mask */
	Rn		= 0x00000000,		/* Round to Nearest */
	Rd		= 0x00002000,		/* Round Down */
	Ru		= 0x00004000,		/* Round Up */
	Rz		= 0x00006000,		/* Round toward Zero */
	Fz		= 0x00008000,		/* Flush to Zero for Um */
};

static Fxsave defaultFxsave;

extern void _fxrstor(Fxsave*);
extern void _fxsave(Fxsave*);


static void
fpusave(Fxsave *fxsave)
{
	_fxsave(fxsave);
}

static void
fpurestore(Fxsave *fxsave)
{
	_fxrstor(fxsave);
}

int
fpudevprocio(Proc* proc, void* a, int32_t n, uintptr_t offset, int write)
{
	/*
	 * Called from procdevtab.read and procdevtab.write
	 * allow user process access to the FPU registers.
	 * This is the only FPU routine which is called directly
	 * from the port code; it would be nice to have dynamic
	 * creation of entries in the device file trees...
	 */
	if (offset >= sizeof(Fxsave))
		return 0;

	uint8_t *p = (uint8_t*)&proc->FPU.fxsave;

	switch (write) {
	default:
		if(offset+n > sizeof(Fxsave))
			n = sizeof(Fxsave) - offset;
		memmove(p+offset, a, n);
		break;
	case 0:
		if(offset+n > sizeof(Fxsave))
			n = sizeof(Fxsave) - offset;
		memmove(a, p+offset, n);
		break;
	}

	return n;
}

void
fpunotify(Ureg* u)
{
	/*
	 * Called when a note is about to be delivered to a
	 * user process, usually at the end of a system call.
	 */
	Proc *up = externup();
	fpusave(&up->FPU.fxsave);
}

void
fpunoted(void)
{
	// Called from sysnoted() via the machine-dependent noted() routine.
	Proc *up = externup();
	fpurestore(&up->FPU.fxsave);
}

void
fpusysrfork(Ureg* u)
{
	/*
	 * Called early in the non-interruptible path of
	 * sysrfork() via the machine-dependent syscall() routine.
	 * Save the state so that it can be easily copied
	 * to the child process later.
	 */

	Proc *up = externup();
	fpusave(&up->FPU.fxsave);
}

void
fpusysrforkchild(Proc* child, Proc* parent)
{
	/*
	 * Called later in sysrfork() via the machine-dependent
	 * sysrforkchild() routine.
	 * Copy the parent FPU state to the child.
	 */
	memmove(&child->FPU.fxsave, &parent->FPU.fxsave, sizeof(Fxsave));
}

void
fpuprocsave(Proc* p)
{
	/*
	 * Called from sched() and sleep() via the machine-dependent
	 * procsave() routine.
	 * About to go in to the scheduler.
	 */

	// The process is dead so don't save anything
	if (p->state == Moribund) {
		return;
	}

	// Save the FPU state without handling pending unmasked exceptions.
	// Postnote() can't be called here as sleep() already has up->rlock,.
	fpusave(&p->FPU.fxsave);
}

void
fpuprocrestore(Proc* p)
{
	// The process has been rescheduled and is about to run.
	fpurestore(&p->FPU.fxsave);
}

void
fpusysprocsetup(Proc* p)
{
	fpurestore(&defaultFxsave);
}

void
acfpusysprocsetup(Proc *p)
{
	fpusysprocsetup(p);
}

static char*
xfpuxm(Ureg* ureg, void* v)
{
	Proc *up = externup();
	uint32_t mxcsr;
	char *cm;

	// #XM - SIMD Floating Point Exception (Vector 19).

	// Save FPU state to check out the error.
	fpusave(&up->FPU.fxsave);

	if(ureg->ip & KZERO)
		panic("#XM: ip=%#p", ureg->ip);

	// Notify the user process.
	mxcsr = up->FPU.fxsave.mxcsr;
	if ((mxcsr & (Im|I)) == I)
		cm = "Invalid Operation";
	else if ((mxcsr & (Dm|D)) == D)
		cm = "Denormal Operand";
	else if ((mxcsr & (Zm|Z)) == Z)
		cm = "Divide-By-Zero";
	else if ((mxcsr & (Om|O)) == O)
		cm = "Numeric Overflow";
	else if ((mxcsr & (Um|U)) == U)
		cm = "Numeric Underflow";
	else if ((mxcsr & (Pm|P)) == P)
		cm = "Precision";
	else
		cm = "Unknown";

	snprint(up->genbuf, sizeof(up->genbuf), "sys: fp: %s Exception mxcsr=%#x", cm, mxcsr);
	return up->genbuf;
}

static void
fpuxm(Ureg *ureg, void *p)
{
	Proc *up = externup();

	char *n = xfpuxm(ureg, p);
	if (n != nil)
		postnote(up, 1, n, NDebug);
}

static char*
acfpuxm(Ureg *ureg, void *p)
{
	return xfpuxm(ureg, p);
}

void
fpuinit(void)
{
	// It's assumed there is an integrated FPU, so Em is cleared;
	uint64_t cr0 = cr0get();
	cr0 &= ~Em;
	cr0 |= Ne|Mp;
	cr0put(cr0);

	uint64_t cr4 = cr4get();
	cr4 |= Osxmmexcpt|Osfxsr;
	cr4put(cr4);

	memset(&defaultFxsave, 0, sizeof(defaultFxsave));

	fpusave(&defaultFxsave);

	defaultFxsave.mxcsrmask = 0x0000FFBF;
	defaultFxsave.mxcsr &= Rn|Pm|Um|Dm;

	if (machp()->machno != 0)
		return;

	// Set up the exception handlers.
	trapenable(IdtXM, fpuxm, 0, "#XM");
	actrapenable(IdtXM, acfpuxm, 0, "#XM");
}
