#include <u.h>
#include <libc.h>
#include <thread.h>
#include "threadimpl.h"

/* first argument goes in a register; simplest just to ignore it */
static void
launcherspim64(int, void (*f)(void *arg), void *arg)
{
	(*f)(arg);
	threadexits(nil);
}

void
_threadinitstack(Thread *t, void (*f)(void*), void *arg)
{
	uvlong *tos;

	tos = (uvlong*)&t->stk[t->stksize&~7];
	*--tos = (uvlong)arg;
	*--tos = (uvlong)f;
	*--tos = 0;	/* first arg to launcherspim64 */
	*--tos = 0;	/* place to store return PC */
	t->sched[JMPBUFPC] = (uvlong)launcherspim64+JMPBUFDPC;
	t->sched[JMPBUFSP] = (uvlong)tos;
}
