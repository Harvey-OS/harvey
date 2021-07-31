/*
 * sparc definition
 */
#include <u.h>
#include <bio.h>
#include "/sparc/include/ureg.h"
#include <mach.h>

#define USER_REG(x)	(0x1000-((32+6)*4)+(ulong)(x))
#define	FP_REG(x)	(0x0000+4+(x))
#define	SCALLOFF	(0x0000+4+168)

#define	REGOFF(x)	(USER_REG(&((struct Ureg *) 0)->x))

#define R1		REGOFF(r1)
#define R7		REGOFF(r7)
#define PC		REGOFF(pc)
#define	R15		REGOFF(r15)

#define	MAXREG	(3+31+2+1+32)
#define	MINREG	1

Reglist sparcreglist[] = {
	{"Y",		REGOFF(y),	RINT|RRDONLY, 'X'},
	{"TBR",		REGOFF(tbr),	RINT|RRDONLY, 'X'},
	{"PSR",		REGOFF(psr),	RINT|RRDONLY, 'X'},
	{"PC",		REGOFF(pc),	RINT, 'X'},
	{"R1",		REGOFF(r1),	RINT, 'X'},
	{"R2",		REGOFF(r2),	RINT, 'X'},
	{"R3",		REGOFF(r3),	RINT, 'X'},
	{"R4",		REGOFF(r4),	RINT, 'X'},
	{"R5",		REGOFF(r5),	RINT, 'X'},
	{"R6",		REGOFF(r6),	RINT, 'X'},
	{"R7",		REGOFF(r7),	RINT, 'X'},
	{"R8",		REGOFF(r8),	RINT, 'X'},
	{"R9",		REGOFF(r9),	RINT, 'X'},
	{"R10",		REGOFF(r10),	RINT, 'X'},
	{"R11",		REGOFF(r11),	RINT, 'X'},
	{"R12",		REGOFF(r12),	RINT, 'X'},
	{"R13",		REGOFF(r13),	RINT, 'X'},
	{"R14",		REGOFF(r14),	RINT, 'X'},
	{"R15",		REGOFF(r15),	RINT, 'X'},
	{"R16",		REGOFF(r16),	RINT, 'X'},
	{"R17",		REGOFF(r17),	RINT, 'X'},
	{"R18",		REGOFF(r18),	RINT, 'X'},
	{"R19",		REGOFF(r19),	RINT, 'X'},
	{"R20",		REGOFF(r20),	RINT, 'X'},
	{"R21",		REGOFF(r21),	RINT, 'X'},
	{"R22",		REGOFF(r22),	RINT, 'X'},
	{"R23",		REGOFF(r23),	RINT, 'X'},
	{"R24",		REGOFF(r24),	RINT, 'X'},
	{"R25",		REGOFF(r25),	RINT, 'X'},
	{"R26",		REGOFF(r26),	RINT, 'X'},
	{"R27",		REGOFF(r27),	RINT, 'X'},
	{"R28",		REGOFF(r28),	RINT, 'X'},
	{"R29",		REGOFF(r29),	RINT, 'X'},
	{"R30",		REGOFF(r30),	RINT, 'X'},
	{"R31",		REGOFF(r31),	RINT, 'X'},
	{"NPC",		REGOFF(npc),	RINT, 'X'},

	{"FSR",		FP_REG(0x00),	RINT, 'X'},
	{"F0",		FP_REG(0x04),	RFLT, 'F'},
	{"F1",		FP_REG(0x08),	RFLT, 'f'},
	{"F2",		FP_REG(0x0C),	RFLT, 'F'},
	{"F3",		FP_REG(0x10),	RFLT, 'f'},
	{"F4",		FP_REG(0x14),	RFLT, 'F'},
	{"F5",		FP_REG(0x18),	RFLT, 'f'},
	{"F6",		FP_REG(0x1C),	RFLT, 'F'},
	{"F7",		FP_REG(0x20),	RFLT, 'f'},
	{"F8",		FP_REG(0x24),	RFLT, 'F'},
	{"F9",		FP_REG(0x28),	RFLT, 'f'},
	{"F10",		FP_REG(0x2C),	RFLT, 'F'},
	{"F11",		FP_REG(0x30),	RFLT, 'f'},
	{"F12",		FP_REG(0x34),	RFLT, 'F'},
	{"F13",		FP_REG(0x38),	RFLT, 'f'},
	{"F14",		FP_REG(0x3C),	RFLT, 'F'},
	{"F15",		FP_REG(0x40),	RFLT, 'f'},
	{"F16",		FP_REG(0x44),	RFLT, 'F'},
	{"F17",		FP_REG(0x48),	RFLT, 'f'},
	{"F18",		FP_REG(0x4C),	RFLT, 'F'},
	{"F19",		FP_REG(0x50),	RFLT, 'f'},
	{"F20",		FP_REG(0x54),	RFLT, 'F'},
	{"F21",		FP_REG(0x58),	RFLT, 'f'},
	{"F22",		FP_REG(0x5C),	RFLT, 'F'},
	{"F23",		FP_REG(0x60),	RFLT, 'f'},
	{"F24",		FP_REG(0x64),	RFLT, 'F'},
	{"F25",		FP_REG(0x68),	RFLT, 'f'},
	{"F26",		FP_REG(0x6C),	RFLT, 'F'},
	{"F27",		FP_REG(0x70),	RFLT, 'f'},
	{"F28",		FP_REG(0x74),	RFLT, 'F'},
	{"F29",		FP_REG(0x78),	RFLT, 'f'},
	{"F30",		FP_REG(0x7C),	RFLT, 'F'},
	{"F31",		FP_REG(0x80),	RFLT, 'f'},
	{  0 }
};

/*
 * sparc has same stack format as mips
 */
Mach msparc =
{
	"sparc",
	MSPARC,		/* machine type */
	sparcreglist,	/* register list */
	MINREG,		/* minimum register */
	MAXREG,		/* maximum register */
	"PC",
	"R1",
	"R15",
	R7,		
	0x1000,		/* page size */
	0xE0000000,	/* kernel base */
	0,		/* kernel text mask */
	0,		/* offset of ksp in /proc/proc */
	4,		/* correction to ksp value */
	4,		/* offset of kpc in /proc/proc */
	0,		/* correction to kpc value */
	SCALLOFF,	/* offset in ublk of sys call # */
	4,		/* quantization of pc */
	"setSB",	/* static base register name */
	0,		/* value */
	4,		/* szaddr */
	4,		/* szreg */
	4,		/* szfloat */
	8,		/* szdouble */
};
