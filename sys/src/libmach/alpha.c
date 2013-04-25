/*
 * alpha definition
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "alpha/ureg.h"
#include <mach.h>

#define	REGOFF(x)	(ulong)(&((struct Ureg *) 0)->x)

#define	REGSIZE		sizeof(struct Ureg)
#define	FPREGSIZE	(8*33)

#define	SP		REGOFF(sp)
#define	PC		REGOFF(pc)
#define	FP_REG(x)	(REGSIZE+8*(x))

Reglist alphareglist[] = {
	{"STATUS",	REGOFF(status),	RINT|RRDONLY, 'W'},
	{"TYPE",	REGOFF(type),	RINT|RRDONLY, 'W'},
	{"A0",	REGOFF(a0),	RINT|RRDONLY, 'W'},
	{"A1",	REGOFF(a1),	RINT|RRDONLY, 'W'},
	{"A2",		REGOFF(a2),	RINT|RRDONLY, 'W'},
	{"PC",		PC,	RINT, 'X'},
	{"SP",		SP,	RINT, 'X'},
	{"R29",		REGOFF(r29),	RINT, 'W'},
	{"R28",		REGOFF(r28),	RINT, 'W'},
	{"R27",		REGOFF(r27),	RINT, 'W'},
	{"R26",		REGOFF(r26),	RINT, 'W'},
	{"R25",		REGOFF(r25),	RINT, 'W'},
	{"R24",		REGOFF(r24),	RINT, 'W'},
	{"R23",		REGOFF(r23),	RINT, 'W'},
	{"R22",		REGOFF(r22),	RINT, 'W'},
	{"R21",		REGOFF(r21),	RINT, 'W'},
	{"R20",		REGOFF(r20),	RINT, 'W'},
	{"R19",		REGOFF(r19),	RINT, 'W'},
	{"R18",		REGOFF(r18),	RINT, 'W'},
	{"R17",		REGOFF(r17),	RINT, 'W'},
	{"R16",		REGOFF(r16),	RINT, 'W'},
	{"R15",		REGOFF(r15),	RINT, 'W'},
	{"R14",		REGOFF(r14),	RINT, 'W'},
	{"R13",		REGOFF(r13),	RINT, 'W'},
	{"R12",		REGOFF(r12),	RINT, 'W'},
	{"R11",		REGOFF(r11),	RINT, 'W'},
	{"R10",		REGOFF(r10),	RINT, 'W'},
	{"R9",		REGOFF(r9),	RINT, 'W'},
	{"R8",		REGOFF(r8),	RINT, 'W'},
	{"R7",		REGOFF(r7),	RINT, 'W'},
	{"R6",		REGOFF(r6),	RINT, 'W'},
	{"R5",		REGOFF(r5),	RINT, 'W'},
	{"R4",		REGOFF(r4),	RINT, 'W'},
	{"R3",		REGOFF(r3),	RINT, 'W'},
	{"R2",		REGOFF(r2),	RINT, 'W'},
	{"R1",		REGOFF(r1),	RINT, 'W'},
	{"R0",		REGOFF(r0),	RINT, 'W'},
	{"F0",		FP_REG(0),	RFLT, 'F'},
	{"F1",		FP_REG(1),	RFLT, 'F'},
	{"F2",		FP_REG(2),	RFLT, 'F'},
	{"F3",		FP_REG(3),	RFLT, 'F'},
	{"F4",		FP_REG(4),	RFLT, 'F'},
	{"F5",		FP_REG(5),	RFLT, 'F'},
	{"F6",		FP_REG(6),	RFLT, 'F'},
	{"F7",		FP_REG(7),	RFLT, 'F'},
	{"F8",		FP_REG(8),	RFLT, 'F'},
	{"F9",		FP_REG(9),	RFLT, 'F'},
	{"F10",		FP_REG(10),	RFLT, 'F'},
	{"F11",		FP_REG(11),	RFLT, 'F'},
	{"F12",		FP_REG(12),	RFLT, 'F'},
	{"F13",		FP_REG(13),	RFLT, 'F'},
	{"F14",		FP_REG(14),	RFLT, 'F'},
	{"F15",		FP_REG(15),	RFLT, 'F'},
	{"F16",		FP_REG(16),	RFLT, 'F'},
	{"F17",		FP_REG(17),	RFLT, 'F'},
	{"F18",		FP_REG(18),	RFLT, 'F'},
	{"F19",		FP_REG(19),	RFLT, 'F'},
	{"F20",		FP_REG(20),	RFLT, 'F'},
	{"F21",		FP_REG(21),	RFLT, 'F'},
	{"F22",		FP_REG(22),	RFLT, 'F'},
	{"F23",		FP_REG(23),	RFLT, 'F'},
	{"F24",		FP_REG(24),	RFLT, 'F'},
	{"F25",		FP_REG(25),	RFLT, 'F'},
	{"F26",		FP_REG(26),	RFLT, 'F'},
	{"F27",		FP_REG(27),	RFLT, 'F'},
	{"F28",		FP_REG(28),	RFLT, 'F'},
	{"F29",		FP_REG(29),	RFLT, 'F'},
	{"F30",		FP_REG(30),	RFLT, 'F'},
	{"F31",		FP_REG(31),	RFLT, 'F'},
	{"FPCR",		FP_REG(32),	RFLT, 'W'},
	{  0 }
};

	/* the machine description */
Mach malpha =
{
	"alpha",
	MALPHA,		/* machine type */
	alphareglist,	/* register set */
	REGSIZE,	/* number of bytes in reg set */
	FPREGSIZE,	/* number of bytes in fp reg set */
	"PC",
	"SP",
	"R29",
	"setSB",	/* static base register name */
	0,		/* static base register value */
	0x2000,		/* page size */
	0x80000000ULL,	/* kernel base */
	0xF0000000ULL,	/* kernel text mask */
	0x7FFFFFFFULL,	/* user stack top */
	4,		/* quantization of pc */
	4,		/* szaddr */
	8,		/* szreg (not used?) */
	4,		/* szfloat */
	8,		/* szdouble */
};
