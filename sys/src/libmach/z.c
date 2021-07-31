/*
 * hobbit definition
 */
#include <u.h>
#include <bio.h>
#include "/hobbit/include/ureg.h"
#include <mach.h>
/*
 * Crisp register stuff
 */
#define USIZE		0x1000
#define USER_REG(x)	(USIZE-0X20-0x20+(ulong)(x))
#define	SCALLOFF	(0x0000+4+4)

#define	REGOFF(x)	(USER_REG(&((struct Ureg *) 0)->x))

#define PSW	REGOFF(psw)
#define PC	REGOFF(pc)
#define MSP	REGOFF(msp)
#define SP	REGOFF(sp)
#define FAULT	REGOFF(fault)
#define ID	REGOFF(id)
#define USER	REGOFF(user)
#define CAUSE	REGOFF(cause)

#define	MAXREG	0
#define	MINREG	0

struct reglist hobbitreglist[] = {
	{"PSW",		PSW,	0, 'X'},
	{"PC",		PC,	0, 'X'},
	{"MSP",		MSP,	0, 'X'},
	{"SP",		SP,	0, 'X'},
	{"FAULT",	FAULT,  0, 'X'},
	{"ID",		ID,	0, 'X'},
	{"USER",	USER,	0, 'X'},
	{"CAUSE",	CAUSE,  0, 'X'},
	{0}
};

Mach mhobbit =
{
	"hobbit",
	hobbitreglist,		/* hobbit registers */
	MINREG,			/* minimum register */
	MAXREG,			/* maximum register */
	PC,
	SP,
	0,			/* no link */
	0,			/* no return reg */
	0,			/* first writable register */
	0x1000,			/* page size */
	0x80000000,		/* kernel base */
	0,			/* kernel text mask */
	4,			/* offset of ksp in /proc/proc */
	0,			/* correction to ksp value */
	12,			/* offset of kpc in /proc/proc */
	0,			/* correction to kpc value */
	SCALLOFF,		/* offset in ublk to sys call # */
	2,			/* quantization of pc */
	0,			/* static base register name */
	0,			/* value */
	4,			/* szaddr */
	4,			/* szreg */
	4,			/* szfloat */
	8,			/* szdouble */
};
