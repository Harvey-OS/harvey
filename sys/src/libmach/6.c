/*
 * 960 definition
 */
#include "u.h"
#include "mach.h"

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

Reglist i960reglist[] = {
	{ "R0",		UR(0), },
	{ "R1",		UR(1), },
	{ "R2",		UR(2), },
	{ "R3",		UR(3), },
	{ "R4",		UR(4), },
	{ "R5",		UR(5), },
	{ "R6",		UR(6), },
	{ "R7",		UR(7), },
	{ "R8",		UR(8), },
	{ "R9",		UR(9), },
	{ "R10",	UR(10), },
	{ "R11",	UR(11), },
	{ "R12",	UR(12), },
	{ "R13",	UR(13), },
	{ "R14",	UR(14), },
	{ "R15",	UR(15), },
	{ "R16",	UR(16), },
	{ "R17",	UR(17), },
	{ "R18",	UR(18), },
	{ "R19",	UR(19), },
	{ "R20",	UR(20), },
	{ "R21",	UR(21), },
	{ "R22",	UR(22), },
	{ "R23",	UR(23), },
	{ "R24",	UR(24), },
	{ "R25",	UR(25), },
	{ "R26",	UR(26), },
	{ "TMP",	UR(27), },
	{ "SB",		UR(28), },
	{ "SP",		SP, },
	{ "PC",		PC, },
	{ "R31",	UR(31), },
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
