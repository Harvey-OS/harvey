/*
 * code to keep track of registers
 */

#include "defs.h"
#include "fns.h"

int	regdirty;

/*
 * get/put registers
 * in our saved copies
 */
ulong
rget(int r)
{
	Reglist *rp;

	for (rp = mach->reglist; rp->rname; rp++) {
		if (rp->roffs == r)
			return (rp->rval);
	}
	dprint("rget: can't find reg %d\n", r);
	error("panic: rget");
	return 1;		/* to shut compiler up */
	/* NOTREACHED */
}

void
rput(int r, ulong v)
{
	Reglist *rp;

	for (rp = mach->reglist; rp->rname; rp++) {
		if (rp->roffs == r) {
			regdirty = 1;
			rp->rval = v;
			return;
		}
	}
	dprint("rput: can't find reg %d\n", r);
	error("panic: rput");
	/* NOTREACHED */
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
		if (rp->rfloat == 1 && c != 'R')
			continue;
		dprint("%-8s %-12lux", rp->rname, rp->rval);
		if ((i % 3) == 0) {
			dprint("\n");
			i = 0;
		}
	}
	if (i != 1)
		dprint("\n");
	printsyscall();
	machdata->excep();
	printpc();
}

/*
 * translate a name to a magic register offset
 * the latter useful in rget/rput
 */
int
rname(char *n)
{
	Reglist *rp;

	for (rp = mach->reglist; rp->rname; rp++)
		if (strcmp(n, rp->rname) == 0)
			return (rp->roffs);
	return (BADREG);
}

/*
 * Common versions for machines with 4-byte registers.
 * Should be called before looking at the process.
 */

void
rsnarf4(Reglist *rp)
{
	for (; rp->rname; rp++)
		get4(cormap, rp->roffs, SEGREGS, (long *)&rp->rval);
	regdirty = 0;
}

void
rrest4(Reglist *rp)
{
	if (pid == 0 || !regdirty)
		return;
	for (; rp->rname; rp++){
		if(put4(cormap, rp->roffs, SEGREGS, rp->rval) == 0){
			char buf[ERRLEN];
			errstr(buf);
			dprint("can't write %s: %s\n", rp->rname, buf);
		}
	}
	regdirty = 0;
}
