#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

/*
 * Find a way to finesse this away if not needed.
 */
int
fpudevprocio(Proc* proc, void* a, long n, uintptr offset, int write)
{
	USED(proc, a, n, offset, write);
	return 0;
}

void
fpusysrfork(Ureg*)
{
}

void
fpusysrforkchild(Proc*, Ureg*, Proc*)
{
}

void
fputrap(Ureg*, int)
{
}

int
fpuemu(Ureg *ur)
{
	int nfp;

	if(waserror()){
		postnote(up, 1, up->errstr, NDebug);
		return 1;
	}
	spllo();
	nfp = fpipower(ur);
	splhi();
	poperror();
	return nfp;
}

void
fpuinit(void)
{
}

int
fpuavail(Ureg*)
{
	return 0;
}

void
fpunotify(Ureg*)
{
}

void
fpunoted(void)
{
}

void
fpusysprocsetup(Proc *p)
{
	p->fpstate = FPinit;
//	fpoff();		/* add to l.s */
}
