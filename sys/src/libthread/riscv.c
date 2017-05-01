/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include "threadimpl.h"
/* first argument goes in a register; simplest just to ignore it */
static void
launcherriscv(uint64_t _ /* return from longjmp */, uint64_t __ /* ignored a1 */,  void (*f)(void *arg), void *arg)
{
	print("launch riscv %p(%p)\n", f, arg);
	(*f)(arg);
	threadexits(nil);
}

void
_threadinitstack(Thread *t, void (*f)(void*), void *arg)
{
	uint64_t *tos;

	tos = (uint64_t*)&t->stk[t->stksize&~7];
	print("TOS: %p\n", tos);
	print("_threadinitstack: thread %p f %p arg %p\n", t, f, arg);
	t->sched[JMPBUFPC] = (uint64_t)launcherriscv+JMPBUFDPC;
	t->sched[JMPBUFSP] = (uint64_t)tos;
	t->sched[JMPBUFARG3] = (uint64_t)f;
	t->sched[JMPBUFARG4] = (uint64_t)arg;
}
