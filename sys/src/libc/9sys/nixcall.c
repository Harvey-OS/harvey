#include <u.h>
#include <libc.h>
#include <tos.h>

#define DBG	if(nixcalldebug)print

int nixcalldebug;

#define	NEXT(i)	(((i)+1)%CALLQSZ)

/*
 * Issue the system call
 */
void
nixcall(Nixcall *nc, int wakekernel)
{
	Callq *callq;

	callq = &_tos->callq;
	if(NEXT(callq->qw) == callq->qr)
		sysfatal("bug: increase Ncalls");

	memmove(&callq->q[callq->qw], nc, sizeof *nc);
	mfence();
	callq->qw = NEXT(callq->qw);
	DBG("libc: syscall qw %d\n", callq->qw);
	if(wakekernel && callq->ksleep)
		nixsyscall();
}

/*
 * Receive the reply for the system call
 * and return tag as given to its nixcall.
 */
void*
nixret(Nixret *nr)
{
	Callq *callq;
	void *tag;

	callq = &_tos->callq;

	if(callq->rr == callq->rw)
		return nil;
	tag = callq->r[callq->rr].tag;
	if(nr != nil)
		*nr = callq->r[callq->rr];
	mfence();
	callq->rr = NEXT(callq->rr);
	return tag;
}
