/*
 * sparc definition
 */
#include "u.h"
#include "/sparc/include/ureg.h"
#include "mach.h"

#define USER_REG(x)	(0x1000-((32+6)*4)+(ulong)(x))
#define	FP_REG(x)	(0x0000+4+(x))
#define	SCALLOFF	(0x0000+4+168)

#define	REGOFF(x)	(USER_REG(&((struct Ureg *) 0)->x))

#define R1		REGOFF(r1)
#define PC		REGOFF(pc)
#define	R15		REGOFF(r15)

#define	MAXREG	(3+31+2+1+32)
#define	MINREG	1

Reglist sparcreglist[] = {
	{"Y",		REGOFF(y), 0},
	{"TBR",		REGOFF(tbr), 0},
	{"PSR",		REGOFF(psr), 0},
/* first register we may write */
	{"R1",		REGOFF(r1), 0},
	{"R2",		REGOFF(r2), 0},
	{"R3",		REGOFF(r3), 0},
	{"R4",		REGOFF(r4), 0},
	{"R5",		REGOFF(r5), 0},
	{"R6",		REGOFF(r6), 0},
	{"R7",		REGOFF(r7), 0},
	{"R8",		REGOFF(r8), 0},
	{"R9",		REGOFF(r9), 0},
	{"R10",		REGOFF(r10), 0},
	{"R11",		REGOFF(r11), 0},
	{"R12",		REGOFF(r12), 0},
	{"R13",		REGOFF(r13), 0},
	{"R14",		REGOFF(r14), 0},
	{"R15",		REGOFF(r15), 0},
	{"R16",		REGOFF(r16), 0},
	{"R17",		REGOFF(r17), 0},
	{"R18",		REGOFF(r18), 0},
	{"R19",		REGOFF(r19), 0},
	{"R20",		REGOFF(r20), 0},
	{"R21",		REGOFF(r21), 0},
	{"R22",		REGOFF(r22), 0},
	{"R23",		REGOFF(r23), 0},
	{"R24",		REGOFF(r24), 0},
	{"R25",		REGOFF(r25), 0},
	{"R26",		REGOFF(r26), 0},
	{"R27",		REGOFF(r27), 0},
	{"R28",		REGOFF(r28), 0},
	{"R29",		REGOFF(r29), 0},
	{"R30",		REGOFF(r30), 0},
	{"R31",		REGOFF(r31), 0},
	{"NPC",		REGOFF(npc), 0},
	{"PC",		REGOFF(pc), 0},

	{"FSR",		FP_REG(0x00), 1},
	{"F0",		FP_REG(0x04), 1},
	{"F1",		FP_REG(0x08), 1},
	{"F2",		FP_REG(0x0C), 1},
	{"F3",		FP_REG(0x10), 1},
	{"F4",		FP_REG(0x14), 1},
	{"F5",		FP_REG(0x18), 1},
	{"F6",		FP_REG(0x1C), 1},
	{"F7",		FP_REG(0x20), 1},
	{"F8",		FP_REG(0x24), 1},
	{"F9",		FP_REG(0x28), 1},
	{"F10",		FP_REG(0x2C), 1},
	{"F11",		FP_REG(0x30), 1},
	{"F12",		FP_REG(0x34), 1},
	{"F13",		FP_REG(0x38), 1},
	{"F14",		FP_REG(0x3C), 1},
	{"F15",		FP_REG(0x40), 1},
	{"F16",		FP_REG(0x44), 1},
	{"F17",		FP_REG(0x48), 1},
	{"F18",		FP_REG(0x4C), 1},
	{"F19",		FP_REG(0x50), 1},
	{"F20",		FP_REG(0x54), 1},
	{"F21",		FP_REG(0x58), 1},
	{"F22",		FP_REG(0x5C), 1},
	{"F23",		FP_REG(0x60), 1},
	{"F24",		FP_REG(0x64), 1},
	{"F25",		FP_REG(0x68), 1},
	{"F26",		FP_REG(0x6C), 1},
	{"F27",		FP_REG(0x70), 1},
	{"F28",		FP_REG(0x74), 1},
	{"F29",		FP_REG(0x78), 1},
	{"F30",		FP_REG(0x7C), 1},
	{"F31",		FP_REG(0x80), 1},
	{  0 }
};

/*
 * sparc has same stack format as mips
 */
Mach msparc =
{
	"sparc",
	sparcreglist,	/* register list */
	MINREG,		/* minimum register */
	MAXREG,		/* maximum register */
	PC,
	R1,
	R15,
	3,		/* first writable register */
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
