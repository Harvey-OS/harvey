#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<mach.h>
		/* table for selecting machine-dependent parameters */

typedef	struct machtab Machtab;

struct machtab
{
	char		*name;			/* machine name */
	short		type;			/* executable type */
	short		boottype;		/* bootable type */
	int		asstype;		/* disassembler code */
	Mach		*mach;			/* machine description */
	Machdata	*machdata;		/* machine functions */
};

extern	Mach		mmips, msparc, m68020, mi386, mi960;
extern	Machdata	mipsmach, sparcmach, m68020mach, i386mach, i960mach;

/*
 *	machine selection table.  machines with native disassemblers should
 *	follow the plan 9 variant in the table; native modes are selectable
 *	only by name.
 */
Machtab	machines[] =
{
	{	"68020",			/*68020*/
		F68020,
		F68020B,
		A68020,
		&m68020,
		&m68020mach,	},
	{	"68020",			/*Next 68040 bootable*/
		F68020,
		FNEXTB,
		A68020,
		&m68020,
		&m68020mach,	},
	{	"mips",				/*plan 9 mips*/
		FMIPS,
		FMIPSB,
		AMIPS,
		&mmips,
		&mipsmach, 	},
	{	"mipsco",			/*native mips - must follow plan 9*/
		FMIPS,
		FMIPSB,
		AMIPSCO,
		&mmips,
		&mipsmach,	},
	{	"sparc",			/*plan 9 sparc */
		FSPARC,
		FSPARCB,
		ASPARC,
		&msparc,
		&sparcmach,	},
	{	"sunsparc",			/*native sparc - must follow plan 9*/
		FSPARC,
		FSPARCB,
		ASUNSPARC,
		&msparc,
		&sparcmach,	},
	{	"386",				/*plan 9 386*/
		FI386,
		FI386B,
		AI386,
		&mi386,
		&i386mach,	},
	{	"86",				/*8086 - a peach of a machine*/
		FI386,
		FI386B,
		AI8086,
		&mi386,
		&i386mach,	},
	{	"960",				/*i960*/
		FI960,
		FI960B,
		AI960,
		&mi960,
		&i960mach,	},
	{	0		},		/*the terminator*/
};

/*
 *	select a machine by executable file type
 */
void
machbytype(int type)
{
	Machtab *mp;

	for (mp = machines; mp->name; mp++){
		if (mp->type == type || mp->boottype == type) {
			asstype = mp->asstype;
			machdata = mp->machdata;
			break;
		}
	}
}
/*
 *	select a machine by name
 */
int
machbyname(char *name)
{
	Machtab *mp;

	if (!name) {
		asstype = AMIPS;
		machdata = &mipsmach;
		mach = &mmips;
		return 1;
	}
	for (mp = machines; mp->name; mp++){
		if (strcmp(mp->name, name) == 0) {
			asstype = mp->asstype;
			machdata = mp->machdata;
			mach = mp->mach;
			return 1;
		}
	}
	return 0;
}
