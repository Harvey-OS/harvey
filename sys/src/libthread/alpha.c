#include <u.h>
#include <libc.h>
#include <thread.h>
#include "threadimpl.h"

/* first argument goes in a register and on the stack; avoid it */
static void
launcheralpha(int, void (*f)(void *arg), void *arg)
{
	(*f)(arg);
	threadexits(nil);
}

void
_threadinitstack(Thread *t, void (*f)(void*), void *arg)
{
	ulong *tos;

	tos = (ulong*)&t->stk[t->stksize&~7];

	*--tos = 0;		/* pad arguments to 8 bytes */
	*--tos = (ulong)arg;
	*--tos = (ulong)f;
	*--tos = 0;		/* first arg */

	*--tos = 0;		/* for alignment with... */
	*--tos = 0;		/* ... place to store return PC */

	t->sched[JMPBUFPC] = (ulong)launcheralpha+JMPBUFDPC;
	t->sched[JMPBUFSP] = (ulong)tos;
}

