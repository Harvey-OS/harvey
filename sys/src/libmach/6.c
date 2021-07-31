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
	{ "R0",		UR(0), 0, 'X'},
	{ "R1",		UR(1), 0, 'X'},
	{ "R2",		UR(2), 0, 'X'},
	{ "R3",		UR(3), 0, 'X'},
	{ "R4",		UR(4), 0, 'X'},
	{ "R5",		UR(5), 0, 'X'},
	{ "R6",		UR(6), 0, 'X'},
	{ "R7",		UR(7), 0, 'X'},
	{ "R8",		UR(8), 0, 'X'},
	{ "R9",		UR(9), 0, 'X'},
	{ "R10",	UR(10), 0, 'X'},
	{ "R11",	UR(11), 0, 'X'},
	{ "R12",	UR(12), 0, 'X'},
	{ "R13",	UR(13), 0, 'X'},
	{ "R14",	UR(14), 0, 'X'},
	{ "R15",	UR(15), 0, 'X'},
	{ "R16",	UR(16), 0, 'X'},
	{ "R17",	UR(17), 0, 'X'},
	{ "R18",	UR(18), 0, 'X'},
	{ "R19",	UR(19), 0, 'X'},
	{ "R20",	UR(20), 0, 'X'},
	{ "R21",	UR(21), 0, 'X'},
	{ "R22",	UR(22), 0, 'X'},
	{ "R23",	UR(23), 0, 'X'},
	{ "R24",	UR(24), 0, 'X'},
	{ "R25",	UR(25), 0, 'X'},
	{ "R26",	UR(26), 0, 'X'},
	{ "TMP",	UR(27), 0, 'X'},
	{ "SB",		UR(28), 0, 'X'},
	{ "SP",		SP, 0, 'X'},
	{ "PC",		PC, 0, 'X'},
	{ "R31",	UR(31), 0, 'X' },
	{  0 }
};

Mach mi960 =
{
	"960",
	i960reglist,		/* register list */
	MINREG,		/* minimum register */
	MAXREG,		/* maximum register */
	PC,
	SP,
	0,
	R4,		/* return register */
	0,		/* first writable register */
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
