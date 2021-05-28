/*
 * RISC-V RV64 definition
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "/riscv64/include/ureg.h"
#include <mach.h>

#define	REGOFF(x)	(u64int)(&((struct Ureg *) 0)->x)
#define	REGSIZE		sizeof(struct Ureg)

#define RCURMODE	REGOFF(curmode)
#define FP_REG(x)	(RCURMODE+8+8*(x))
#define	FPREGSIZE	(8*33)	

Reglist riscv64reglist[] = {
	{"STATUS",	REGOFF(status),	RINT|RRDONLY, 'Y'},
	{"CAUSE",	REGOFF(cause),	RINT|RRDONLY, 'Y'},
	{"IE",		REGOFF(ie),	RINT|RRDONLY, 'Y'},
	{"TVAL",	REGOFF(tval),	RINT|RRDONLY, 'Y'},
	{"CURMODE",	REGOFF(curmode), RINT|RRDONLY, 'Y'},
	{"PC",		REGOFF(pc),	RINT, 'Y'},
	{"SP",		REGOFF(r2),	RINT, 'Y'},
	{"R31",		REGOFF(r31),	RINT, 'Y'},
	{"R30",		REGOFF(r30),	RINT, 'Y'},
	{"R28",		REGOFF(r28),	RINT, 'Y'},
	{"R27",		REGOFF(r27),	RINT, 'Y'},
	{"R26",		REGOFF(r26),	RINT, 'Y'},
	{"R25",		REGOFF(r25),	RINT, 'Y'},
	{"R24",		REGOFF(r24),	RINT, 'Y'},
	{"R23",		REGOFF(r23),	RINT, 'Y'},
	{"R22",		REGOFF(r22),	RINT, 'Y'},
	{"R21",		REGOFF(r21),	RINT, 'Y'},
	{"R20",		REGOFF(r20),	RINT, 'Y'},
	{"R19",		REGOFF(r19),	RINT, 'Y'},
	{"R18",		REGOFF(r18),	RINT, 'Y'},
	{"R17",		REGOFF(r17),	RINT, 'Y'},
	{"R16",		REGOFF(r16),	RINT, 'Y'},
	{"R15",		REGOFF(r15),	RINT, 'Y'},
	{"R14",		REGOFF(r14),	RINT, 'Y'},
	{"R13",		REGOFF(r13),	RINT, 'Y'},
	{"R12",		REGOFF(r12),	RINT, 'Y'},
	{"R11",		REGOFF(r11),	RINT, 'Y'},
	{"R10",		REGOFF(r10),	RINT, 'Y'},
	{"R9",		REGOFF(r9),	RINT, 'Y'},
	{"R8",		REGOFF(r8),	RINT, 'Y'},
	{"R7",		REGOFF(r7),	RINT, 'Y'},
	{"R6",		REGOFF(r6),	RINT, 'Y'},
	{"R5",		REGOFF(r5),	RINT, 'Y'},
	{"R4",		REGOFF(r4),	RINT, 'Y'},
	{"R3",		REGOFF(r3),	RINT, 'Y'},
	{"R2",		REGOFF(r2),	RINT, 'Y'},
	{"R1",		REGOFF(r1),	RINT, 'Y'},
	{"F0",		FP_REG(0),	RFLT,		'F'},
	{"F1",		FP_REG(1),	RFLT,		'F'},
	{"F2",		FP_REG(2),	RFLT,		'F'},
	{"F3",		FP_REG(3),	RFLT,		'F'},
	{"F4",		FP_REG(4),	RFLT,		'F'},
	{"F5",		FP_REG(5),	RFLT,		'F'},
	{"F6",		FP_REG(6),	RFLT,		'F'},
	{"F7",		FP_REG(7),	RFLT,		'F'},
	{"F8",		FP_REG(8),	RFLT,		'F'},
	{"F9",		FP_REG(9),	RFLT,		'F'},
	{"F10",		FP_REG(10),	RFLT,		'F'},
	{"F11",		FP_REG(11),	RFLT,		'F'},
	{"F12",		FP_REG(12),	RFLT,		'F'},
	{"F13",		FP_REG(13),	RFLT,		'F'},
	{"F14",		FP_REG(14),	RFLT,		'F'},
	{"F15",		FP_REG(15),	RFLT,		'F'},
	{"F16",		FP_REG(16),	RFLT,		'F'},
	{"F17",		FP_REG(17),	RFLT,		'F'},
	{"F18",		FP_REG(18),	RFLT,		'F'},
	{"F19",		FP_REG(19),	RFLT,		'F'},
	{"F20",		FP_REG(20),	RFLT,		'F'},
	{"F21",		FP_REG(21),	RFLT,		'F'},
	{"F22",		FP_REG(22),	RFLT,		'F'},
	{"F23",		FP_REG(23),	RFLT,		'F'},
	{"F24",		FP_REG(24),	RFLT,		'F'},
	{"F25",		FP_REG(25),	RFLT,		'F'},
	{"F26",		FP_REG(26),	RFLT,		'F'},
	{"F27",		FP_REG(27),	RFLT,		'F'},
	{"F28",		FP_REG(28),	RFLT,		'F'},
	{"F29",		FP_REG(29),	RFLT,		'F'},
	{"F30",		FP_REG(30),	RFLT,		'F'},
	{"F31",		FP_REG(31),	RFLT,		'F'},
	{"FPCSR",	FP_REG(32)+4,	RFLT,		'X'},
	{  0 }
};

	/* the machine description */
Mach mriscv64 =
{
	"riscv64",
	MRISCV64,		/* machine type */
	riscv64reglist,	/* register set */
	REGSIZE,	/* register set size */
	FPREGSIZE,	/* FP register set size */
	"PC",		/* name of PC */
	"SP",		/* name of SP */
	"R1",		/* name of link register */
	"setSB",	/* static base register name */
	0,		/* static base register value */
	0x1000,		/* page size */
	/* these are Sv39 values */
	0xffffffc080000000ULL,	/* kernel base */
	0x8000000000000000ULL,	/* kernel text mask for all Sv* */
	0x0000003fffffffffULL,	/* user stack top */
	2,		/* quantization of pc */
	8,		/* szaddr */
	8,		/* szreg */
	4,		/* szfloat */
	8,		/* szdouble */
};
