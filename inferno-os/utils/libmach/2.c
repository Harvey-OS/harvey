/*
 * 68020 definition
 */
#include <lib9.h>
#include "ureg2.h"
#include <bio.h>
#include "mach.h"

#define	MAXREG	0
#define	MINREG	0

#define	REGOFF(x)	(ulong)(&((struct Ureg *) 0)->x)

#define	VO	REGOFF(vo)		/* vo, 2 bytes */
#define	SR	REGOFF(sr)		/* sr, 2 bytes */
#define	R0	REGOFF(r0)
#define	PC	REGOFF(pc)
#define	DBMAGIC	REGOFF(magic)
#define	SP	REGOFF(usp)

#define	REGSIZE		(R0+4)
#define	FCTL(x)		(REGSIZE+(x)*4)
#define	FREG(x)		(FCTL(3)+(x)*12)
#define	FPREGSIZE	(11*12)

/*
 *	68020 register set
 */
Reglist m68020reglist[] = {
	{"VO",		VO,		RINT,	'x'},
	{"SR",		SR,		RINT,	'x'},
	{"MAGIC",	DBMAGIC,	RINT,	'X'},
	{"PC",		PC,		RINT,	'X'},
	{"A7",		SP,		RINT,	'X'},
	{"KSP",		REGOFF(sp),	RINT,	'X'},
	{"A6",		REGOFF(a6),	RINT,	'X'},
	{"A5",		REGOFF(a5),	RINT,	'X'},
	{"A4",		REGOFF(a4),	RINT,	'X'},
	{"A3",		REGOFF(a3),	RINT,	'X'},
	{"A2",		REGOFF(a2),	RINT,	'X'},
	{"A1",		REGOFF(a1),	RINT,	'X'},
	{"A0",		REGOFF(a0),	RINT,	'X'},
	{"R7",		REGOFF(r7),	RINT,	'X'},
	{"R6",		REGOFF(r6),	RINT,	'X'},
	{"R5",		REGOFF(r5),	RINT,	'X'},
	{"R4",		REGOFF(r4),	RINT,	'X'},
	{"R3",		REGOFF(r3),	RINT,	'X'},
	{"R2",		REGOFF(r2),	RINT,	'X'},
	{"R1",		REGOFF(r1),	RINT,	'X'},
	{"R0",		REGOFF(r0),	RINT,	'X'},
	{"FPCR",	FCTL(0),	RFLT,	'X'},
	{"FPSR",	FCTL(1),	RFLT,	'X'},
	{"FPIAR",	FCTL(2),	RFLT,	'X'},
	{"F0",		FREG(0),	RFLT,	'8'},
	{"F1",		FREG(1),	RFLT,	'8'},
	{"F2",		FREG(2),	RFLT,	'8'},
	{"F3",		FREG(3),	RFLT,	'8'},
	{"F4",		FREG(4),	RFLT,	'8'},
	{"F5",		FREG(5),	RFLT,	'8'},
	{"F6",		FREG(6),	RFLT,	'8'},
	{"F7",		FREG(7),	RFLT,	'8'},
	{0}
};

Mach m68020 =
{
	"68020",
	M68020,		/* machine type */
	m68020reglist,	/* register list */
	REGSIZE,	/* number of bytes in reg set */
	FPREGSIZE,	/* number of bytes in fp reg set */
	"PC",
	"A7",
	0,		/* link register */
	"a6base",	/* static base register name */
	0,		/* value */
	0x2000,		/* page size */
	0x80000000U,	/* kernel base */
	0x80000000U,	/* kernel text mask */
	0x7FFFFFFFU,	/* user stack top */
	2,		/* quantization of pc */
	4,		/* szaddr */
	4,		/* szreg */
	4,		/* szfloat */
	8,		/* szdouble */
};
