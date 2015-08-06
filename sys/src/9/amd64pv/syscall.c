/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "../port/error.h"

#include "../../libc/9syscall/sys.h"

#include <tos.h>

#include "amd64.h"
#include "ureg.h"

extern int nosmp;

typedef struct {
	uintptr_t	ip;
	Ureg*	arg0;
	char*	arg1;
	char	msg[ERRMAX];
	Ureg*	old;
	Ureg	ureg;
} NFrame;

/*
 *   Return user to state before notify()
 */
void
noted(Ureg* cur, uintptr_t arg0)
{
	Proc *up = externup();
	NFrame *nf;
	Note note;
	Ureg *nur;

	qlock(&(&up->debug)->qlock);
	if(arg0 != NRSTR && !up->notified){
		qunlock(&(&up->debug)->qlock);
		pprint("suicide: call to noted when not notified\n");
		pexit("Suicide", 0);
	}
	up->notified = 0;
	fpunoted();

	nf = up->ureg;

	/* sanity clause */
	if(!okaddr(PTR2UINT(nf), sizeof(NFrame), 0)){
		qunlock(&(&up->debug)->qlock);
		pprint("suicide: bad ureg %#p in noted\n", nf);
		pexit("Suicide", 0);
	}

	/*
	 * Check the segment selectors are all valid.
	 */
	nur = &nf->ureg;
	if(nur->cs != SSEL(SiUCS, SsRPL3) || nur->ss != SSEL(SiUDS, SsRPL3)) {
		qunlock(&(&up->debug)->qlock);
		pprint("suicide: bad segment selector in noted\n");
		pexit("Suicide", 0);
	}

	/* don't let user change system flags */
	nur->flags &= (Of|Df|Sf|Zf|Af|Pf|Cf);
	nur->flags |= cur->flags & ~(Of|Df|Sf|Zf|Af|Pf|Cf);

	memmove(cur, nur, sizeof(Ureg));

	switch((int)arg0){
	case NCONT:
	case NRSTR:
		if(!okaddr(nur->ip, BY2SE, 0) || !okaddr(nur->sp, BY2SE, 0)){
			qunlock(&(&up->debug)->qlock);
			pprint("suicide: trap in noted pc=%#p sp=%#p\n",
				nur->ip, nur->sp);
			pexit("Suicide", 0);
		}
		up->ureg = nf->old;
		qunlock(&(&up->debug)->qlock);
		break;
	case NSAVE:
		if(!okaddr(nur->ip, BY2SE, 0) || !okaddr(nur->sp, BY2SE, 0)){
			qunlock(&(&up->debug)->qlock);
			pprint("suicide: trap in noted pc=%#p sp=%#p\n",
				nur->ip, nur->sp);
			pexit("Suicide", 0);
		}
		qunlock(&(&up->debug)->qlock);

		splhi();
		nf->arg1 = nf->msg;
		nf->arg0 = &nf->ureg;
		cur->bp = PTR2UINT(nf->arg0);
		nf->ip = 0;
		cur->sp = PTR2UINT(nf);
		break;
	default:
		memmove(&note, &up->lastnote, sizeof(Note));
		qunlock(&(&up->debug)->qlock);
		pprint("suicide: bad arg %#p in noted: %s\n", arg0, note.msg);
		pexit(note.msg, 0);
		break;
	case NDFLT:
		memmove(&note, &up->lastnote, sizeof(Note));
		qunlock(&(&up->debug)->qlock);
		if(note.flag == NDebug)
			pprint("suicide: %s\n", note.msg);
		pexit(note.msg, note.flag != NDebug);
		break;
	}
}

/*
 *  Call user, if necessary, with note.
 *  Pass user the Ureg struct and the note on his stack.
 */
int
notify(Ureg* ureg)
{
	Proc *up = externup();
	int l;
	Mpl pl;
	Note note;
	uintptr_t sp;
	NFrame *nf;

	/*
	 * Calls procctl splhi, see comment in procctl for the reasoning.
	 */
	if(up->procctl)
		procctl(up);
	if(up->nnote == 0)
		return 0;

	fpunotify(ureg);

	pl = spllo();
	qlock(&(&up->debug)->qlock);

	up->notepending = 0;
	memmove(&note, &up->note[0], sizeof(Note));
	if(strncmp(note.msg, "sys:", 4) == 0){
		l = strlen(note.msg);
		if(l > ERRMAX-sizeof(" pc=0x0123456789abcdef"))
			l = ERRMAX-sizeof(" pc=0x0123456789abcdef");
		sprint(note.msg+l, " pc=%#p", ureg->ip);
	}

	if(note.flag != NUser && (up->notified || up->notify == nil)){
		qunlock(&(&up->debug)->qlock);
		if(note.flag == NDebug)
			pprint("suicide: %s\n", note.msg);
		pexit(note.msg, note.flag != NDebug);
	}

	if(up->notified){
		qunlock(&(&up->debug)->qlock);
		splhi();
		return 0;
	}

	if(up->notify == nil){
		qunlock(&(&up->debug)->qlock);
		pexit(note.msg, note.flag != NDebug);
	}
	if(!okaddr(PTR2UINT(up->notify), sizeof(ureg->ip), 0)){
		qunlock(&(&up->debug)->qlock);
		pprint("suicide: bad function address %#p in notify\n",
			up->notify);
		pexit("Suicide", 0);
	}

	sp = ureg->sp - sizeof(NFrame);
	if(!okaddr(sp, sizeof(NFrame), 1)){
		qunlock(&(&up->debug)->qlock);
		pprint("suicide: bad stack address %#p in notify\n", sp);
		pexit("Suicide", 0);
	}

	nf = UINT2PTR(sp);
	memmove(&nf->ureg, ureg, sizeof(Ureg));
	nf->old = up->ureg;
	up->ureg = nf;	/* actually the NFrame, for noted */
	memmove(nf->msg, note.msg, ERRMAX);
	nf->arg1 = nf->msg;
	nf->arg0 = &nf->ureg;
	ureg->di = (uintptr)nf->arg0;
	ureg->si = (uintptr)nf->arg1;
	//print("Setting di to %p and si to %p\n", ureg->di, ureg->si);
	ureg->bp = PTR2UINT(nf->arg0);
	nf->ip = 0;

	ureg->sp = sp;
	ureg->ip = PTR2UINT(up->notify);
	up->notified = 1;
	up->nnote--;
	memmove(&up->lastnote, &note, sizeof(Note));
	memmove(&up->note[0], &up->note[1], up->nnote*sizeof(Note));

	qunlock(&(&up->debug)->qlock);
	splx(pl);

	return 1;
}

void
noerrorsleft(void)
{
	Proc *up = externup();
	int i;

	if(up->nerrlab){
		/* NIX processes will have a waserror in their handler */
		if(up->ac != nil && up->nerrlab == 1)
			return;

		print("bad errstack: %d extra\n", up->nerrlab);
		for(i = 0; i < NERR; i++)
			print("sp=%#p pc=%#p\n",
				up->errlab[i].sp, up->errlab[i].pc);
		panic("error stack");
	}
}

/* it should be unsigned. FIXME */
void
syscall(int badscallnr, Ureg *ureg)
{
	// can only handle 4 args right now.
	uintptr_t a0, a1, a2, a3;
	uintptr_t a4, a5 = 0;

	a0 = ureg->di;
	a1 = ureg->si;
	a2 = ureg->dx;
	a3 = ureg->r10;
	a4 = ureg->r8;
	Proc *up = externup();
	unsigned int scallnr = (unsigned int) badscallnr;
	if (0) iprint("Syscall %d, %lx, %lx, %lx %lx %lx\n", scallnr, a0, a1, a2, a3, a4);
	char *e;
	uintptr_t	sp;
	int s, printallsyscalls;
	int64_t startns, stopns;
	Ar0 ar0;
	static Ar0 zar0;

	/* Do you want to print syscalls for debugging? */
	if(nosmp)
		printallsyscalls = 1;
	else
		printallsyscalls = 0;

	if(!userureg(ureg))
		panic("syscall: cs %#llux\n", ureg->cs);

	cycles(&up->kentry);

	m->syscall++;
	up->nsyscall++;
	up->nqsyscall++;
	up->insyscall = 1;
	up->pc = ureg->ip;
	up->dbgreg = ureg;
	sp = ureg->sp;
	startns = 0;
	if (0) hi("so far syscall!\n");
	if (printallsyscalls) {
		syscallfmt(scallnr, a0, a1, a2, a3, a4, a5);
		if(up->syscalltrace) {
			if(1) iprint("E %s\n", up->syscalltrace);
			free(up->syscalltrace);
			up->syscalltrace = nil;
		}
	}

	if(up->procctl == Proc_tracesyscall){
		/*
		 * Redundant validaddr.  Do we care?
		 * Tracing syscalls is not exactly a fast path...
		 * Beware, validaddr currently does a pexit rather
		 * than an error if there's a problem; that might
		 * change in the future.
		 */
		if(sp < (USTKTOP-BIGPGSZ) || sp > (USTKTOP-sizeof(up->arg)-BY2SE))
			validaddr(UINT2PTR(sp), sizeof(up->arg)+BY2SE, 0);

		syscallfmt(scallnr, a0, a1, a2, a3, a4, a5);
		up->procctl = Proc_stopme;
		procctl(up);
		if(up->syscalltrace)
			free(up->syscalltrace);
		up->syscalltrace = nil;
		startns = todget(nil);
	}
	if (0) hi("more syscall!\n");
	up->scallnr = scallnr;
	if(scallnr == RFORK)
		fpusysrfork(ureg);
	spllo();

	sp = ureg->sp;
	up->nerrlab = 0;
	ar0 = zar0;
	if(!waserror()){
		if(scallnr >= nsyscall || systab[scallnr].f == nil){
			pprint("bad sys call number %d pc %#llux\n",
				scallnr, ureg->ip);
			postnote(up, 1, "sys: bad sys call", NDebug);
			error(Ebadarg);
		}

		if(sp < (USTKTOP-BIGPGSZ) || sp > (USTKTOP-sizeof(up->arg)-BY2SE))
			validaddr(UINT2PTR(sp), sizeof(up->arg)+BY2SE, 0);

		memmove(up->arg, UINT2PTR(sp+BY2SE), sizeof(up->arg));
		up->psstate = systab[scallnr].n;
	if (0) hi("call syscall!\n");
		systab[scallnr].f(&ar0, a0, a1, a2, a3, a4, a5);
	if (0) hi("it returned!\n");
		if(scallnr == SYSR1){
			/*
			 * BUG: must go when ron binaries go.
			 * NIX: Returning from execac().
			 * This means that the process is back to the
			 * time sharing core. However, the process did
			 * already return from the system call, when dispatching
			 * the user code to the AC. The only thing left is to
			 * return. The user registers should be ok, because
			 * up->dbgreg has been the user context for the process.
			 */
			return;
		}
		poperror();
	}
	else{
		/* failure: save the error buffer for errstr */
		e = up->syserrstr;
		up->syserrstr = up->errstr;
		up->errstr = e;
		if(DBGFLG && up->pid == 1)
			iprint("%s: syscall %s error %s\n",
				up->text, systab[scallnr].n, up->syserrstr);
		ar0 = systab[scallnr].r;
	}

	/*
	 * NIX: for the execac() syscall, what follows is done within
	 * the system call, because it never returns.
	 * See acore.c:/^retfromsyscall
	 */

	noerrorsleft();

	/*
	 * Put return value in frame.
	 */
	ureg->ax = ar0.p;

	if (printallsyscalls) {
		stopns = todget(nil);
		sysretfmt(scallnr, &ar0, startns, stopns, a0, a1, a2, a3, a4, a5);
		if(up->syscalltrace) {
			if (1) iprint("X %s\n", up->syscalltrace);
			free(up->syscalltrace);
			up->syscalltrace = nil;
		}
	}

	if(up->procctl == Proc_tracesyscall){
		stopns = todget(nil);
		up->procctl = Proc_stopme;
		sysretfmt(scallnr, &ar0, startns, stopns, a0, a1, a2, a3, a4, a5);
		s = splhi();
		procctl(up);
		splx(s);
		if(up->syscalltrace)
			free(up->syscalltrace);
		up->syscalltrace = nil;
	}else if(up->procctl == Proc_totc || up->procctl == Proc_toac)
		procctl(up);

	if (0) hi("past sysretfmt\n");
	up->insyscall = 0;
	up->psstate = 0;

	if(scallnr == NOTED)
		noted(ureg, a0);

	if (0) hi("now to splihi\n");
	splhi();
	if(scallnr != RFORK && (up->procctl || up->nnote))
		notify(ureg);

	/* if we delayed sched because we held a lock, sched now */
	if(up->delaysched){
		sched();
		splhi();
	}
	kexit(ureg);
	if (0) hi("done kexit\n");
}

uintptr_t
sysexecstack(uintptr_t stack, int argc)
{
	/*
	 * Given a current bottom-of-stack and a count
	 * of pointer arguments to be pushed onto it followed
	 * by an integer argument count, return a suitably
	 * aligned new bottom-of-stack which will satisfy any
	 * hardware stack-alignment contraints.
	 * Rounding the stack down to be aligned with the
	 * natural size of a pointer variable usually suffices,
	 * but some architectures impose further restrictions,
	 * e.g. 32-bit SPARC, where the stack must be 8-byte
	 * aligned although pointers and integers are 32-bits.
	 */
	USED(argc);

	return STACKALIGN(stack);
}

void*
sysexecregs(uintptr_t entry, uint32_t ssize, void *argv, uint32_t nargs, void *tos)
{
	Proc *up = externup();
	uintptr_t *sp;
	Ureg *ureg;

	sp = (uintptr_t*)(USTKTOP - ssize);

	ureg = up->dbgreg;
	ureg->sp = PTR2UINT(sp);
	ureg->ip = entry;
	ureg->type = 64;			/* fiction for acid */
	ureg->di = nargs;
	ureg->si = (uintptr_t)argv;
	ureg->dx = (uintptr_t)tos;

	/*
	 * return the address of kernel/user shared data
	 * (e.g. clock stuff)
	 */
	return UINT2PTR(USTKTOP-sizeof(Tos));
}

void
sysprocsetup(Proc* p)
{
	fpusysprocsetup(p);
}

void
sysrforkchild(Proc* child, Proc* parent)
{
	Ureg *cureg;
// If STACKPAD is 1 things go very bad very quickly.
// But it is the right value ...
#define STACKPAD 1 /* for return PC? */
	/*
	 * Add STACKPAD*BY2SE to the stack to account for
	 *  - the return PC
	 *  (NOT NOW) - trap's arguments (syscallnr, ureg)
	 */
	child->sched.sp = PTR2UINT(child->kstack+KSTACK-((sizeof(Ureg)+STACKPAD*BY2SE)));
	child->sched.pc = PTR2UINT(sysrforkret);

	cureg = (Ureg*)(child->sched.sp+STACKPAD*BY2SE);
	memmove(cureg, parent->dbgreg, sizeof(Ureg));

	/* Things from bottom of syscall which were never executed */
	child->psstate = 0;
	child->insyscall = 0;
	//iprint("Child SP set tp %p\n", (void *)child->sched.sp);

	fpusysrforkchild(child, parent);
}
