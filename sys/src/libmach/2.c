/*
 * 68020 definition
 */
#include <u.h>
#include "/68020/include/ureg.h"
#include <bio.h>
#include <mach.h>

#define	MAXREG	0
#define	MINREG	0

/*
 * 68020 register offsets relative to beginning of ublock
 */
#define	USIZE		8192		/* size of the user block */
#define	UREG(x)		(USIZE-(2+4+2+(8+8+1+1)*4)+(ulong)(x))
#define	FCTL(x)		(0x0000+4+216+(x)*4)
#define	FREG(x)		(0x0000+4+216+12+(x)*12)
#define	SCALLOFF	(0x0000+4+216+108)

#define	REGOFF(x)	(UREG(&((struct Ureg *) 0)->x))

#define	VO	REGOFF(vo)		/* vo, 2 bytes */
#define	SR	REGOFF(sr)		/* sr, 2 bytes */

#define	R0	REGOFF(r0)
#define	PC	REGOFF(pc)
#define	DBMAGIC	REGOFF(magic)
#define	SP	REGOFF(usp)

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
	MINREG,		/* minimum register */
	MAXREG,		/* maximum register */
	"PC",
	"A7",
	0,		/* link register */
	R0,		/* return register */
	0x2000,		/* page size */
	0x80000000,	/* kernel base */
	0,		/* kernel text mask */
	0,		/* offset of ksp in /proc/proc */
	4,		/* correction to ksp value */
	4,		/* offset of kpc in /proc/proc */
	0,		/* correction to kpc value */
	SCALLOFF,	/* offset in ublk of syscall #*/
	2,		/* quantization of pc */
	"a6base",	/* static base register name */
	0,		/* value */
	4,		/* szaddr */
	4,		/* szreg */
	4,		/* szfloat */
	8,		/* szdouble */
};
