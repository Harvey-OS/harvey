#include <u.h>
#include <libc.h>
#include <thread.h>
#include "threadimpl.h"

/* first argument goes in a register; simplest just to ignore it */
static void
launcherriscv(int, void (*f)(void *arg), void *arg)
{
	if (f == nil)
		sysfatal("launcherriscv: nil f passed: arg %#p", arg);
	(*f)(arg);
	threadexits(nil);
}

void
_threadinitstack(Thread *t, void (*f)(void*), void *arg)
{
	uintptr *tos;

	tos = (uintptr *)&t->stk[t->stksize&~7];
	*--tos = (uintptr)arg;
	*--tos = (uintptr)f;
	t->sched[JMPBUFPC] = (uintptr)launcherriscv+JMPBUFDPC;
	t->sched[JMPBUFSP] = (uintptr)tos - 2*sizeof(uintptr); /* 1st arg, return PC */
}
