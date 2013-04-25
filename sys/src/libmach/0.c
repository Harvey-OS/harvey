/*
 * mips r4k definition
 *
 * currently no compiler - not related to 0c
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "mips2ureg.h"
#include <mach.h>

#define	FPREGBYTES	4
#define	REGOFF(x)	(ulong)(&((struct Ureg *) 0)->x)

#define SP		REGOFF(sp)
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
	{"HI",		REGOFF(hhi),		RINT|RRDONLY, 'W'},
	{"LO",		REGOFF(hlo),		RINT|RRDONLY, 'W'},
	{"PC",		PC,		RINT, 'X'},
	{"SP",		SP,		RINT, 'X'},
	{"R31",		R31,		RINT, 'W'},
	{"R30",		REGOFF(hr30),	RINT, 'W'},
	{"R28",		REGOFF(hr28),	RINT, 'W'},
	{"R27",		REGOFF(hr27),	RINT, 'W'},
	{"R26",		REGOFF(hr26),	RINT, 'W'},
	{"R25",		REGOFF(hr25),	RINT, 'W'},
	{"R24",		REGOFF(hr24),	RINT, 'W'},
	{"R23",		REGOFF(hr23),	RINT, 'W'},
	{"R22",		REGOFF(hr22),	RINT, 'W'},
	{"R21",		REGOFF(hr21),	RINT, 'W'},
	{"R20",		REGOFF(hr20),	RINT, 'W'},
	{"R19",		REGOFF(hr19),	RINT, 'W'},
	{"R18",		REGOFF(hr18),	RINT, 'W'},
	{"R17",		REGOFF(hr17),	RINT, 'W'},
	{"R16",		REGOFF(hr16),	RINT, 'W'},
	{"R15",		REGOFF(hr15),	RINT, 'W'},
	{"R14",		REGOFF(hr14),	RINT, 'W'},
	{"R13",		REGOFF(hr13),	RINT, 'W'},
	{"R12",		REGOFF(hr12),	RINT, 'W'},
	{"R11",		REGOFF(hr11),	RINT, 'W'},
	{"R10",		REGOFF(hr10),	RINT, 'W'},
	{"R9",		REGOFF(hr9),	RINT, 'W'},
	{"R8",		REGOFF(hr8),	RINT, 'W'},
	{"R7",		REGOFF(hr7),	RINT, 'W'},
	{"R6",		REGOFF(hr6),	RINT, 'W'},
	{"R5",		REGOFF(hr5),	RINT, 'W'},
	{"R4",		REGOFF(hr4),	RINT, 'W'},
	{"R3",		REGOFF(hr3),	RINT, 'W'},
	{"R2",		REGOFF(hr2),	RINT, 'W'},
	{"R1",		REGOFF(hr1),	RINT, 'W'},
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

	/* mips r4k big-endian */
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
	0x7FFFFFFFULL,	/* user stack top */
	4,		/* quantization of pc */
	4,		/* szaddr */
	8,		/* szreg */
	4,		/* szfloat */
	8,		/* szdouble */
};

	/* mips r4k little-endian */
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
	0xC0000000ULL,	/* kernel base */
	0x40000000ULL,	/* kernel text mask */
	0x7FFFFFFFULL,	/* user stack top */
	4,		/* quantization of pc */
	4,		/* szaddr */
	8,		/* szreg */
	4,		/* szfloat */
	8,		/* szdouble */
};
