/*
 * arm co-processors
 * CP15 (system control) is the one that gets used the most in practice.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "arm.h"

#define MAP2PCSPACE(va, pc) ((uintptr)(va) & ~KSEGM | (pc) & KSEGM)

enum {
	/* alternates:	0xe12fff1e	BX (R14); last e is R14 */
	/*		0xe28ef000	B 0(R14); second e is R14 (ken) */
	Retinst	= 0xe1a0f00e,		/* MOV R14, R15 */
};

void
cpwr(int cp, int op1, int crn, int crm, int op2, ulong val)
{
	volatile ulong instr[2];
	void *pcaddr;
	void (*fp)(ulong);

	op1 &= 7;
	op2 &= 7;
	crn &= 017;
	crm &= 017;
	cp &= 017;
	/* MCR.  Rt will be R0. */
	instr[0] = 0xee000010 |
		op1 << 21 | crn << 16 | cp << 8 | op2 << 5 | crm;
	instr[1] = Retinst;

	pcaddr = (void *)MAP2PCSPACE(instr, getcallerpc(&cp));
	cachedwbse(pcaddr, sizeof instr);
	cacheiinv();

	fp = (void (*)(ulong))pcaddr;
	(*fp)(val);
	coherence();
}

void
cpwrsc(int op1, int crn, int crm, int op2, ulong val)
{
	cpwr(CpSC, op1, crn, crm, op2, val);
}

ulong
cprd(int cp, int op1, int crn, int crm, int op2)
{
	volatile ulong instr[2];
	void *pcaddr;
	ulong (*fp)(void);

	op1 &= 7;
	op2 &= 7;
	crn &= 017;
	crm &= 017;
	/*
	 * MRC.  return value will be in R0, which is convenient.
	 * Rt will be R0.
	 */
	instr[0] = 0xee100010 |
		op1 << 21 | crn << 16 | cp << 8 | op2 << 5 | crm;
	instr[1] = Retinst;

	pcaddr = (void *)MAP2PCSPACE(instr, getcallerpc(&cp));
	cachedwbse(pcaddr, sizeof instr);
	cacheiinv();

	fp = (ulong (*)(void))pcaddr;
	return (*fp)();
}

ulong
cprdsc(int op1, int crn, int crm, int op2)
{
	return cprd(CpSC, op1, crn, crm, op2);
}

/* floating point */

ulong
fprd(int fpreg)
{
	volatile ulong instr[2];
	void *pcaddr;
	ulong (*fp)(void);

	fpreg &= 017;
	/*
	 * VMRS.  return value will be in R0, which is convenient.
	 * Rt will be R0.
	 */
	instr[0] = 0xeef00a10 | fpreg << 16 | 0 << 12;
	instr[1] = Retinst;
	coherence();

	pcaddr = (void *)MAP2PCSPACE(instr, getcallerpc(&fpreg));
	cachedwbse(pcaddr, sizeof instr);
	cacheiinv();

	fp = (ulong (*)(void))pcaddr;
	return (*fp)();
}

void
fpwr(int fpreg, ulong val)
{
	volatile ulong instr[2];
	void *pcaddr;
	void (*fp)(ulong);

	fpreg &= 017;
	/* VMSR.  Rt will be R0. */
	instr[0] = 0xeee00a10 | fpreg << 16 | 0 << 12;
	instr[1] = Retinst;
	coherence();

	pcaddr = (void *)MAP2PCSPACE(instr, getcallerpc(&fpreg));
	cachedwbse(pcaddr, sizeof instr);
	cacheiinv();

	fp = (void (*)(ulong))pcaddr;
	(*fp)(val);
	coherence();
}
