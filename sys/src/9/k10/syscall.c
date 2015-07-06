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
	Mach *m = machp();
	NFrame *nf;
	Note note;
	Ureg *nur;

	qlock(&m->externup->debug);
	if(arg0 != NRSTR && !m->externup->notified){
		qunlock(&m->externup->debug);
		pprint("suicide: call to noted when not notified\n");
		pexit("Suicide", 0);
	}
	m->externup->notified = 0;
	fpunoted();

	nf = m->externup->ureg;

	/* sanity clause */
	if(!okaddr(PTR2UINT(nf), sizeof(NFrame), 0)){
		qunlock(&m->externup->debug);
		pprint("suicide: bad ureg %#p in noted\n", nf);
		pexit("Suicide", 0);
	}

	/*
	 * Check the segment selectors are all valid.
	 */
	nur = &nf->ureg;
	if(nur->cs != SSEL(SiUCS, SsRPL3) || nur->ss != SSEL(SiUDS, SsRPL3)) {
		qunlock(&m->externup->debug);
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
			qunlock(&m->externup->debug);
			pprint("suicide: trap in noted pc=%#p sp=%#p\n",
				nur->ip, nur->sp);
			pexit("Suicide", 0);
		}
		m->externup->ureg = nf->old;
		qunlock(&m->externup->debug);
		break;
	case NSAVE:
		if(!okaddr(nur->ip, BY2SE, 0) || !okaddr(nur->sp, BY2SE, 0)){
			qunlock(&m->externup->debug);
			pprint("suicide: trap in noted pc=%#p sp=%#p\n",
				nur->ip, nur->sp);
			pexit("Suicide", 0);
		}
		qunlock(&m->externup->debug);

		splhi();
		nf->arg1 = nf->msg;
		nf->arg0 = &nf->ureg;
		cur->bp = PTR2UINT(nf->arg0);
		nf->ip = 0;
		cur->sp = PTR2UINT(nf);
		break;
	default:
		memmove(&note, &m->externup->lastnote, sizeof(Note));
		qunlock(&m->externup->debug);
		pprint("suicide: bad arg %#p in noted: %s\n", arg0, note.msg);
		pexit(note.msg, 0);
		break;
	case NDFLT:
		memmove(&note, &m->externup->lastnote, sizeof(Note));
		qunlock(&m->externup->debug);
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
	Mach *m = machp();
	int l;
	Mpl pl;
	Note note;
	uintptr_t sp;
	NFrame *nf;

	/*
	 * Calls procctl splhi, see comment in procctl for the reasoning.
	 */
	if(m->externup->procctl)
		procctl(m->externup);
	if(m->externup->nnote == 0)
		return 0;

	fpunotify(ureg);

	pl = spllo();
	qlock(&m->externup->debug);

	m->externup->notepending = 0;
	memmove(&note, &m->externup->note[0], sizeof(Note));
	if(strncmp(note.msg, "sys:", 4) == 0){
		l = strlen(note.msg);
		if(l > ERRMAX-sizeof(" pc=0x0123456789abcdef"))
			l = ERRMAX-sizeof(" pc=0x0123456789abcdef");
		sprint(note.msg+l, " pc=%#p", ureg->ip);
	}

	if(note.flag != NUser && (m->externup->notified || m->externup->notify == nil)){
		qunlock(&m->externup->debug);
		if(note.flag == NDebug)
			pprint("suicide: %s\n", note.msg);
		pexit(note.msg, note.flag != NDebug);
	}

	if(m->externup->notified){
		qunlock(&m->externup->debug);
		splhi();
		return 0;
	}

	if(m->externup->notify == nil){
		qunlock(&m->externup->debug);
		pexit(note.msg, note.flag != NDebug);
	}
	if(!okaddr(PTR2UINT(m->externup->notify), sizeof(ureg->ip), 0)){
		qunlock(&m->externup->debug);
		pprint("suicide: bad function address %#p in notify\n",
			m->externup->notify);
		pexit("Suicide", 0);
	}

	sp = ureg->sp - sizeof(NFrame);
	if(!okaddr(sp, sizeof(NFrame), 1)){
		qunlock(&m->externup->debug);
		pprint("suicide: bad stack address %#p in notify\n", sp);
		pexit("Suicide", 0);
	}

	nf = UINT2PTR(sp);
	memmove(&nf->ureg, ureg, sizeof(Ureg));
	nf->old = m->externup->ureg;
	m->externup->ureg = nf;	/* actually the NFrame, for noted */
	memmove(nf->msg, note.msg, ERRMAX);
	nf->arg1 = nf->msg;
	nf->arg0 = &nf->ureg;
	ureg->di = (uintptr)nf->arg0;
	ureg->si = (uintptr)nf->arg1;
	//print("Setting di to %p and si to %p\n", ureg->di, ureg->si);
	ureg->bp = PTR2UINT(nf->arg0);
	nf->ip = 0;

	ureg->sp = sp;
	ureg->ip = PTR2UINT(m->externup->notify);
	m->externup->notified = 1;
	m->externup->nnote--;
	memmove(&m->externup->lastnote, &note, sizeof(Note));
	memmove(&m->externup->note[0], &m->externup->note[1], m->externup->nnote*sizeof(Note));

	qunlock(&m->externup->debug);
	splx(pl);

	return 1;
}

void
noerrorsleft(void)
{
	Mach *m = machp();
	int i;

	if(m->externup->nerrlab){
		/* NIX processes will have a waserror in their handler */
		if(m->externup->ac != nil && m->externup->nerrlab == 1)
			return;

		print("bad errstack: %d extra\n", m->externup->nerrlab);
		for(i = 0; i < NERR; i++)
			print("sp=%#p pc=%#p\n",
				m->externup->errlab[i].sp, m->externup->errlab[i].pc);
		panic("error stack");
	}
}

int printallsyscalls;
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
	Mach *m = machp();
	unsigned int scallnr = (unsigned int) badscallnr;
	if (0) iprint("Syscall %d, %lx, %lx, %lx %lx %lx\n", scallnr, a0, a1, a2, a3, a4);
	char *e;
	uintptr_t	sp;
	int s;
	int64_t startns, stopns;
	Ar0 ar0;
	static Ar0 zar0;

	if(!userureg(ureg))
		panic("syscall: cs %#llux\n", ureg->cs);

	cycles(&m->externup->kentry);

	m->syscall++;
	m->externup->nsyscall++;
	m->externup->nqsyscall++;
	m->externup->insyscall = 1;
	m->externup->pc = ureg->ip;
	m->externup->dbgreg = ureg;
	sp = ureg->sp;
	startns = 0;
	if (0) hi("so far syscall!\n");
	if (printallsyscalls) {
		syscallfmt(scallnr, a0, a1, a2, a3, a4, a5);
		if(m->externup->syscalltrace) {
			print("E %s\n", m->externup->syscalltrace);
			free(m->externup->syscalltrace);
			m->externup->syscalltrace = nil;
		}
	}

	if(m->externup->procctl == Proc_tracesyscall){
		/*
		 * Redundant validaddr.  Do we care?
		 * Tracing syscalls is not exactly a fast path...
		 * Beware, validaddr currently does a pexit rather
		 * than an error if there's a problem; that might
		 * change in the future.
		 */
		if(sp < (USTKTOP-BIGPGSZ) || sp > (USTKTOP-sizeof(m->externup->arg)-BY2SE))
			validaddr(UINT2PTR(sp), sizeof(m->externup->arg)+BY2SE, 0);

		syscallfmt(scallnr, a0, a1, a2, a3, a4, a5);
		m->externup->procctl = Proc_stopme;
		procctl(m->externup);
		if(m->externup->syscalltrace)
			free(m->externup->syscalltrace);
		m->externup->syscalltrace = nil;
		startns = todget(nil);
	}
	if (0) hi("more syscall!\n");
	m->externup->scallnr = scallnr;
	if(scallnr == RFORK)
		fpusysrfork(ureg);
	spllo();

	sp = ureg->sp;
	m->externup->nerrlab = 0;
	ar0 = zar0;
	if(!waserror()){
		if(scallnr >= nsyscall || systab[scallnr].f == nil){
			pprint("bad sys call number %d pc %#llux\n",
				scallnr, ureg->ip);
			postnote(m->externup, 1, "sys: bad sys call", NDebug);
			error(Ebadarg);
		}

		if(sp < (USTKTOP-BIGPGSZ) || sp > (USTKTOP-sizeof(m->externup->arg)-BY2SE))
			validaddr(UINT2PTR(sp), sizeof(m->externup->arg)+BY2SE, 0);

		memmove(m->externup->arg, UINT2PTR(sp+BY2SE), sizeof(m->externup->arg));
		m->externup->psstate = systab[scallnr].n;
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
			 * m->externup->dbgreg has been the user context for the process.
			 */
			return;
		}
		poperror();
	}
	else{
		/* failure: save the error buffer for errstr */
		e = m->externup->syserrstr;
		m->externup->syserrstr = m->externup->errstr;
		m->externup->errstr = e;
		if(DBGFLG && m->externup->pid == 1)
			iprint("%s: syscall %s error %s\n",
				m->externup->text, systab[scallnr].n, m->externup->syserrstr);
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
		if(m->externup->syscalltrace) {
			print("X %s\n", m->externup->syscalltrace);
			free(m->externup->syscalltrace);
			m->externup->syscalltrace = nil;
		}
	}

	if(m->externup->procctl == Proc_tracesyscall){
		stopns = todget(nil);
		m->externup->procctl = Proc_stopme;
		sysretfmt(scallnr, &ar0, startns, stopns, a0, a1, a2, a3, a4, a5);
		s = splhi();
		procctl(m->externup);
		splx(s);
		if(m->externup->syscalltrace)
			free(m->externup->syscalltrace);
		m->externup->syscalltrace = nil;
	}else if(m->externup->procctl == Proc_totc || m->externup->procctl == Proc_toac)
		procctl(m->externup);

	if (0) hi("past sysretfmt\n");
	m->externup->insyscall = 0;
	m->externup->psstate = 0;

	if(scallnr == NOTED)
		noted(ureg, a0);

	if (0) hi("now to splhi\n");
	splhi();
	if(scallnr != RFORK && (m->externup->procctl || m->externup->nnote))
		notify(ureg);

	/* if we delayed sched because we held a lock, sched now */
	if(m->externup->delaysched){
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
	Mach *m = machp();
	uintptr_t *sp;
	Ureg *ureg;

	ssize = (ssize + 15) & ~15; /* userland (fpu) needs %rsp to be 16-aligned */
	sp = (uintptr_t*)(USTKTOP - ssize);

	ureg = m->externup->dbgreg;
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
