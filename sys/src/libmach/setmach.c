/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<mach.h>
		/* table for selecting machine-dependent parameters */

typedef	struct machtab Machtab;

struct machtab
{
	char		*name;			/* machine name */
	int16_t		type;			/* executable type */
	int16_t		boottype;		/* bootable type */
	int		asstype;		/* disassembler code */
	Mach		*mach;			/* machine description */
	Machdata	*machdata;		/* machine functions */
};

extern	Mach		mmips, msparc, m68020, mi386, mamd64,
			marm, mmips2be, mmips2le, mpower, mpower64, malpha, msparc64;
extern	Machdata	mipsmach, mipsmachle, sparcmach, m68020mach, i386mach,
			armmach, mipsmach2le, powermach, alphamach, sparc64mach;

/*
 *	machine selection table.  machines with native disassemblers should
 *	follow the plan 9 variant in the table; native modes are selectable
 *	only by name.
 */
Machtab	machines[] =
{
	{	"amd64",			/*amd64*/
		FAMD64,
		FAMD64B,
		AAMD64,
		&mamd64,
		&i386mach,	},
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
