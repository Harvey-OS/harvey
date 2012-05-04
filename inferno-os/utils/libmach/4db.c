#include <lib9.h>
#include <bio.h>
#include "mach.h"
/*
 * Mips-specific debugger interface
 */

char	*mipsexcep(Map*, Rgetter);
int	mipsfoll(Map*, ulong, Rgetter, ulong*);
int	mipsinst(Map*, ulong, char, char*, int);
int	mipsdas(Map*, ulong, char*, int);
int	mipsinstlen(Map*, ulong);
/*
 *	Debugger interface
 */
Machdata mipsmach2be =
{
	{0, 0, 0, 0xD},		/* break point */
	4,			/* break point size */

	beswab,			/* short to local byte order */
	beswal,			/* long to local byte order */
	beswav,			/* vlong to local byte order */
	risctrace,		/* C traceback */
	riscframe,		/* Frame finder */
	mipsexcep,		/* print exception */
	0,			/* breakpoint fixup */
	beieeesftos,		/* single precision float printer */
	beieeedftos,		/* double precisioin float printer */
	mipsfoll,		/* following addresses */
	mipsinst,		/* print instruction */
	mipsdas,		/* dissembler */
	mipsinstlen,		/* instruction size */
};

/*
 *	Debugger interface
 */
Machdata mipsmach2le =
{
	{0, 0, 0, 0xD},		/* break point */
	4,			/* break point size */

	leswab,			/* short to local byte order */
	leswal,			/* long to local byte order */
	leswav,			/* vlong to local byte order */
	risctrace,		/* C traceback */
	riscframe,		/* Frame finder */
	mipsexcep,		/* print exception */
	0,			/* breakpoint fixup */
	leieeesftos,		/* single precision float printer */
	leieeedftos,		/* double precisioin float printer */
	mipsfoll,		/* following addresses */
	mipsinst,		/* print instruction */
	mipsdas,		/* dissembler */
	mipsinstlen,		/* instruction size */
};
