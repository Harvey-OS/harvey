#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"tos.h"

typedef struct Nixpctl Nixpctl;

struct Nixpctl
{
	int	cmd;
	void*	arg;
	Rendez	kwait;
	Rendez	uwait;
	Tos*	tos;
	Proc*	kproc;
};

enum
{
	NCnone = 0,
	NCinit,
	NCdie,
	NCdone = ~0,

	Nwait = 1,	/* how many seconds to poll before sleeping */
};

/*
 * TODO:
 * A process is issuing a system call using the Callq.
 * If the call does not block, we can handle it here.
 * If it can block, we must start another kproc to
 * handle it or the user won't be able to issue other system
 * calls before the call unblocks.
 * Replies can't be handled as they are now. As system calls
 * are done, their replies must be added to the reply queue,
 * the order won't be the same.
 */
static void
nixsyscall(Nixcall *nc, Nixret *nr)
{
	/* testing */
	DBG("nixsyscall %d\n", nc->scall);
	nr->tag = nc->tag;
	nr->sret = -nc->scall;
	nr->err = nil;
}


#define	NEXT(i)	(((i)+1)%CALLQSZ)

static int
cmddone(void *a)
{
	Nixpctl *pctl;

	pctl = a;
	return pctl->cmd == NCdone;
}

static int
cmdready(void *a)
{
	Nixpctl *pctl;
	Tos *tos;
	Callq *callq;

	pctl = a;
	tos = pctl->tos;
	callq = &tos->callq;
	return pctl->cmd != NCdone || callq->qr != callq->qw;
}

/*
 * Kernel context for a NIX process using system call queues.
 * It shares all segments with the user, and polls for
 * system calls.
 * A note posted aborts the process.
 */
static void
nixkproc(void *a)
{
	Nixpctl *pctl;
	Proc *p;
	Callq *callq;
	int i;

	pctl = a;
	p = pctl->arg;
	DBG("nixkproc %d for proc %d\n", up->pid, p->pid);
	for(i = 0; i < NSEG; i++)
		if(p->seg[i] != nil){
			up->seg[i] = p->seg[i];
			incref(up->seg[i]);
		}
	/*
	 * BUG: must also share fds and other resources
	 * so we could issue system calls for p.
	 */
	pctl->kproc = up;
	pctl->cmd = NCdone;
	wakeup(&pctl->uwait);
	up->errstr[0] = 0;
	if(waserror()){
		DBG("nixkproc: noted %s\n", up->errstr);
		goto Abort;
	}
	callq = &pctl->tos->callq;
	for(;;){
		/*
		 * Poll the call queue for system calls and
		 * pctl for commands.
		 */
		for(i = 0; i < 1000; i++)
			if(!cmdready(pctl))
				yield();
			else
				break;
		/*
		 * Wait for them if it's been a while.
		 */
		while(!cmdready(pctl)){
			callq->ksleep = 1;
			DBG("nixkproc: sleeping\n");
			mfence();
			sleep(&pctl->kwait, cmdready, pctl);
			DBG("nixkproc: awake\n");
		}
		callq->ksleep = 0;
		mfence();
		DBG("nixcall cmd %d qr %d qw %d\n",
			pctl->cmd, callq->qr, callq->qw);
		switch(pctl->cmd){
		case NCdone:
			break;
		case NCdie:
			/*
			 * How could we die if we are in the
			 * middle of a system call?
			 * We could perhaps be notified by the
			 * user process?
			 */
			goto Done;
			break;
		default:
			panic("unknown command in nixkproc");
		}
		if(callq->qr != callq->qw){
			nixsyscall(&callq->q[callq->qr], &callq->r[callq->rw]);
			mfence();
			callq->qr = NEXT(callq->qr);
			mfence();
			callq->rw = NEXT(callq->rw);
		}
	}
Done:
	poperror();
Abort:
	free(pctl);
	DBG("nixkproc %d done\n", up->pid);
	pexit(nil, 1);
}

/*
 * Called from pexit; get rid of the kproc for this process.
 */
void
stopnixproc(void)
{
	Nixpctl *pctl;

	pctl = up->nixpctl;
	if(pctl == nil)
		return;
	DBG("stopnixproc\n");
	pctl->cmd = NCdie;
	postnote(pctl->kproc, 1, "kill", NDebug);
	up->nixpctl = nil;
}

/*
 * Actual system call to indicate that this process is
 * using queue based system calls, and to awake its kernel
 * process if it's aslept waiting for work.
 */
void
sysnixsyscall(Ar0*, va_list)
{
	DBG("sysnixsyscall. nixpctl %#p\n", up->nixpctl);
	if(up->nixpctl == 0){
		up->nixpctl = smalloc(sizeof(Nixpctl));
		up->nixpctl->cmd = NCinit;
		up->nixpctl->tos = (Tos*)(USTKTOP-sizeof(Tos));
		up->nixpctl->arg = up;
		kproc("nix", nixkproc, up->nixpctl);
		sleep(&up->nixpctl->uwait, cmddone, up->nixpctl);
	}
	wakeup(&up->nixpctl->kwait);
}
