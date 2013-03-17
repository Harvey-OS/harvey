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

typedef struct {
	uintptr	ip;
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
noted(Ureg* cur, uintptr arg0)
{
	NFrame *nf;
	Note note;
	Ureg *nur;

	qlock(&up->debug);
	if(arg0 != NRSTR && !up->notified){
		qunlock(&up->debug);
		pprint("suicide: call to noted when not notified\n");
		pexit("Suicide", 0);
	}
	up->notified = 0;
	fpunoted();

	nf = up->ureg;

	/* sanity clause */
	if(!okaddr(PTR2UINT(nf), sizeof(NFrame), 0)){
		qunlock(&up->debug);
		pprint("suicide: bad ureg %#p in noted\n", nf);
		pexit("Suicide", 0);
	}

	/*
	 * Check the segment selectors are all valid.
	 */
	nur = &nf->ureg;
	if(nur->cs != SSEL(SiUCS, SsRPL3) || nur->ss != SSEL(SiUDS, SsRPL3)
	|| nur->ds != SSEL(SiUDS, SsRPL3) || nur->es != SSEL(SiUDS, SsRPL3)
	|| nur->gs != SSEL(SiUDS, SsRPL3)){
		qunlock(&up->debug);
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
			qunlock(&up->debug);
			pprint("suicide: trap in noted pc=%#p sp=%#p\n",
				nur->ip, nur->sp);
			pexit("Suicide", 0);
		}
		up->ureg = nf->old;
		qunlock(&up->debug);
		break;
	case NSAVE:
		if(!okaddr(nur->ip, BY2SE, 0) || !okaddr(nur->sp, BY2SE, 0)){
			qunlock(&up->debug);
			pprint("suicide: trap in noted pc=%#p sp=%#p\n",
				nur->ip, nur->sp);
			pexit("Suicide", 0);
		}
		qunlock(&up->debug);

		splhi();
		nf->arg1 = nf->msg;
		nf->arg0 = &nf->ureg;
		cur->bp = PTR2UINT(nf->arg0);
		nf->ip = 0;
		cur->sp = PTR2UINT(nf);
		break;
	default:
		memmove(&note, &up->lastnote, sizeof(Note));
		qunlock(&up->debug);
		pprint("suicide: bad arg %#p in noted: %s\n", arg0, note.msg);
		pexit(note.msg, 0);
		break;
	case NDFLT:
		memmove(&note, &up->lastnote, sizeof(Note));
		qunlock(&up->debug);
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
	int l;
	Mpl pl;
	Note note;
	uintptr sp;
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
	qlock(&up->debug);

	up->notepending = 0;
	memmove(&note, &up->note[0], sizeof(Note));
	if(strncmp(note.msg, "sys:", 4) == 0){
		l = strlen(note.msg);
		if(l > ERRMAX-sizeof(" pc=0x0123456789abcdef"))
			l = ERRMAX-sizeof(" pc=0x0123456789abcdef");
		sprint(note.msg+l, " pc=%#p", ureg->ip);
	}

	if(note.flag != NUser && (up->notified || up->notify == nil)){
		qunlock(&up->debug);
		if(note.flag == NDebug)
			pprint("suicide: %s\n", note.msg);
		pexit(note.msg, note.flag != NDebug);
	}

	if(up->notified){
		qunlock(&up->debug);
		splhi();
		return 0;
	}

	if(up->notify == nil){
		qunlock(&up->debug);
		pexit(note.msg, note.flag != NDebug);
	}
	if(!okaddr(PTR2UINT(up->notify), sizeof(ureg->ip), 0)){
		qunlock(&up->debug);
		pprint("suicide: bad function address %#p in notify\n",
			up->notify);
		pexit("Suicide", 0);
	}

	sp = ureg->sp;
	sp -= 128;	/* debugging: preserve context causing problem */
	sp -= sizeof(NFrame);
	if(!okaddr(sp, sizeof(NFrame), 1)){
		qunlock(&up->debug);
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
	ureg->bp = PTR2UINT(nf->arg0);
	nf->ip = 0;

	ureg->sp = sp;
	ureg->ip = PTR2UINT(up->notify);
	up->notified = 1;
	up->nnote--;
	memmove(&up->lastnote, &note, sizeof(Note));
	memmove(&up->note[0], &up->note[1], up->nnote*sizeof(Note));

	qunlock(&up->debug);
	splx(pl);

	return 1;
}

void
noerrorsleft(void)
{
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

int
badsyscall(int scallnr)
{
	if(scallnr >= nsyscall || systab[scallnr].f == nil)
		return 1;

	return 0;
}

void
traceend(int scallnr, uintptr sp, Ar0 *ar0, vlong startns, vlong stopns)
{
	int s;
	up->procctl = Proc_stopme;
	sysretfmt(scallnr, (va_list)(sp+BY2SE), ar0, startns, stopns);
	s = splhi();
	procctl(up);
	splx(s);
	if(up->syscalltrace)
		free(up->syscalltrace);
	up->syscalltrace = nil;
}

/* special case: leaving sysrfork as a child. We consider 0 time to have passed.
 * always return 0.
 */
void
rforktraceend(void)
{
	traceend(RFORK, 0, nil, 0, 0);
}
/* it should be unsigned. FIXME */
void
syscall(int badscallnr, Ureg* ureg)
{
	unsigned int scallnr = (u8int) badscallnr;
	char *e;
	uintptr	sp;
	int s;
	vlong startns, stopns;
	Ar0 ar0;
	static Ar0 zar0;
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

		syscallfmt(scallnr, (va_list)(sp+BY2SE));
		up->procctl = Proc_stopme;
		procctl(up);
		if(up->syscalltrace)
			free(up->syscalltrace);
		up->syscalltrace = nil;
		startns = todget(nil);
	}

	up->scallnr = scallnr;
	if(scallnr == RFORK)
		fpusysrfork(ureg);
	spllo();

	sp = ureg->sp;
	up->nerrlab = 0;
	ar0 = zar0;
	if(!waserror()){
		if(badsyscall(scallnr)){
			pprint("bad sys call number %d pc %#llux\n",
				scallnr, ureg->ip);
			postnote(up, 1, "sys: bad sys call", NDebug);
			error(Ebadarg);
		}

		if(sp < (USTKTOP-BIGPGSZ) || sp > (USTKTOP-sizeof(up->arg)-BY2SE))
			validaddr(UINT2PTR(sp), sizeof(up->arg)+BY2SE, 0);

		memmove(up->arg, UINT2PTR(sp+BY2SE), sizeof(up->arg));
		up->psstate = systab[scallnr].n;

		systab[scallnr].f(&ar0, (va_list)up->arg);
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
		char *syscallname = "INVALID";
		ar0.i = -1;
		/* failure: save the error buffer for errstr */
		e = up->syserrstr;
		up->syserrstr = up->errstr;
		up->errstr = e;
		/* this used to panic the kernel on a bad system call # */
		if(!badsyscall(scallnr)){
			syscallname = systab[scallnr].n;
			ar0 = systab[scallnr].r;
		}
		if(DBGFLG && up->pid == 1)
			iprint("%s: syscall %s error %s\n",
				up->text, syscallname, up->syserrstr);
	}

	/*
	 * NIX: for the execac() syscall, what follows is done within
	 * the system call, because it never returns.
	 */

	noerrorsleft();

	/*
	 * Put return value in frame.
	 */
	ureg->ax = ar0.p;

	if(up->procctl == Proc_tracesyscall){
		stopns = todget(nil);
		traceend(scallnr, sp, &ar0, startns, stopns);
	}else if(up->procctl == Proc_totc || up->procctl == Proc_toac)
		procctl(up);


	up->insyscall = 0;
	up->psstate = 0;

	if(scallnr == NOTED)
		noted(ureg, *(uintptr*)(sp+BY2SE));

	splhi();
	if(scallnr != RFORK && (up->procctl || up->nnote))
		notify(ureg);

	/* if we delayed sched because we held a lock, sched now */
	if(up->delaysched){
		sched();
		splhi();
	}
	kexit(ureg);
}

uintptr
sysexecstack(uintptr stack, int argc)
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
sysexecregs(uintptr entry, ulong ssize, ulong nargs)
{
	uintptr *sp;
	Ureg *ureg;

	sp = (uintptr*)(USTKTOP - ssize);
	*--sp = nargs;

	ureg = up->dbgreg;
	ureg->sp = PTR2UINT(sp);
	ureg->ip = entry;
	ureg->type = 64;			/* fiction for acid */

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
sysrforkchild(Proc* child, Proc* parent, void (*ret)(void), uintptr stack)
{
	Ureg *cureg;

	/*
	 * Add 3*BY2SE to the stack to account for
	 *  - the return PC
	 *  - trap's arguments (syscallnr, ureg)
	 */
	child->sched.sp = PTR2UINT(child->kstack+KSTACK-(sizeof(Ureg)+3*BY2SE));
	child->sched.pc = PTR2UINT(ret);

	cureg = (Ureg*)(child->sched.sp+3*BY2SE);
	memmove(cureg, parent->dbgreg, sizeof(Ureg));
	if (stack)
		cureg->sp = stack;
	/* Things from bottom of syscall which were never executed */
	child->psstate = 0;
	child->insyscall = 0;

	fpusysrforkchild(child, parent);
}
