#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include <tos.h>
#include "ureg.h"

#include "arm.h"

/*
 * A lot of this stuff doesn't belong here
 * but this is a convenient dumping ground for
 * later sorting into the appropriate buckets.
 */

/* Give enough context in the ureg to produce a kernel stack for
 * a sleeping process
 */
void
setkernur(Ureg* ureg, Proc* p)
{
	ureg->pc = p->sched.pc;
	ureg->sp = p->sched.sp+4;
	ureg->r14 = PTR2UINT(sched);
}

/*
 * called in sysfile.c
 */
void
evenaddr(uintptr addr)
{
	if(addr & 3){
		postnote(up, 1, "sys: odd address", NDebug);
		error(Ebadarg);
	}
}

/* go to user space */
void
kexit(Ureg*)
{
	uvlong t;
	Tos *tos;

	/* precise time accounting, kernel exit */
	tos = (Tos*)(USTKTOP-sizeof(Tos));
	t = fastticks(nil);
	tos->kcycles += t - up->kentry;
	tos->pcycles = up->pcycles;
	tos->cyclefreq = Frequency;
	tos->pid = up->pid;

	/* make visible immediately to user proc */
	cachedwbinvse(tos, sizeof *tos);
}

/*
 *  return the userpc the last exception happened at
 */
uintptr
userpc(void)
{
	Ureg *ureg = up->dbgreg;
	return ureg->pc;
}

/* This routine must save the values of registers the user is not permitted
 * to write from devproc and then restore the saved values before returning.
 */
void
setregisters(Ureg* ureg, char* pureg, char* uva, int n)
{
	USED(ureg, pureg, uva, n);
}

/*
 *  this is the body for all kproc's
 */
static void
linkproc(void)
{
	spllo();
	up->kpfun(up->kparg);
	pexit("kproc exiting", 0);
}

/*
 *  setup stack and initial PC for a new kernel proc.  This is architecture
 *  dependent because of the starting stack location
 */
void
kprocchild(Proc *p, void (*func)(void*), void *arg)
{
	p->sched.pc = PTR2UINT(linkproc);
	p->sched.sp = PTR2UINT(p->kstack+KSTACK);

	p->kpfun = func;
	p->kparg = arg;
}

/*
 *  pc output by dumpaproc
 */
uintptr
dbgpc(Proc* p)
{
	Ureg *ureg;

	ureg = p->dbgreg;
	if(ureg == 0)
		return 0;

	return ureg->pc;
}

/*
 *  set mach dependent process state for a new process
 */
void
procsetup(Proc* p)
{
	fpusysprocsetup(p);
}

/*
 *  Save the mach dependent part of the process state.
 */
void
procsave(Proc* p)
{
	uvlong t;

	cycles(&t);
	p->pcycles += t;

	fpuprocsave(p);
}

void
procrestore(Proc* p)
{
	uvlong t;

	if(p->kp)
		return;
	t = lcycles();
	p->pcycles -= t;

	fpuprocrestore(p);
}

int
userureg(Ureg* ureg)
{
	return (ureg->psr & PsrMask) == PsrMusr;
}

/*
 * atomic ops
 * make sure that we don't drag in the C library versions
 */

long
_xdec(long *p)
{
	int s, v;

	s = splhi();
	v = --*p;
	splx(s);
	return v;
}

void
_xinc(long *p)
{
	int s;

	s = splhi();
	++*p;
	splx(s);
}

int
ainc(int *p)
{
	int s, v;

	s = splhi();
	v = ++*p;
	splx(s);
	return v;
}

int
adec(int *p)
{
	int s, v;

	s = splhi();
	v = --*p;
	splx(s);
	return v;
}

int
cas32(void* addr, u32int old, u32int new)
{
	int r, s;

	s = splhi();
	if(r = (*(u32int*)addr == old))
		*(u32int*)addr = new;
	splx(s);
	if (r)
		coherence();
	return r;
}
