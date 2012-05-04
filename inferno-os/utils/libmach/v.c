/*
 * mips definition
 */
#include <lib9.h>
#include <bio.h>
#include "uregv.h"
#include "mach.h"

#define	REGOFF(x)	(ulong)(&((struct Ureg *) 0)->x)

#define SP		REGOFF(u0.sp)
#define PC		REGOFF(pc)
#define	R1		REGOFF(r1)
#define	R31		REGOFF(r31)
#define	FP_REG(x)	(R1+4+4*(x))

#define	REGSIZE		sizeof(struct Ureg)
#define	FPREGSIZE	(4*33)

Reglist mipsreglist[] = {
	{"STATUS",	REGOFF(status),		RINT|RRDONLY, 'X'},
	{"CAUSE",	REGOFF(cause),		RINT|RRDONLY, 'X'},
	{"BADVADDR",	REGOFF(badvaddr),	RINT|RRDONLY, 'X'},
	{"TLBVIRT",	REGOFF(tlbvirt),	RINT|RRDONLY, 'X'},
	{"HI",		REGOFF(hi),		RINT|RRDONLY, 'X'},
	{"LO",		REGOFF(lo),		RINT|RRDONLY, 'X'},
	{"PC",		PC,		RINT, 'X'},
	{"SP",		SP,		RINT, 'X'},
	{"R31",		R31,		RINT, 'X'},
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
	{"F0",		FP_REG(0),	RFLT, 'F'},
	{"F1",		FP_REG(1),	RFLT, 'f'},
	{"F2",		FP_REG(2),	RFLT, 'F'},
	{"F3",		FP_REG(3),	RFLT, 'f'},
	{"F4",		FP_REG(4),	RFLT, 'F'},
	{"F5",		FP_REG(5),	RFLT, 'f'},
	{"F6",		FP_REG(6),	RFLT, 'F'},
	{"F7",		FP_REG(7),	RFLT, 'f'},
	{"F8",		FP_REG(8),	RFLT, 'F'},
	{"F9",		FP_REG(9),	RFLT, 'f'},
	{"F10",		FP_REG(10),	RFLT, 'F'},
	{"F11",		FP_REG(11),	RFLT, 'f'},
	{"F12",		FP_REG(12),	RFLT, 'F'},
	{"F13",		FP_REG(13),	RFLT, 'f'},
	{"F14",		FP_REG(14),	RFLT, 'F'},
	{"F15",		FP_REG(15),	RFLT, 'f'},
	{"F16",		FP_REG(16),	RFLT, 'F'},
	{"F17",		FP_REG(17),	RFLT, 'f'},
	{"F18",		FP_REG(18),	RFLT, 'F'},
	{"F19",		FP_REG(19),	RFLT, 'f'},
	{"F20",		FP_REG(20),	RFLT, 'F'},
	{"F21",		FP_REG(21),	RFLT, 'f'},
	{"F22",		FP_REG(22),	RFLT, 'F'},
	{"F23",		FP_REG(23),	RFLT, 'f'},
	{"F24",		FP_REG(24),	RFLT, 'F'},
	{"F25",		FP_REG(25),	RFLT, 'f'},
	{"F26",		FP_REG(26),	RFLT, 'F'},
	{"F27",		FP_REG(27),	RFLT, 'f'},
	{"F28",		FP_REG(28),	RFLT, 'F'},
	{"F29",		FP_REG(29),	RFLT, 'f'},
	{"F30",		FP_REG(30),	RFLT, 'F'},
	{"F31",		FP_REG(31),	RFLT, 'f'},
	{"FPCR",	FP_REG(32),	RFLT, 'X'},
	{  0 }
};

	/* the machine description */
Mach mmips =
{
	"mips",
	MMIPS,		/* machine type */
	mipsreglist,	/* register set */
	REGSIZE,	/* number of bytes in reg set */
	FPREGSIZE,	/* number of bytes in fp reg set */
	"PC",		/* name of PC */
	"SP",		/* name of SP */
	"R31",		/* name of link register */
	"setR30",	/* static base register name */
	0,		/* value */
	0x1000,		/* page size */
	0xC0000000,	/* kernel base */
	0x40000000,	/* kernel text mask */
	4,		/* quantization of pc */
	4,		/* szaddr */
	4,		/* szreg */
	4,		/* szfloat */
	8,		/* szdouble */
};
