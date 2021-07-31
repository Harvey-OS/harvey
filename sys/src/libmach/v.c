/*
 * mips definition
 */
#include <u.h>
#include <bio.h>
#include "/mips/include/ureg.h"
#include <mach.h>


#define USER_REG(x)	(0x1000-4-0xA0+(ulong)(x))
#define	FP_REG(x)	(0x0000+4+(x))
#define	SCALLOFF	(0x0000+4+132)

#define	REGOFF(x)	(USER_REG(&((struct Ureg *) 0)->x))

#define SP		REGOFF(sp)
#define PC		REGOFF(pc)
#define	R1		REGOFF(r1)
#define	R31		REGOFF(r31)

#define	MINREG	1
#define	MAXREG	64

Reglist mipsreglist[] = {
	{"STATUS",	REGOFF(status),	RINT|RRDONLY, 'X'},
	{"CAUSE",	REGOFF(cause),	RINT|RRDONLY, 'X'},
	{"BADVADDR",	REGOFF(badvaddr),	RINT|RRDONLY, 'X'},
	{"TLBVIRT",	REGOFF(tlbvirt),	RINT|RRDONLY, 'X'},
	{"HI",		REGOFF(hi),	RINT|RRDONLY, 'X'},
	{"LO",		REGOFF(lo),	RINT|RRDONLY, 'X'},
#define	FW	5	/* first register we may write */
	{"PC",		PC,	RINT, 'X'},
	{"SP",		SP,	RINT, 'X'},
	{"R31",		R31,	RINT, 'X'},
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
	{"F0",		FP_REG(0x00),	RFLT, 'F'},
	{"F1",		FP_REG(0x04),	RFLT, 'f'},
	{"F2",		FP_REG(0x08),	RFLT, 'F'},
	{"F3",		FP_REG(0x0C),	RFLT, 'f'},
	{"F4",		FP_REG(0x10),	RFLT, 'F'},
	{"F5",		FP_REG(0x14),	RFLT, 'f'},
	{"F6",		FP_REG(0x18),	RFLT, 'F'},
	{"F7",		FP_REG(0x1C),	RFLT, 'f'},
	{"F8",		FP_REG(0x20),	RFLT, 'F'},
	{"F9",		FP_REG(0x24),	RFLT, 'f'},
	{"F10",		FP_REG(0x28),	RFLT, 'F'},
	{"F11",		FP_REG(0x2C),	RFLT, 'f'},
	{"F12",		FP_REG(0x30),	RFLT, 'F'},
	{"F13",		FP_REG(0x34),	RFLT, 'f'},
	{"F14",		FP_REG(0x38),	RFLT, 'F'},
	{"F15",		FP_REG(0x3C),	RFLT, 'f'},
	{"F16",		FP_REG(0x40),	RFLT, 'F'},
	{"F17",		FP_REG(0x44),	RFLT, 'f'},
	{"F18",		FP_REG(0x48),	RFLT, 'F'},
	{"F19",		FP_REG(0x4C),	RFLT, 'f'},
	{"F20",		FP_REG(0x50),	RFLT, 'F'},
	{"F21",		FP_REG(0x54),	RFLT, 'f'},
	{"F22",		FP_REG(0x58),	RFLT, 'F'},
	{"F23",		FP_REG(0x5C),	RFLT, 'f'},
	{"F24",		FP_REG(0x60),	RFLT, 'F'},
	{"F25",		FP_REG(0x64),	RFLT, 'f'},
	{"F26",		FP_REG(0x68),	RFLT, 'F'},
	{"F27",		FP_REG(0x6C),	RFLT, 'f'},
	{"F28",		FP_REG(0x70),	RFLT, 'F'},
	{"F29",		FP_REG(0x74),	RFLT, 'f'},
	{"F30",		FP_REG(0x78),	RFLT, 'F'},
	{"F31",		FP_REG(0x7C),	RFLT, 'f'},
	{"FPCR",	FP_REG(0x80),	RFLT, 'X'},
	{  0 }
};

	/* the machine description */
Mach mmips =
{
	"mips",
	MMIPS,		/* machine type */
	mipsreglist,	/* register set */
	MINREG,		/* minimum register */
	MAXREG,		/* maximum register */
	"PC",
	"SP",
	"R31",
	R1,		/* return reg */
	0x1000,		/* page size */
	0xC0000000,	/* kernel base */
	0x40000000,	/* kernel text mask */
	0,		/* offset of ksp in /proc/proc */
	0,		/* correction to ksp value */
	4,		/* offset of kpc in /proc/proc */
	0,		/* correction to kpc value */
	SCALLOFF,	/* offset to sys call # in ublk */
	4,		/* quantization of pc */
	"setR30",	/* static base register name */
	0,		/* value */
	4,		/* szaddr */
	4,		/* szreg */
	4,		/* szfloat */
	8,		/* szdouble */
};
