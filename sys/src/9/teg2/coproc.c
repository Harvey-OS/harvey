/*
 * arm co-processors
 * mainly to cope with arm hard-wiring register numbers into instructions.
 *
 * CP15 (system control) is the one that gets used the most in practice.
 * these routines must be callable from KZERO space or the 0 segment.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "arm.h"

enum {
	/* alternates:	0xe12fff1e	BX (R14); last e is R14 */
	/*		0xe28ef000	B 0(R14); second e is R14 (ken) */
	Retinst	= 0xe1a0f00e,		/* MOV R14, R15 */

	Opmask	= MASK(3),
	Regmask	= MASK(4),
};

typedef ulong (*Pufv)(void);
typedef void  (*Pvfu)(ulong);

static void
setupcpop(ulong instr[2], ulong opcode, int cp, int op1, int crn, int crm,
	int op2)
{
	ulong instrsz[2];

	op1 &= Opmask;
	op2 &= Opmask;
	crn &= Regmask;
	crm &= Regmask;
	cp  &= Regmask;
	instr[0] = opcode | op1 << 21 | crn << 16 | cp << 8 | op2 << 5 | crm;
	instr[1] = Retinst;

	cachedwbse(instr, sizeof instrsz);
	cacheiinv();
}

ulong
cprd(int cp, int op1, int crn, int crm, int op2)
{
	int s, r;
	volatile ulong instr[2];
	Pufv fp;

	s = splhi();
	/*
	 * MRC.  return value will be in R0, which is convenient.
	 * Rt will be R0.
	 */
	setupcpop(instr, 0xee100010, cp, op1, crn, crm, op2);
	fp = (Pufv)instr;
	r = fp();
	splx(s);
	return r;
}

void
cpwr(int cp, int op1, int crn, int crm, int op2, ulong val)
{
	int s;
	volatile ulong instr[2];
	Pvfu fp;

	s = splhi();
	setupcpop(instr, 0xee000010, cp, op1, crn, crm, op2); /* MCR, Rt is R0 */
	fp = (Pvfu)instr;
	fp(val);
	coherence();
	splx(s);
}

ulong
cprdsc(int op1, int crn, int crm, int op2)
{
	return cprd(CpSC, op1, crn, crm, op2);
}

void
cpwrsc(int op1, int crn, int crm, int op2, ulong val)
{
	cpwr(CpSC, op1, crn, crm, op2, val);
}

/* floating point */

/* fp coproc control */
static void
setupfpctlop(ulong instr[2], int opcode, int fpctlreg)
{
	ulong instrsz[2];

	fpctlreg &= Nfpctlregs - 1;
	instr[0] = opcode | fpctlreg << 16 | 0 << 12 | CpFP << 8;
	instr[1] = Retinst;

	cachedwbse(instr, sizeof instrsz);
	cacheiinv();
}

ulong
fprd(int fpreg)
{
	int s, r;
	volatile ulong instr[2];
	Pufv fp;

	if (!m->fpon) {
		dumpstack();
		panic("fprd: cpu%d fpu off", m->machno);
	}
	s = splhi();
	/*
	 * VMRS.  return value will be in R0, which is convenient.
	 * Rt will be R0.
	 */
	setupfpctlop(instr, 0xeef00010, fpreg);
	fp = (Pufv)instr;
	r = fp();
	splx(s);
	return r;
}

void
fpwr(int fpreg, ulong val)
{
	int s;
	volatile ulong instr[2];
	Pvfu fp;

	/* fpu might be off and this VMSR might enable it */
	s = splhi();
	setupfpctlop(instr, 0xeee00010, fpreg);		/* VMSR, Rt is R0 */
	fp = (Pvfu)instr;
	fp(val);
	coherence();
	splx(s);
}

/* fp register access; don't bother with single precision */
static void
setupfpop(ulong instr[2], int opcode, int fpreg)
{
	ulong instrsz[2];

	instr[0] = opcode | 0 << 16 | (fpreg & (16 - 1)) << 12;
	if (fpreg >= 16)
		instr[0] |= 1 << 22;		/* high bit of dfp reg # */
	instr[1] = Retinst;

	cachedwbse(instr, sizeof instrsz);
	cacheiinv();
}

ulong
fpsavereg(int fpreg, uvlong *fpp)
{
	int s, r;
	volatile ulong instr[2];
	ulong (*fp)(uvlong *);

	if (!m->fpon)
		panic("fpsavereg: cpu%d fpu off", m->machno);
	s = splhi();
	/*
	 * VSTR.  pointer will be in R0, which is convenient.
	 * Rt will be R0.
	 */
	setupfpop(instr, 0xed000000 | CpDFP << 8, fpreg);
	fp = (ulong (*)(uvlong *))instr;
	r = fp(fpp);
	splx(s);
	coherence();
	return r;			/* not too meaningful */
}

void
fprestreg(int fpreg, uvlong val)
{
	int s;
	volatile ulong instr[2];
	void (*fp)(uvlong *);

	if (!m->fpon)
		panic("fprestreg: cpu%d fpu off", m->machno);
	s = splhi();
	setupfpop(instr, 0xed100000 | CpDFP << 8, fpreg); /* VLDR, Rt is R0 */
	fp = (void (*)(uvlong *))instr;
	fp(&val);
	coherence();
	splx(s);
}
