/*
 * code to keep track of registers
 */

#include "defs.h"
#include "fns.h"

/*
 * translate a name to a magic register offset
 */
Reglist*
rname(char *name)
{
	Reglist *rp;

	for (rp = mach->reglist; rp->rname; rp++)
		if (strcmp(name, rp->rname) == 0)
			return rp;
	return 0;
}

static ulong
getreg(Map *map, Reglist *rp)
{

	long l;
	int ret;

	l = 0;
	ret = 0;
	switch (rp->rformat)
	{
	case 'x':
		ret = get2(map, rp->raddr, (ushort*) &l);
		break;
	case 'X':
	case 'f':
	case 'F':
		ret = get4(map, rp->raddr, &l);
		break;
	default:
		werrstr("can't retrieve register %s", rp->rname);
		error("%r");
	}
	if (ret < 0) {
		werrstr("Register %s: %r", rp->rname);
		error("%r");
	}
	l += rp->rdelta;
	return l;
}

ulong
rget(Map *map, char *name)
{
	Reglist *rp;

	rp = rname(name);
	if (!rp)
		error("invalid register name");
	return getreg(map, rp);
}

void
rput(Map *map, char *name, ulong v)
{
	Reglist *rp;
	int ret;

	rp = rname(name);
	if (!rp)
		error("invalid register name");
	if (rp->rflags & RRDONLY)
		error("register is read-only");
	switch (rp->rformat)
	{
	case 'x':
		ret = put2(map, rp->raddr, (ushort) v);
		break;
	case 'X':
	case 'f':
	case 'F':
		ret = put4(map, rp->raddr, (long) v);
		break;
	default:
		ret = -1;
	}
	if (ret < 0)
		error("can't write register");
}

void
fixregs(Map *map)
{
	Reglist *rp;
	long val;

	if (machdata->ufixup) {
		if (machdata->ufixup(map, &val) < 0)
			error("can't fixup register addresses: %r");
	} else
		val = 0;
	for (rp = mach->reglist; rp->rname; rp++)
		rp->raddr = mach->kbase+rp->roffs-val;
}

void
adjustreg(char *name, ulong raddr, long rdelta)
{
	Reglist *rp;

	rp = rname(name);
	if (!rp)
		error("invalid register name");
	rp->raddr=raddr;
	rp->rdelta=rdelta;
}
	
/*
 * print the registers
 */
void
printregs(int c)
{
	Reglist *rp;
	int i;

	for (i = 1, rp = mach->reglist; rp->rname; rp++, i++) {
		if ((rp->rflags & RFLT)) {
			if (c != 'R')
				continue;
			if (rp->rformat == '8' || rp->rformat == '3')
				continue;
		}
		dprint("%-8s %-12lux", rp->rname, getreg(cormap, rp));
		if ((i % 3) == 0) {
			dprint("\n");
			i = 0;
		}
	}
	if (i != 1)
		dprint("\n");
	printsyscall();
	dprint ("%s\n", machdata->excep(cormap, rget));
	printpc();
}
