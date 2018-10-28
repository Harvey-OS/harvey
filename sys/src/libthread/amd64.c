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

static void
launcheramd64(void (*f)(void *arg), void *arg)
{
	(*f)(arg);
	threadexits(nil);
}

void
_threadinitstack(Thread *t, void (*f)(void*), void *arg)
{
	// Ensure tos is aligned to 16 bytes
	uint64_t *tos = (uint64_t*)&t->stk[t->stksize&~0x0F];

	t->sched[JMPBUFPC] = (uint64_t)launcheramd64+JMPBUFDPC;
	// Ensure we'll still be aligned to 16 bytes after return address is
	// pushed onto the stack.  Important because otherwise we'll get a GP
	// exception if we try to use some SIMD instructions with a stack that's
	// not aligned to 16 bytes.
	t->sched[JMPBUFSP] = (uint64_t)tos - 8;
	t->sched[JMPBUFARG1] = (uint64_t)f;
	t->sched[JMPBUFARG2] = (uint64_t)arg;
}
