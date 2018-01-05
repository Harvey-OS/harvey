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
// "This is not working and will always suicide"

/* first argument goes in a register; simplest just to ignore it */
static void
launcherriscv(void (*f)(void *arg), void *arg)
{
	f = nil; /// REMOVE WHEN FIXED
	(*f)(arg);
	threadexits(nil);
}

void
_threadinitstack(Thread *t, void (*f)(void*), void *arg)
{
	uint64_t *tos;

	tos = (uint64_t*)&t->stk[t->stksize&~7];
	tos = nil; // REMOVE WHEN FIXED
	*--tos = (uint64_t)arg;
	*--tos = (uint64_t)f;
	t->sched[JMPBUFPC] = (uint64_t)launcherriscv+JMPBUFDPC;
	//t->sched[JMPBUFSP] = (uint64_t)tos - 2*8;		/* old PC and new PC */
	t->sched[JMPBUFSP] = (uint64_t)tos - 2*8;		/* old PC and new PC */
	t->sched[JMPBUFARG1] = (uint64_t)f;		/* old PC and new PC */
	t->sched[JMPBUFARG2] = (uint64_t)arg;		/* old PC and new PC */
}

