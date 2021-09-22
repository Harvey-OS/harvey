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
#include "riscv.h"

#ifndef incref
int
incref(Ref *r)
{
	int x;

	lock(r);
	x = ++r->ref;
	unlock(r);
	return x;
}
#endif /* incref */

#ifndef decref
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
#endif /* decref */

void
procrestore(Proc *p)
{
	uvlong t;

	if(p->kp)
		return;
	cycles(&t);
	p->pcycles -= t;

	fpuprocrestore(p);
	wbinvd();
//	idlewake();			/* other runnable procs? */
}

/*
 *  Save the mach dependent part of the process state.
 */
void
procsave(Proc *p)
{
	uvlong t;

	cycles(&t);
	p->pcycles += t;

	fpuprocsave(p);

	/*
	 * flush mmu contents so another proc on this cpu can start fresh.
	 */
	mmuflushtlb(m->ptroot);
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
	p->sched.sp = PTR2UINT(p->kstack+KSTACK-sizeof(uintptr));
	p->sched.sp = STACKALIGN(p->sched.sp);

	p->kpfun = func;
	p->kparg = arg;
}

void
evenaddr(uintptr addr)
{
	int align;

	/*
	 * The hardware Plan 9 runs on
	 * does not usually have instructions which move 64-bit
	 * quantities directly, synthesizing the operations
	 * with 32-bit move instructions. Therefore, the compiler
	 * (and hardware) usually only enforce 32-bit alignment,
	 * if at all.
	 */
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

