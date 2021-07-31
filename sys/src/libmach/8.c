/*
 * 386 definition
 */
#include <u.h>
#include <bio.h>
#include "/386/include/ureg.h"
#include <mach.h>

#define	MINREG	0
#define	MAXREG	0

#define USIZE		0x1000
#define USER_REG(x)	(USIZE-((19)*4)+(ulong)(x))
#define FP_CTL(x)	(0x0000+4+4*(x))
#define FP_REG(x,y)	(0x0000+4+7*4+10*(x)+4*(y))
#define	SCALLOFF	(0x0000+4+108)

#define	REGOFF(x)	(USER_REG(&((struct Ureg *) 0)->x))

#define PC		REGOFF(pc)
#define SP		REGOFF(sp)
#define	AX		REGOFF(ax)

Reglist i386reglist[] = {
	{"DI",		REGOFF(di),	0, 'X'},
	{"SI",		REGOFF(si),	0, 'X'},
	{"BP",		REGOFF(bp),	0, 'X'},
	{"BX",		REGOFF(bx),	0, 'X'},
	{"DX",		REGOFF(dx),	0, 'X'},
	{"CX",		REGOFF(cx),	0, 'X'},
	{"AX",		REGOFF(ax),	0, 'X'},
	{"GS",		REGOFF(gs),	0, 'X'},
	{"FS",		REGOFF(fs),	0, 'X'},
	{"ES",		REGOFF(es),	0, 'X'},
	{"DS",		REGOFF(ds),	0, 'X'},
	{"TRAP",	REGOFF(trap), 	0, 'X'},
	{"ECODE",	REGOFF(ecode),	0, 'X'},
	{"PC",		PC,		0, 'X'},
	{"CS",		REGOFF(cs),	0, 'X'},
	{"EFLAGS",	REGOFF(flags),	0, 'X'},
	{"SP",		SP,		0, 'X'},
	{"SS",		REGOFF(ss),	0, 'X'},

	{"E0",		FP_CTL(0),	1, 'f'},
	{"E1",		FP_CTL(1),	1, 'f'},
	{"E2",		FP_CTL(2),	1, 'f'},
	{"E3",		FP_CTL(3),	1, 'f'},
	{"E4",		FP_CTL(4),	1, 'f'},
	{"E5",		FP_CTL(5),	1, 'f'},
	{"E6",		FP_CTL(6),	1, 'f'},
	{"F0L",		FP_REG(7, 0),	1, 'f'},
	{"F0M",		FP_REG(7, 1),	1, 'f'},
	{"F0H",		FP_REG(7, 2),	1, 'f'},
	{"F1L",		FP_REG(6, 0),	1, 'f'},
	{"F1M",		FP_REG(6, 1),	1, 'f'},
	{"F1H",		FP_REG(6, 2),	1, 'f'},
	{"F2L",		FP_REG(5, 0),	1, 'f'},
	{"F2M",		FP_REG(5, 1),	1, 'f'},
	{"F2H",		FP_REG(5, 2),	1, 'f'},
	{"F3L",		FP_REG(4, 0),	1, 'f'},
	{"F3M",		FP_REG(4, 1),	1, 'f'},
	{"F3H",		FP_REG(4, 2),	1, 'f'},
	{"F4L",		FP_REG(3, 0),	1, 'f'},
	{"F4M",		FP_REG(3, 1),	1, 'f'},
	{"F4H",		FP_REG(3, 2),	1, 'f'},
	{"F5L",		FP_REG(2, 0),	1, 'f'},
	{"F5M",		FP_REG(2, 1),	1, 'f'},
	{"F5H",		FP_REG(2, 2),	1, 'f'},
	{"F6L",		FP_REG(1, 0),	1, 'f'},
	{"F6M",		FP_REG(1, 1),	1, 'f'},
	{"F6H",		FP_REG(1, 2),	1, 'f'},
	{"F7L",		FP_REG(0, 0),	1, 'f'},
	{"F7M",		FP_REG(0, 1),	1, 'f'},
	{"F7H",		FP_REG(0, 2),	1, 'f'},
	{  0 }
};

Mach mi386 =
{
	"386",
	i386reglist,	/* register list */
	MINREG,		/* minimum register */
	MAXREG,		/* maximum register */
	PC,
	SP,
	0,
	AX,		/* return reg */
	0,		/* first writable register */
	0x1000,		/* page size */
	0x80000000,	/* kernel base */
	0,		/* kernel text mask */
	0,		/* offset of ksp in /proc/proc */
	0,		/* correction to ksp value */
	4,		/* offset of kpc in /proc/proc */
	0,		/* correction to kpc value */
	SCALLOFF,	/* offset in ublk to sys call # */
	1,		/* quantization of pc */
	"setSB",	/* static base register name (bogus anyways) */
	0,		/* value */
	4,		/* szaddr */
	4,		/* szreg */
	4,		/* szfloat */
	8,		/* szdouble */
};
