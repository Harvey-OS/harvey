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
 * called in syscallfmt.c, sysfile.c, sysproc.c
 */
void
validalign(uintptr addr, unsigned align)
{
	/*
	 * Plan 9 is a 32-bit O/S, and the hardware it runs on
	 * does not usually have instructions which move 64-bit
	 * quantities directly, synthesizing the operations
	 * with 32-bit move instructions. Therefore, the compiler
	 * (and hardware) usually only enforce 32-bit alignment,
	 * if at all.
	 *
	 * Take this out if the architecture warrants it.
	 */
	if(align == sizeof(vlong))
		align = sizeof(long);

	/*
	 * Check align is a power of 2, then addr alignment.
	 */
	if((align != 0 && !(align & (align-1))) && !(addr & (align-1)))
		return;
	postnote(up, 1, "sys: odd address", NDebug);
	error(Ebadarg);
	/*NOTREACHED*/
}

/* go to user space */
void
kexit(Ureg*)
{
	uvlong t;
	Tos *tos;

	/* precise time accounting, kernel exit */
	tos = (Tos*)(USTKTOP-sizeof(Tos));
	cycles(&t);
	tos->kcycles += t - up->kentry;
	tos->pcycles = up->pcycles;
	tos->cyclefreq = m->cpuhz;
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

// TODO: save and restore VFPv3 FP state once 5[cal] know the new registers.
	fpuprocsave(p);
	/*
	 * Prevent the following scenario:
	 *	pX sleeps on cpuA, leaving its page tables in mmul1
	 *	pX wakes up on cpuB, and exits, freeing its page tables
	 *  pY on cpuB allocates a freed page table page and overwrites with data
	 *  cpuA takes an interrupt, and is now running with bad page tables
	 * In theory this shouldn't hurt because only user address space tables
	 * are affected, and mmuswitch will clear mmul1 before a user process is
	 * dispatched.  But empirically it correlates with weird problems, eg
	 * resetting of the core clock at 0x4000001C which confuses local timers.
	 */
	if(conf.nmach > 1)
		mmuswitch(nil);
}

void
procrestore(Proc* p)
{
	uvlong t;

	if(p->kp)
		return;
	cycles(&t);
	p->pcycles -= t;

	fpuprocrestore(p);
}

int
userureg(Ureg* ureg)
{
	return (ureg->psr & PsrMask) == PsrMusr;
}
