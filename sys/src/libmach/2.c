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
#define	FREG(x)		(0x0000+4+216+(x))
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
	{"VO",		VO,		0,	'x'},
	{"SR",		SR,		0,	'x'},
	{"MAGIC",	DBMAGIC,	0,	'X'},
	{"PC",		PC,		0,	'X'},
	{"A7",		SP,		0,	'X'},
	{"KSP",		REGOFF(sp),	0,	'X'},
	{"A6",		REGOFF(a6),	0,	'X'},
	{"A5",		REGOFF(a5),	0,	'X'},
	{"A4",		REGOFF(a4),	0,	'X'},
	{"A3",		REGOFF(a3),	0,	'X'},
	{"A2",		REGOFF(a2),	0,	'X'},
	{"A1",		REGOFF(a1),	0,	'X'},
	{"A0",		REGOFF(a0),	0,	'X'},
	{"R7",		REGOFF(r7),	0,	'X'},
	{"R6",		REGOFF(r6),	0,	'X'},
	{"R5",		REGOFF(r5),	0,	'X'},
	{"R4",		REGOFF(r4),	0,	'X'},
	{"R3",		REGOFF(r3),	0,	'X'},
	{"R2",		REGOFF(r2),	0,	'X'},
	{"R1",		REGOFF(r1),	0,	'X'},
	{"R0",		REGOFF(r0),	0,	'X'},
	{"FPCR",	FREG(0*4),	1,	'X'},
	{"FPSR",	FREG(1*4),	1,	'X'},
	{"FPIAR",	FREG(2*4),	1,	'X'},
	{"F0H",		FREG(3*4),	1,	'8'},
	{"F0M",		FREG(4*4),	1,	'8'},
	{"F0L",		FREG(5*4),	1,	'8'},
	{"F1H",		FREG(6*4),	1,	'8'},
	{"F1M",		FREG(7*4),	1,	'8'},
	{"F1L",		FREG(8*4),	1,	'8'},
	{"F2H",		FREG(9*4),	1,	'8'},
	{"F2M",		FREG(10*4),	1,	'8'},
	{"F2L",		FREG(11*4),	1,	'8'},
	{"F3H",		FREG(12*4),	1,	'8'},
	{"F3M",		FREG(13*4),	1,	'8'},
	{"F3L",		FREG(14*4),	1,	'8'},
	{"F4H",		FREG(15*4),	1,	'8'},
	{"F4M",		FREG(16*4),	1,	'8'},
	{"F4L",		FREG(17*4),	1,	'8'},
	{"F5H",		FREG(18*4),	1,	'8'},
	{"F5M",		FREG(19*4),	1,	'8'},
	{"F5L",		FREG(20*4),	1,	'8'},
	{"F6H",		FREG(21*4),	1,	'8'},
	{"F6M",		FREG(22*4),	1,	'8'},
	{"F6L",		FREG(23*4),	1,	'8'},
	{"F7H",		FREG(24*4),	1,	'8'},
	{"F7M",		FREG(25*4),	1,	'8'},
	{"F7L",		FREG(26*4),	1,	'8'},
	{0}
};

Mach m68020 =
{
	"68020",
	m68020reglist,	/* register list */
	MINREG,		/* minimum register */
	MAXREG,		/* maximum register */
	PC,
	SP,
	0,		/* no link */
	R0,		/* return register */
	0,		/* first writable register */
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
