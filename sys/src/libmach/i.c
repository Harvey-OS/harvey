/*
 * RISC-V definition
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "/riscv/include/ureg.h"
#include <mach.h>

#define	REGOFF(x)	(ulong)(&((struct Ureg *) 0)->x)
#define	REGSIZE		sizeof(struct Ureg)

Reglist riscvreglist[] = {
	{"PC",		REGOFF(pc),	RINT, 'X'},
	{"SP",		REGOFF(r2),	RINT, 'X'},
	{"R31",		REGOFF(r31),	RINT, 'X'},
	{"R30",		REGOFF(r30),	RINT, 'X'},
	{"R28",		REGOFF(r28),	RINT, 'X'},
	{"R27",		REGOFF(r27),	RINT, 'X'},
	{"R26",		REGOFF(r26),	RINT, 'X'},
	{"R25",		REGOFF(r25),	RINT, 'X'},
	{"R24",		REGOFF(r24),	RINT, 'X'},
	{"R23",		REGOFF(r23),	RINT, 'X'},
	{"R22",		REGOFF(r22),	RINT, 'X'},
	{"R21",		REGOFF(r21),	RINT, 'X'},
	{"R20",		REGOFF(r20),	RINT, 'X'},
	{"R19",		REGOFF(r19),	RINT, 'X'},
	{"R18",		REGOFF(r18),	RINT, 'X'},
	{"R17",		REGOFF(r17),	RINT, 'X'},
	{"R16",		REGOFF(r16),	RINT, 'X'},
	{"R15",		REGOFF(r15),	RINT, 'X'},
	{"R14",		REGOFF(r14),	RINT, 'X'},
	{"R13",		REGOFF(r13),	RINT, 'X'},
	{"R12",		REGOFF(r12),	RINT, 'X'},
	{"R11",		REGOFF(r11),	RINT, 'X'},
	{"R10",		REGOFF(r10),	RINT, 'X'},
	{"R9",		REGOFF(r9),	RINT, 'X'},
	{"R8",		REGOFF(r8),	RINT, 'X'},
	{"R7",		REGOFF(r7),	RINT, 'X'},
	{"R6",		REGOFF(r6),	RINT, 'X'},
	{"R5",		REGOFF(r5),	RINT, 'X'},
	{"R4",		REGOFF(r4),	RINT, 'X'},
	{"R3",		REGOFF(r3),	RINT, 'X'},
	{"R2",		REGOFF(r2),	RINT, 'X'},
	{"R1",		REGOFF(r1),	RINT, 'X'},
	{  0 }
};

	/* the machine description */
Mach mriscv =
{
	"riscv",	/* cputype */
	MRISCV,		/* machine type */
	riscvreglist,	/* register set */
	REGSIZE,	/* register set size */
	0,		/* fp register set size */
	"PC",		/* name of PC */
	"SP",		/* name of SP */
	"R1",		/* name of link register */
	"setSB",	/* static base register name */
	0,		/* static base register value */
	0x1000,		/* page size */
	0x80000000ULL,	/* kernel base */
	0xC0000000ULL,	/* kernel text mask */
	0x7FFFFFFFULL,	/* user stack top */
	2,		/* quantization of pc (could be compressed) */
	4,		/* szaddr */
	4,		/* szreg */
	4,		/* szfloat */
	8,		/* szdouble */
};
