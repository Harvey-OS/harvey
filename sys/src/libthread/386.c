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
launcher386(void (*f)(void *arg), void *arg)
{
	(*f)(arg);
	threadexits(nil);
}

void
_threadinitstack(Thread *t, void (*f)(void*), void *arg)
{
	uint32_t *tos;

	tos = (uint32_t*)&t->stk[t->stksize&~7];
	*--tos = (uint32_t)arg;
	*--tos = (uint32_t)f;
	t->sched[JMPBUFPC] = (uint32_t)launcher386+JMPBUFDPC;
	t->sched[JMPBUFSP] = (uint32_t)tos - 8;		/* old PC and new PC */
}
