/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * EPISODE 12B
 * How to recognise different types of trees from quite a long way away.
 * NO. 1
 * THE LARCH
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

int
incref(Ref *r)
{
	int x;

	lock(r);
	x = ++r->ref;
	unlock(r);
	return x;
}

int
decref(Ref *r)
{
	int x;

	lock(r);
	x = --r->ref;
	unlock(r);
	if(x < 0)
		panic("decref pc=%#p", getcallerpc(&r));

	return x;
}

void
procrestore(Proc *p)
{
	uint64_t t;

	if(p->kp)
		return;
	cycles(&t);
	p->pcycles -= t;

	fpuprocrestore(p);
}

/*
 *  Save the mach dependent part of the process state.
 *  NB: the caller should mmuflushtlb after procsave().
 *  procsave/procrestore don't touch the mmu, they
 *  care about fpu, mostly.
 */
void
procsave(Proc *p)
{
	uint64_t t;

	cycles(&t);
	p->pcycles += t;

	fpuprocsave(p);
}

static void
linkproc(void)
{
	spllo();
	up->kpfun(up->kparg);
	pexit("kproc dying", 0);
}

void
kprocchild(Proc* p, void (*func)(void*), void* arg)
{
	/*
	 * gotolabel() needs a word on the stack in
	 * which to place the return PC used to jump
	 * to linkproc().
	 */
	p->sched.pc = PTR2UINT(linkproc);
	p->sched.sp = PTR2UINT(p->kstack+KSTACK-BY2SE);
	p->sched.sp = STACKALIGN(p->sched.sp);

	p->kpfun = func;
	p->kparg = arg;
}

/*
 *  put the processor in the halt state if we've no processes to run.
 *  an interrupt will get us going again.
 *  The boot TC in nix can't halt, because it must stay alert in
 *  case an AC makes a handler process ready.
 *  We should probably use mwait in that case.
 */
void
idlehands(void)
{
if(0)
	if(m->machno != 0)
		halt();
}
