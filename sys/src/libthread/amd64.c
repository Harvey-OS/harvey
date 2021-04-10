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
	u64 *tos = (u64*)&t->stk[t->stksize&~0x0F];

	t->sched[JMPBUFPC] = (u64)launcheramd64+JMPBUFDPC;
	// Ensure we'll still be aligned to 16 bytes after return address is
	// pushed onto the stack.  Important because otherwise we'll get a GP
	// exception if we try to use some SIMD instructions with a stack that's
	// not aligned to 16 bytes.
	t->sched[JMPBUFSP] = (u64)tos - 8;
	t->sched[JMPBUFARG1] = (u64)f;
	t->sched[JMPBUFARG2] = (u64)arg;
}
