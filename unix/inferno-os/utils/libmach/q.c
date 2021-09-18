/*
 * PowerPC definition
 *	forsyth@terzarima.net
 */
#include <lib9.h>
#include <bio.h>
#include "uregq.h"
#include "mach.h"


#define	REGOFF(x)	(ulong) (&((struct Ureg *) 0)->x)

#define SP		REGOFF(sp)
#define PC		REGOFF(pc)
#define	R3		REGOFF(r3)	/* return reg */
#define	LR		REGOFF(lr)
#define R31		REGOFF(r31)
#define FP_REG(x)	(R31+4+8*(x))

#define	REGSIZE		sizeof(struct Ureg)
#define	FPREGSIZE	(8*33)	

Reglist powerreglist[] = {
	{"CAUSE",	REGOFF(cause),	RINT|RRDONLY,	'X'},
	{"SRR1",	REGOFF(srr1),	RINT|RRDONLY,	'X'},
	{"PC",		REGOFF(pc),	RINT,		'X'},
	{"LR",		REGOFF(lr),	RINT,		'X'},
	{"CR",		REGOFF(cr),	RINT,		'X'},
	{"XER",		REGOFF(xer),	RINT,		'X'},
	{"CTR",		REGOFF(ctr),	RINT,		'X'},
	{"PC",		PC,		RINT,		'X'},
	{"SP",		SP,		RINT,		'X'},
	{"R0",		REGOFF(r0),	RINT,		'X'},
	/* R1 is SP */
	{"R2",		REGOFF(r2),	RINT,		'X'},
	{"R3",		REGOFF(r3),	RINT,		'X'},
	{"R4",		REGOFF(r4),	RINT,		'X'},
	{"R5",		REGOFF(r5),	RINT,		'X'},
	{"R6",		REGOFF(r6),	RINT,		'X'},
	{"R7",		REGOFF(r7),	RINT,		'X'},
	{"R8",		REGOFF(r8),	RINT,		'X'},
	{"R9",		REGOFF(r9),	RINT,		'X'},
	{"R10",		REGOFF(r10),	RINT,		'X'},
	{"R11",		REGOFF(r11),	RINT,		'X'},
	{"R12",		REGOFF(r12),	RINT,		'X'},
	{"R13",		REGOFF(r13),	RINT,		'X'},
	{"R14",		REGOFF(r14),	RINT,		'X'},
	{"R15",		REGOFF(r15),	RINT,		'X'},
	{"R16",		REGOFF(r16),	RINT,		'X'},
	{"R17",		REGOFF(r17),	RINT,		'X'},
	{"R18",		REGOFF(r18),	RINT,		'X'},
	{"R19",		REGOFF(r19),	RINT,		'X'},
	{"R20",		REGOFF(r20),	RINT,		'X'},
	{"R21",		REGOFF(r21),	RINT,		'X'},
	{"R22",		REGOFF(r22),	RINT,		'X'},
	{"R23",		REGOFF(r23),	RINT,		'X'},
	{"R24",		REGOFF(r24),	RINT,		'X'},
	{"R25",		REGOFF(r25),	RINT,		'X'},
	{"R26",		REGOFF(r26),	RINT,		'X'},
	{"R27",		REGOFF(r27),	RINT,		'X'},
	{"R28",		REGOFF(r28),	RINT,		'X'},
	{"R29",		REGOFF(r29),	RINT,		'X'},
	{"R30",		REGOFF(r30),	RINT,		'X'},
	{"R31",		REGOFF(r31),	RINT,		'X'},
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
	{"FPSCR",	FP_REG(32)+4,	RFLT,		'X'},
	{  0 }
};

	/* the machine description */
Mach mpower =
{
	"power",
	MPOWER,		/* machine type */
	powerreglist,	/* register set */
	REGSIZE,	/* number of bytes in register set */
	FPREGSIZE,	/* number of bytes in FP register set */
	"PC",		/* name of PC */
	"SP",		/* name of SP */
	"LR",		/* name of link register */
	"setSB",	/* static base register name */
	0,		/* value */
	0x1000,		/* page size */
	0x80000000U,	/* kernel base */
	0xF0000000U,	/* kernel text mask */
	0x7FFFFFFFU,	/* user stack top */
	4,		/* quantization of pc */
	4,		/* szaddr */
	4,		/* szreg */
	4,		/* szfloat */
	8,		/* szdouble */
};
