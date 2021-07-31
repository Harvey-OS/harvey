#include	<alef.h>
#include	"run.h"

extern Ref	ALEFnproc;

/*
 * Start a new process, build an argument area in the new stack, copy
 * accross the parameters. linksp clears the parents spin location after
 * stack transfer.  The macros ALIGN and ALIGN_UP are defined in run.h
 * and compile out for machines that don't require strict stack alignment.
 */
void
ALEF_proc(void (*f)(void), int argsize, ...)
{
	Task *t;
	Tdb *tdb;
	int spin;
	byte *stack;

	stack = ALEFalloc(ALEFstack+sizeof(Task)+sizeof(Tdb), 0);
	if(stack == nil)
		abort();

	memset(stack, 0, sizeof(Task)+sizeof(Tdb));
	t = (Task*)stack;
	t->stack = stack;
	stack += sizeof(Task);

	tdb = (Tdb*)stack;
	stack += sizeof(Tdb);
	tdb->ntask = 1;
	tdb->runhd = nil;
	tdb->ctask = t;
	t->tdb = tdb;

	if(MAXBECOME > argsize)
		argsize = MAXBECOME;

	stack = (byte*)ALIGN(stack, 8);		/* see ../$objtype/run.h */
	argsize = ALIGN_UP(argsize, 8);		/* see ../$objtype/run.h */

	stack = stack+ALEFstack-(argsize+sizeof(byte*));
	memmove(stack, ..., argsize);

	ALEFnproc.inc();

	spin = 1;
	if(rfork(ALEFrfflag) == 0) {
		PROC.tdb = tdb;
		ALEF_linksp(&spin, stack, f);
	}

	while(spin)
		;
}
