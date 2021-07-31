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
#define FP_REG(x)	(0x0000+4+7*4+10*(x))
#define	SCALLOFF	(0x0000+4+108)

#define	REGOFF(x)	(USER_REG(&((struct Ureg *) 0)->x))

#define PC		REGOFF(pc)
#define SP		REGOFF(sp)
#define	AX		REGOFF(ax)

Reglist i386reglist[] = {
	{"DI",		REGOFF(di),	RINT, 'X'},
	{"SI",		REGOFF(si),	RINT, 'X'},
	{"BP",		REGOFF(bp),	RINT, 'X'},
	{"BX",		REGOFF(bx),	RINT, 'X'},
	{"DX",		REGOFF(dx),	RINT, 'X'},
	{"CX",		REGOFF(cx),	RINT, 'X'},
	{"AX",		REGOFF(ax),	RINT, 'X'},
	{"GS",		REGOFF(gs),	RINT, 'X'},
	{"FS",		REGOFF(fs),	RINT, 'X'},
	{"ES",		REGOFF(es),	RINT, 'X'},
	{"DS",		REGOFF(ds),	RINT, 'X'},
	{"TRAP",	REGOFF(trap), 	RINT, 'X'},
	{"ECODE",	REGOFF(ecode),	RINT, 'X'},
	{"PC",		PC,		RINT, 'X'},
	{"CS",		REGOFF(cs),	RINT, 'X'},
	{"EFLAGS",	REGOFF(flags),	RINT, 'X'},
	{"SP",		SP,		RINT, 'X'},
	{"SS",		REGOFF(ss),	RINT, 'X'},

	{"E0",		FP_CTL(0),	RFLT, 'X'},
	{"E1",		FP_CTL(1),	RFLT, 'X'},
	{"E2",		FP_CTL(2),	RFLT, 'X'},
	{"E3",		FP_CTL(3),	RFLT, 'X'},
	{"E4",		FP_CTL(4),	RFLT, 'X'},
	{"E5",		FP_CTL(5),	RFLT, 'X'},
	{"E6",		FP_CTL(6),	RFLT, 'X'},
	{"F0",		FP_REG(7),	RFLT, '3'},
	{"F1",		FP_REG(6),	RFLT, '3'},
	{"F2",		FP_REG(5),	RFLT, '3'},
	{"F3",		FP_REG(4),	RFLT, '3'},
	{"F4",		FP_REG(3),	RFLT, '3'},
	{"F5",		FP_REG(2),	RFLT, '3'},
	{"F6",		FP_REG(1),	RFLT, '3'},
	{"F7",		FP_REG(0),	RFLT, '3'},
	{  0 }
};

Mach mi386 =
{
	"386",
	MI386,		/* machine type */
	i386reglist,	/* register list */
	MINREG,		/* minimum register */
	MAXREG,		/* maximum register */
	"PC",
	"SP",
	0,		/* link register */
	AX,		/* return reg */
	0x1000,		/* page size */
	0x80100000,	/* kernel base */
	0,		/* kernel text mask */
	0,		/* offset of ksp in /proc/proc */
	4,		/* correction to ksp value */
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
