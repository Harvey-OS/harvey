#include "defs.h"
#include "fns.h"

extern	Mach	mmips;
extern	Mach	msparc;
extern	Mach	m68020;
extern	Mach	mi386;
extern	Mach	mi960;
extern	Mach	mhobbit;

extern	Machdata	mipsmach;
extern	Machdata	sparcmach;
extern	Machdata	m68020mach;
extern	Machdata	i386mach;
extern	Machdata	i960mach;
extern	Machdata	hobbitmach;

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
	{	"hobbit",			/*hobbit*/
		FHOBBIT,
		FHOBBITB,
		AHOBBIT,
		&mhobbit,
		&hobbitmach,	},
	{	0		},		/*the terminator*/
};

int 	asstype = AMIPS;			/*disassembler type*/
Machdata *machdata = &mipsmach;			/*machine type*/
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
/*
 * return an address for a local variable
 * no register vars, unfortunately, as we can't provide an address
 * fn is the procedure; var the local name
 */

void
localaddr(char *fn, char *var)
{
	Symbol s;
	ADDR fp;
	extern ADDR expv;
	extern int expsp;

	if (fn) {
		if (lookup(fn, 0, &s) == 0)
			error("function not found");
	}
	else {
		if (findsym(rget(mach->pc), CTEXT, &s) == 0)
			error("function not found");
	}
	if ((fp = machdata->findframe(s.value)) == 0)
		error("stack frame not found");
	if (var == 0) {
		expsp = SEGNONE;
		expv = fp;
		return;
	}
	if (findlocal(&s, var, &s) == 0)
		error("bad local variable");
	switch (s.class) {
	case CAUTO:
		expv = fp - s.value;
		break;
	case CPARAM:		/* assume address size is stack width */
		expv = fp + s.value + mach->szaddr;
		break;
	default:
		error("bad local symbol");
	}
	expsp = SEGNONE;
}
/*
 * print a stack traceback
 * give locals if argument == 'C'
 */
#define	EVEN(x)	((x)&~1)

void
ctrace(int modif)
{
	long moved, sp, j;
	ulong pc;
	Symbol s;
	int found;

	if (adrflg) {
		if (get4(cormap, EVEN(adrval), SEGDATA, (long *) &sp) == 0) {
			dprint("%s\n", errflg);
			return;
		}
		if (get4(cormap, EVEN(adrval+4), SEGDATA, (long *) &pc) == 0) {
			dprint("%s\n", errflg);
			return;
		}
	} else {
		sp = EVEN(rget(mach->sp));
		pc = rget(mach->pc);
	}
	j = 0;
	while (pc) {
		if ((moved = pc2sp(pc)) == -1)
			break;
		found = findsym(pc, CTEXT, &s);
		if (!found)
			dprint("?(");
		else
			dprint("%s(", s.name);
		sp += moved;
		get4(cormap, sp, SEGDATA, (long *)&pc);
		if (found)
			printparams(&s, sp);
		dprint(") ");
		if(found)
			printsource(s.value);
		dprint(" called from ");
		psymoff(pc, SEGTEXT, " ");
		printsource(pc);
		dprint("\n");
		if (modif == 'C' && found)
			printlocals(&s, sp);
		sp += mach->szaddr;	/*assumes address size = stack width*/
		if(++j > 40){
			dprint("(trace truncated)\n");
			break;
		}
	}
}

ulong
findframe(ulong addr)
{
	ulong sp, pc;
	int moved;
	Symbol s;

	sp = EVEN(rget(mach->sp));
	pc = rget(mach->pc);
	while (!errflg) {
		if ((moved = -pc2sp(pc)) == 1)
			return sp;
		sp -= moved;
		findsym(pc, CTEXT, &s);
		if (addr == s.value)
			return sp;
		get4(cormap, sp, SEGDATA, (long *) &pc);
		sp += mach->szaddr;	/*assumes sizeof(addr) = stack width*/
	}
	return 0;
}
