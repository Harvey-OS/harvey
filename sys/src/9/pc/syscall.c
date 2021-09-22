#include	"u.h"
#include	"tos.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"../port/error.h"

void	kexit(Ureg*);			/* trap.c */

void	noted(Ureg*, ulong);

/*
 *  system calls
 */
#include "../port/systab.h"

static void
stopme_freetrace(void)
{
	int s;

	up->procctl = Proc_stopme;
	s = splhi();
	procctl(up);
	splx(s);

	if(up->syscalltrace)
		free(up->syscalltrace);
	up->syscalltrace = nil;
}

static void
tracecall(Ureg *ureg, ulong sp, ulong scallnr, vlong *stnsp)
{
	/*
	 * Redundant validaddr.  Do we care?  Tracing syscalls is not exactly
	 * a fast path...  Beware, validaddr currently calls pexit rather than
	 * error if there's a problem; that might change in the future.
	 */
	if(sp < USTKTOP-BY2PG || sp > USTKTOP-(sizeof(Sargs)+BY2WD))
		validaddr(sp, sizeof(Sargs)+BY2WD, 0);

	syscallfmt(scallnr, ureg->pc, (va_list)(sp+BY2WD));
	stopme_freetrace();
	*stnsp = todget(nil);
}

static void
traceret(ulong sp, ulong scallnr, long ret, vlong startns)
{
	up->procctl = Proc_stopme;
	sysretfmt(scallnr, (va_list)(sp+BY2WD), ret, startns, todget(nil));
	stopme_freetrace();
}

static void
baderrstk(ulong scallnr)
{
	int i;

	print("bad errstack [%lud]: %d extra\n", scallnr, up->nerrlab);
	for(i = 0; i < NERR; i++)
		print("sp=%lux pc=%lux\n", up->errlab[i].sp, up->errlab[i].pc);
	panic("error stack");
}

/*
 *  Syscall is called directly from assembler without going through trap().
 *  Syscall is an optimized version of trap().
 */
void
syscall(Ureg* ureg)
{
	long ret;
	ulong sp, scallnr;
	vlong startns;
	char *e;

	if((ureg->cs & 0xFFFF) != UESEL)
		panic("syscall: cs %#4.4luX", ureg->cs);

	cycles(&up->kentry);

	m->syscall++;
	up->insyscall = 1;
	up->pc = ureg->pc;
	up->dbgreg = ureg;

	sp = ureg->usp;
	scallnr = ureg->ax;
	up->scallnr = scallnr;

	if(up->procctl == Proc_tracesyscall)
		tracecall(ureg, sp, scallnr, &startns);
	if(scallnr == RFORK && up->fpstate == FPactive){
		fpsave(&up->fpsave);
		up->fpstate = FPinactive;
	}
	spllo();

	up->nerrlab = 0;
	ret = -1;
	if(!waserror()){
		/* validate and perform the system call */
		if(scallnr >= nsyscall || systab[scallnr] == 0){
			pprint("bad sys call number %lud pc %lux\n",
				scallnr, ureg->pc);
			postnote(up, 1, "sys: bad sys call", NDebug);
			error(Ebadarg);
		}

		if(sp < USTKTOP-BY2PG || sp > USTKTOP-(sizeof(Sargs)+BY2WD))
			validaddr(sp, sizeof(Sargs)+BY2WD, 0);

		up->s = *((Sargs*)(sp+BY2WD));
		up->psstate = sysctab[scallnr];

		ret = systab[scallnr](up->s.args);
		poperror();
	}else{
		/* failure: save the error buffer for errstr */
		e = up->syserrstr;
		up->syserrstr = up->errstr;
		up->errstr = e;
		if(0 && up->pid == 1)
			print("syscall %lud error %s\n", scallnr, up->syserrstr);
	}
	if(up->nerrlab)
		baderrstk(scallnr);

	/*
	 *  Put return value in frame.  On the x86 the syscall is
	 *  just another trap and the return value from syscall is
	 *  ignored.  On other machines the return value is put into
	 *  the results register by caller of syscall.
	 */
	ureg->ax = ret;
	if(up->procctl == Proc_tracesyscall)
		traceret(sp, scallnr, ret, startns);

	up->insyscall = 0;
	up->psstate = 0;

	if(scallnr == NOTED)
		noted(ureg, *(ulong*)(sp+BY2WD));
	if(scallnr!=RFORK && (up->procctl || up->nnote)){
		splhi();
		notify(ureg);
	}
	if(up->delaysched)	/* we delayed sched because we held a lock? */
		sched();
	splhi();
	kexit(ureg);
}

static void
dieifbadaddr(ulong addr, ulong len, int write, char *what)
{
	if(!okaddr(addr, len, write)){
		qunlock(&up->debug);
		pprint("suicide: bad address in notify: %s %#p\n", what, addr);
		pexit("Suicide", 0);
	}
}

typedef struct Stkargs {
	void	*pc;
	Ureg	*ureg;
	char	*errstr;
} Stkargs;

enum {
	Stkspace = ERRMAX + BY2WD + sizeof(Stkargs),
};

/*
 *  Call user, if necessary, with note.
 *  Pass user the Ureg struct and the note on his stack.
 */
int
notify(Ureg* ureg)
{
	int l;
	char *err;
	ulong s, sp;
	Note *n;
	Stkargs *stkargs;

	if(up->procctl)
		procctl(up);
	if(up->nnote == 0)
		return 0;

	if(up->fpstate == FPactive){
		fpsave(&up->fpsave);
		up->fpstate = FPinactive;
	}
	up->fpstate |= FPillegal;

	s = spllo();
	qlock(&up->debug);
	up->notepending = 0;
	n = &up->note[0];
	if(strncmp(n->msg, "sys:", 4) == 0){
		l = strlen(n->msg);
		if(l > ERRMAX-15)	/* " pc=0x12345678\0" */
			l = ERRMAX-15;
		seprint(n->msg+l, &n->msg[sizeof n->msg], " pc=%#.8lux",
			ureg->pc);
	}

	if(n->flag!=NUser && (up->notified || up->notify==0)){
		if(n->flag == NDebug)
			pprint("suicide: %s\n", n->msg);
		qunlock(&up->debug);
		pexit(n->msg, n->flag!=NDebug);
	}

	if(up->notified){
		qunlock(&up->debug);
		splhi();
		return 0;
	}

	if(!up->notify){
		qunlock(&up->debug);
		pexit(n->msg, n->flag!=NDebug);
	}

	/*
	 * deliver a note to current process: validate writable stack region
	 * in ureg->usp for Ureg+errstr & up->notify; push Ureg, errstr, etc.;
	 * set note variables.
	 *
	 * old (1st ed.) ports save(d) into up->Notsave here, to be verified
	 * in noted().  now noted checks the segment selectors and flags in
	 * its ureg arg. against those in up->ureg.
	 *
	 * debugging: -64*BY2WD to preserve context causing problem.
	 */
	sp = ureg->usp - 64*BY2WD - sizeof(Ureg);
    if(0) print("%s %lud: notify %.8lux %.8lux %.8lux %s\n",
	up->text, up->pid, ureg->pc, ureg->usp, sp, n->msg);

	dieifbadaddr((ulong)up->notify, 1, Read, "up->notify");
	dieifbadaddr(sp - Stkspace, sizeof(Ureg) + Stkspace, Write, "sp");

	memmove((Ureg*)sp, ureg, sizeof(Ureg));
	*(Ureg**)(sp-BY2WD) = up->ureg;	/* word under Ureg is old up->ureg */
	up->ureg = (void*)sp;

	sp -= BY2WD+ERRMAX;
	err = (char *)sp;
	memmove(err, up->note[0].msg, ERRMAX);

	sp -= sizeof *stkargs;
	stkargs = (Stkargs *)sp;
	stkargs->pc = 0;
	stkargs->ureg = up->ureg;
	stkargs->errstr = err;

	ureg->usp = sp;
	ureg->pc = (ulong)up->notify;
	up->notified = 1;
	up->nnote--;
	memmove(&up->lastnote, &up->note[0], sizeof(Note));
	memmove(&up->note[0], &up->note[1], up->nnote*sizeof(Note));

	qunlock(&up->debug);
	splx(s);
	return 1;
}

/*
 *   Return user to state before notify()
 */
void
noted(Ureg* ureg, ulong arg0)
{
	Ureg *nureg;
	ulong oureg, sp;

	qlock(&up->debug);
	if(arg0!=NRSTR && !up->notified) {
		qunlock(&up->debug);
		pprint("call to noted() when not notified\n");
		pexit("Suicide", 0);
	}
	up->notified = 0;

	nureg = up->ureg;	/* pointer to user-returned Ureg struct */

	up->fpstate &= ~FPillegal;

	/* sanity clause */
	oureg = (ulong)nureg;
	if(!okaddr((ulong)oureg-BY2WD, BY2WD+sizeof(Ureg), Read)){
		qunlock(&up->debug);
		pprint("bad ureg in noted or call to noted when not notified\n");
		pexit("Suicide", 0);
	}

	/*
	 * Check the segment selectors are all valid, otherwise
	 * a fault will be taken on attempting to return to the
	 * user process.
	 * Take care with the comparisons as different processor
	 * generations push segment descriptors in different ways.
	 */
	if((nureg->cs & 0xFFFF) != UESEL || (nureg->ss & 0xFFFF) != UDSEL
	  || (nureg->ds & 0xFFFF) != UDSEL || (nureg->es & 0xFFFF) != UDSEL
	  || (nureg->fs & 0xFFFF) != UDSEL || (nureg->gs & 0xFFFF) != UDSEL){
		qunlock(&up->debug);
		pprint("bad segment selector in noted\n");
		pexit("Suicide", 0);
	}

	/* don't let user change system flags */
	/* 0xcd5 are overflow, dir'n, sign, zero, adjust, parity, carry */
	nureg->flags = (ureg->flags & ~0xCD5) | (nureg->flags & 0xCD5);

	memmove(ureg, nureg, sizeof(Ureg));

	switch(arg0){
	case NCONT:
	case NRSTR:
    if(0) print("%s %lud: noted %.8lux %.8lux\n",
	up->text, up->pid, nureg->pc, nureg->usp);
		if(!okaddr(nureg->pc, 1, Read) ||
		   !okaddr(nureg->usp, BY2WD, Read)){
			qunlock(&up->debug);
			pprint("suicide: trap in noted\n");
			pexit("Suicide", 0);
		}
		up->ureg = (Ureg*)(*(ulong*)(oureg-BY2WD));
		qunlock(&up->debug);
		break;

	case NSAVE:
		if(!okaddr(nureg->pc, BY2WD, Read)
		|| !okaddr(nureg->usp, BY2WD, Read)){
			qunlock(&up->debug);
			pprint("suicide: trap in noted NSAVE\n");
			pexit("Suicide", 0);
		}
		qunlock(&up->debug);
		sp = oureg - Stkspace;
		splhi();
		ureg->sp = sp;
		((ulong*)sp)[1] = oureg;	/* arg 1 0(FP) is ureg* */
		((ulong*)sp)[0] = 0;		/* arg 0 is pc */
		break;

	default:
		pprint("unknown noted arg %#lux\n", arg0);
		up->lastnote.flag = NDebug;
		/* fall through */

	case NDFLT:
		if(up->lastnote.flag == NDebug){
			qunlock(&up->debug);
			pprint("suicide: %s\n", up->lastnote.msg);
		} else
			qunlock(&up->debug);
		pexit(up->lastnote.msg, up->lastnote.flag!=NDebug);
	}
}
