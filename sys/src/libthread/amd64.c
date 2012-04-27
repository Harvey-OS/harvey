#include <u.h>
#include <libc.h>
#include <thread.h>
#include "threadimpl.h"

/* first argument goes in a register; simplest just to ignore it */
static void
launcheramd64(int, void (*f)(void *arg), void *arg)
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
	*--tos = 0;	/* first arg to launcheramd64 */
	t->sched[JMPBUFPC] = (uvlong)launcheramd64+JMPBUFDPC;
	t->sched[JMPBUFSP] = (uvlong)tos - 2*8;		/* old PC and new PC */
}

