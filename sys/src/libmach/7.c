/*
 * arm64 definition
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "/arm64/include/ureg.h"
#include <mach.h>

#define	REGOFF(x)	offsetof(Ureg, x)

#define SP		REGOFF(sp)
#define PC		REGOFF(pc)

#define	REGSIZE		sizeof(Ureg)
#define	FPREGSIZE	(8*32)	/* TO DO: vregs */

Reglist arm64reglist[] =
{
	{"TYPE",		REGOFF(type),		RINT|RRDONLY, 'X'},
	{"PSR",		REGOFF(psr),		RINT|RRDONLY, 'X'},
	{"PC",		PC,				RINT, 'Y'},
	{"SP",		SP,				RINT, 'Y'},
	{"R30",		REGOFF(link),		RINT, 'Y'},
	{"R29",		REGOFF(r29),		RINT, 'Y'},
	{"R28",		REGOFF(r28),		RINT, 'Y'},
	{"R27",		REGOFF(r27),		RINT, 'Y'},
	{"R26",		REGOFF(r26),		RINT, 'Y'},
	{"R25",		REGOFF(r25),		RINT, 'Y'},
	{"R24",		REGOFF(r24),		RINT, 'Y'},
	{"R23",		REGOFF(r23),		RINT, 'Y'},
	{"R22",		REGOFF(r22),		RINT, 'Y'},
	{"R21",		REGOFF(r21),		RINT, 'Y'},
	{"R20",		REGOFF(r20),		RINT, 'Y'},
	{"R19",		REGOFF(r19),		RINT, 'Y'},
	{"R18",		REGOFF(r18),		RINT, 'Y'},
	{"R17",		REGOFF(r17),		RINT, 'Y'},
	{"R16",		REGOFF(r16),		RINT, 'Y'},
	{"R15",		REGOFF(r15),		RINT, 'Y'},
	{"R14",		REGOFF(r14),		RINT, 'Y'},
	{"R13",		REGOFF(r13),		RINT, 'Y'},
	{"R12",		REGOFF(r12),		RINT, 'Y'},
	{"R11",		REGOFF(r11),		RINT, 'Y'},
	{"R10",		REGOFF(r10),		RINT, 'Y'},
	{"R9",		REGOFF(r9),		RINT, 'Y'},
	{"R8",		REGOFF(r8),		RINT, 'Y'},
	{"R7",		REGOFF(r7),		RINT, 'Y'},
	{"R6",		REGOFF(r6),		RINT, 'Y'},
	{"R5",		REGOFF(r5),		RINT, 'Y'},
	{"R4",		REGOFF(r4),		RINT, 'Y'},
	{"R3",		REGOFF(r3),		RINT, 'Y'},
	{"R2",		REGOFF(r2),		RINT, 'Y'},
	{"R1",		REGOFF(r1),		RINT, 'Y'},
	{"R0",		REGOFF(r0),		RINT, 'Y'},
	{  0 }
};

	/* the machine description */
Mach marm64 =
{
	"arm64",
	MARM64,		/* machine type */
	arm64reglist,	/* register set */
	REGSIZE,	/* register set size */
	FPREGSIZE,	/* fp register set size */
	"PC",		/* name of PC */
	"SP",		/* name of SP */
	"LR",		/* name of link register */
	"setSB",	/* static base register name */
	0,		/* static base register value */
	0x100000,		/* page size (really text/data boundary rounding) */
	0xfffffffff0110000ull,	/* kernel base */
	0xffff800000000000ull,	/* kernel text mask */
	0x00007ffffffff000ull,	/* user stack top */
	4,		/* quantization of pc */
	8,		/* szaddr */
	8,		/* szreg */
	4,		/* szfloat */
	8,		/* szdouble */
};
