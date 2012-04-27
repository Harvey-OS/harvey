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
	ulong *tos;

	tos = (ulong*)&t->stk[t->stksize&~7];
	*--tos = (ulong)arg;
	*--tos = (ulong)f;
	t->sched[JMPBUFPC] = (ulong)launcher386+JMPBUFDPC;
	t->sched[JMPBUFSP] = (ulong)tos - 8;		/* old PC and new PC */
}

