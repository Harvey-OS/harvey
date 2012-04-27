#include <u.h>
#include <libc.h>
#include <thread.h>
#include "threadimpl.h"

/* first argument goes in a register; simplest just to ignore it */
static void
launcherarm(int, void (*f)(void *arg), void *arg)
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
	*--tos = 0;	/* first arg to launchermips */
	*--tos = 0;	/* place to store return PC */

	t->sched[JMPBUFPC] = (ulong)launcherarm+JMPBUFDPC;
	t->sched[JMPBUFSP] = (ulong)tos;
}

