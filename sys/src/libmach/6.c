/*
 * 960 definition
 */
#include <u.h>
#include <bio.h>
#include <mach.h>

#define	MINREG	0
#define	MAXREG	0

#define USIZE		0x1000
#define UR(x)		(USIZE-((32)*4)+4*(x))

/*
 *  until we decide how we'll take faults, we really
 *  don't know what the saved regs will look like.
 *  this is just a guess. -- presotto
 */
#define SP	UR(29)
#define PC	UR(30)
#define	R4	UR(4)

Reglist i960reglist[] = {
	{ "R0",		UR(0), RINT, 'X'},
	{ "R1",		UR(1), RINT, 'X'},
	{ "R2",		UR(2), RINT, 'X'},
	{ "R3",		UR(3), RINT, 'X'},
	{ "R4",		UR(4), RINT, 'X'},
	{ "R5",		UR(5), RINT, 'X'},
	{ "R6",		UR(6), RINT, 'X'},
	{ "R7",		UR(7), RINT, 'X'},
	{ "R8",		UR(8), RINT, 'X'},
	{ "R9",		UR(9), RINT, 'X'},
	{ "R10",	UR(10), RINT, 'X'},
	{ "R11",	UR(11), RINT, 'X'},
	{ "R12",	UR(12), RINT, 'X'},
	{ "R13",	UR(13), RINT, 'X'},
	{ "R14",	UR(14), RINT, 'X'},
	{ "R15",	UR(15), RINT, 'X'},
	{ "R16",	UR(16), RINT, 'X'},
	{ "R17",	UR(17), RINT, 'X'},
	{ "R18",	UR(18), RINT, 'X'},
	{ "R19",	UR(19), RINT, 'X'},
	{ "R20",	UR(20), RINT, 'X'},
	{ "R21",	UR(21), RINT, 'X'},
	{ "R22",	UR(22), RINT, 'X'},
	{ "R23",	UR(23), RINT, 'X'},
	{ "R24",	UR(24), RINT, 'X'},
	{ "R25",	UR(25), RINT, 'X'},
	{ "R26",	UR(26), RINT, 'X'},
	{ "TMP",	UR(27), RINT, 'X'},
	{ "SB",		UR(28), RINT, 'X'},
	{ "SP",		SP, RINT, 'X'},
	{ "PC",		PC, RINT, 'X'},
	{ "R31",	UR(31), RINT, 'X' },
	{  0 }
};

Mach mi960 =
{
	"960",
	MI960,		/* machine type */
	i960reglist,		/* register list */
	MINREG,		/* minimum register */
	MAXREG,		/* maximum register */
	"PC",
	"SP",
	0,		/* link register */
	R4,		/* return register */
	0x1000,		/* page size */
	0x80000000,	/* kernel base */
	0,		/* kernel text mask */
	0,		/* offset of ksp in /proc/proc */
	0,		/* correction to ksp value */
	4,		/* offset of kpc in /proc/proc */
	0,		/* correction to kpc value */
	0,		/* offset in ublk of syscall #*/
	1,		/* quantization of pc */
	"setSB",	/* static base register name */
	0,		/* value */
	4,		/* szaddr */
	4,		/* szreg */
	4,		/* szfloat */
	8,		/* szdouble */
};
