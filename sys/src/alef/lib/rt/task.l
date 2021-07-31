#include	<alef.h>
#include	"run.h"
/*
 *	There are all sorts of achitecture-dependent fudge factors to
 *	properly align the stack and position the PC.  These values
 *	and macros are defined in run.h.  Notice that ALIGN and ALIGN_UP
 *	compile out for most architectures.
 */
void
ALEF_task(void (*f)(void), int argsize, ...)
{
	Task *t;
	Tdb *tdb;
	byte *stack;

	stack = ALEFalloc(ALEFstack+sizeof(Task), 0);
	if(stack == nil)
		abort();

	memset(stack, 0, sizeof(Task));
	t = (Task*)stack;
	t->stack = stack;

	tdb = PROC.tdb;
	tdb->lock();
	tdb->ntask++;
	if(tdb->runhd)
		tdb->runtl->link = t;
	else
		tdb->runhd = t;
	tdb->runtl = t;
	t->tdb = tdb;
	tdb->unlock();

	if(MAXBECOME > argsize)
		argsize = MAXBECOME;

	stack += sizeof(Task);
	stack = (byte*)ALIGN(stack, 8);		/* see ../$objtype/run.h */
	argsize = ALIGN_UP(argsize, 8);		/* see ../$objtype/run.h */

	stack = stack+ALEFstack-(argsize+sizeof(byte*));
	memmove(stack, ..., argsize);

	t->sp = (uint)stack-4-SP_DELTA;		/* see ../$objtype/run.h */
	*(uint*)(t->sp+SP_DELTA) = (uint)f;	/* see ../$objtype/run.h */
	t->pc = (uint)ALEF_linktask - PC_DELTA;	/* see ../$objtype/run.h */
}
