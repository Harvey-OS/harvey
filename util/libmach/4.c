/*
 * mips definition
 */
#include <lib9.h>
#include <bio.h>
#include "ureg4.h"
#include "mach.h"

#define	FPREGBYTES	4
#define	REGOFF(x)	(ulong)(&((struct Ureg *) 0)->x)

#define SP		REGOFF(u0.sp)
#define PC		REGOFF(pc)
#define	R1		REGOFF(hr1)
#define	R31		REGOFF(hr31)
#define	FP_REG(x)	(R1+8+FPREGBYTES*(x))

#define	REGSIZE		sizeof(struct Ureg)
#define	FPREGSIZE	(FPREGBYTES*33)

Reglist mips2reglist[] = {
	{"STATUS",	REGOFF(status),		RINT|RRDONLY, 'X'},
	{"CAUSE",	REGOFF(cause),		RINT|RRDONLY, 'X'},
	{"BADVADDR",	REGOFF(badvaddr),	RINT|RRDONLY, 'X'},
	{"TLBVIRT",	REGOFF(tlbvirt),	RINT|RRDONLY, 'X'},
	{"HI",		REGOFF(hhi),		RINT|RRDONLY, 'Y'},
	{"LO",		REGOFF(hlo),		RINT|RRDONLY, 'Y'},
	{"PC",		PC,		RINT, 'X'},
	{"SP",		SP,		RINT, 'X'},
	{"R31",		R31,		RINT, 'Y'},
	{"R30",		REGOFF(hr30),	RINT, 'Y'},
	{"R28",		REGOFF(hr28),	RINT, 'Y'},
	{"R27",		REGOFF(hr27),	RINT, 'Y'},
	{"R26",		REGOFF(hr26),	RINT, 'Y'},
	{"R25",		REGOFF(hr25),	RINT, 'Y'},
	{"R24",		REGOFF(hr24),	RINT, 'Y'},
	{"R23",		REGOFF(hr23),	RINT, 'Y'},
	{"R22",		REGOFF(hr22),	RINT, 'Y'},
	{"R21",		REGOFF(hr21),	RINT, 'Y'},
	{"R20",		REGOFF(hr20),	RINT, 'Y'},
	{"R19",		REGOFF(hr19),	RINT, 'Y'},
	{"R18",		REGOFF(hr18),	RINT, 'Y'},
	{"R17",		REGOFF(hr17),	RINT, 'Y'},
	{"R16",		REGOFF(hr16),	RINT, 'Y'},
	{"R15",		REGOFF(hr15),	RINT, 'Y'},
	{"R14",		REGOFF(hr14),	RINT, 'Y'},
	{"R13",		REGOFF(hr13),	RINT, 'Y'},
	{"R12",		REGOFF(hr12),	RINT, 'Y'},
	{"R11",		REGOFF(hr11),	RINT, 'Y'},
	{"R10",		REGOFF(hr10),	RINT, 'Y'},
	{"R9",		REGOFF(hr9),	RINT, 'Y'},
	{"R8",		REGOFF(hr8),	RINT, 'Y'},
	{"R7",		REGOFF(hr7),	RINT, 'Y'},
	{"R6",		REGOFF(hr6),	RINT, 'Y'},
	{"R5",		REGOFF(hr5),	RINT, 'Y'},
	{"R4",		REGOFF(hr4),	RINT, 'Y'},
	{"R3",		REGOFF(hr3),	RINT, 'Y'},
	{"R2",		REGOFF(hr2),	RINT, 'Y'},
	{"R1",		REGOFF(hr1),	RINT, 'Y'},
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
Mach mmips2be =
{
	"mips2",
	MMIPS2,		/* machine type */
	mips2reglist,	/* register set */
	REGSIZE,	/* number of bytes in reg set */
	FPREGSIZE,	/* number of bytes in fp reg set */
	"PC",		/* name of PC */
	"SP",		/* name of SP */
	"R31",		/* name of link register */
	"setR30",	/* static base register name */
	0,		/* SB value */
	0x1000,		/* page size */
	0xC0000000,	/* kernel base */
	0x40000000,	/* kernel text mask */
	4,		/* quantization of pc */
	4,		/* szaddr */
	8,		/* szreg */
	4,		/* szfloat */
	8,		/* szdouble */
};

Mach mmips2le =
{
	"mips2",
	NMIPS2,		/* machine type */
	mips2reglist,	/* register set */
	REGSIZE,	/* number of bytes in reg set */
	FPREGSIZE,	/* number of bytes in fp reg set */
	"PC",		/* name of PC */
	"SP",		/* name of SP */
	"R31",		/* name of link register */
	"setR30",	/* static base register name */
	0,		/* SB value */
	0x1000,		/* page size */
	0xC0000000,	/* kernel base */
	0x40000000,	/* kernel text mask */
	4,		/* quantization of pc */
	4,		/* szaddr */
	8,		/* szreg */
	4,		/* szfloat */
	8,		/* szdouble */
};
