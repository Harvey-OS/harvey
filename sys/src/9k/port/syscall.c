/*
 * system call entry and return.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "../port/error.h"
#include "/sys/src/libc/9syscall/sys.h"
#include <tos.h>
#include "ureg.h"

/*
 * system call tracing.  moved out of line for clarity and speed since
 * it's not in the usual code path.
 */

static void
stopme_freetrace(void)
{
	Mpl s;

	up->procctl = Proc_stopme;
	s = splhi();
	procctl(up);
	splx(s);

	if(up->syscalltrace) {
		free(up->syscalltrace);
		up->syscalltrace = nil;
	}
}

static vlong
tracecall(uintptr sp, int scallnr)
{
	/*
	 * Redundant validaddr.  Do we care?  Tracing syscalls is not exactly
	 * a fast path...  Beware, validaddr currently calls pexit rather than
	 * error if there's a problem; that might change in the future.
	 */
	if(sp < USTKTOP-PGSZ || sp > USTKTOP-sizeof(up->arg))
		validaddr((void *)sp, sizeof(up->arg), 0);

	/* last arg is a Proc.arg.arg */
	syscallfmt(scallnr, (va_list)((Sysargs *)sp)->arg);
	stopme_freetrace();
	return todget(nil);
}

static void
traceret(Ar0 *ar0p, uintptr sp, int scallnr, vlong startns)
{
	up->procctl = Proc_stopme;
	/* skip return PC at sp to produce Ureg*. */
	sysretfmt(scallnr, (va_list)SKIPSTKPC(sp), ar0p, startns,
		todget(nil));
	stopme_freetrace();
}

static void
baderrstk(int scallnr)
{
	int i;

	print("bad errstack [%d]: %d extra\n", scallnr, up->nerrlab);
	for(i = 0; i < NERR; i++)
		print("sp=%#p pc=%#p\n", up->errlab[i].sp, up->errlab[i].pc);
	panic("error stack");
}

static void
badsyscall(uint scallnr, Ureg *ureg)
{
	pprint("bad sys call number %d pc %#p\n", scallnr, ureg->pc);
	postnote(up, 1, "sys: bad sys call", NDebug);
	error(Ebadarg);
}

void
syscall(uint scallnr, Ureg* ureg, Ar0 *ar0p)
{
	char *e;
	uintptr	sp, sppc;
	vlong startns;
	Systab *scall;

	if(!userureg(ureg))
		panic("syscall: not from user mode");

	cycles(&up->kentry);

	if(0 && up->nlocks)	/* extra debugging: see qlock, kexit */
		print("syscall: nlocks %d\n", up->nlocks);

	m->syscall++;
	up->insyscall = 1;
	up->pc = ureg->pc;
	sp = ureg->sp;		/* on user-mode stack */
	up->scallnr = scallnr;
	up->dbgreg = ureg;	/* save for use in sysexecregs, sysrforkchild */
	startns = 0;

	if(up->procctl == Proc_tracesyscall)
		startns = tracecall(sp, scallnr);
	if(scallnr == RFORK)
		fpusysrfork(ureg);
	spllo();

	up->nerrlab = 0;
	scall = nil;
	if(!waserror()){
		if(scallnr >= nsyscall)
			badsyscall(scallnr, ureg);
		scall = &systab[scallnr];

		if(sp < USTKTOP - PGSZ || sp > USTKTOP - sizeof(up->arg))
			validaddr((void *)sp, sizeof(up->arg), 0);

		sppc = (uintptr)sp + sizeof(uintptr);
		if (sizeof(void *) == sizeof(vlong) &&
		   (uintptr)up->arg.arg % sizeof(vlong) != sppc % sizeof(vlong))
			print("syscall %d mis-aligned args; sp + sizeof pc "
				"%#p up->arg.arg %#p\n",
				scallnr, sppc, up->arg.arg);
		USED(sppc);
		memmove(&up->arg, (Sysargs *)sp, sizeof up->arg);
		up->psstate = scall->n;

		/*
		 * switch out to implement scall.  The return value goes into
		 * *ar0p, unless error() is called.  The cast to va_list is
		 * a crude simulation of va_start, which can't be used here.
		 */
		scall->f(ar0p, (va_list)up->arg.arg);
		/*
		 * rfork's child returns via sysrforkret from
		 * gotolabel(&up->sched), and exec on success will
		 * resume (upon return) at the new image's entry point.
		 */
		poperror();
	} else {
		/* error during system call: save the error buffer for errstr */
		e = up->syserrstr;
		up->syserrstr = up->errstr;
		up->errstr = e;
		if(DBGFLG && up->pid == 1)
			iprint("%s: syscall %s error %s\n", up->text,
				(scall? scall->n: "bogus"), up->syserrstr);
		if (scall)
			*ar0p = scall->r;
	}
	if(up->nerrlab)
		baderrstk(scallnr);

	/* propagate any Fs changes (risc-v) back to user mode */
	fpsts2ureg(ureg);

	if(up->procctl == Proc_tracesyscall)
		traceret(ar0p, sp, scallnr, startns);

	up->insyscall = 0;
	up->psstate = nil;

	if(scallnr == NOTED)
		/*
		 * second arg is continuation style (e.g., NCONT, NDFLT).
		 * noted advances ureg->pc if needed.
		 */
		noted(ureg, ((int *)up->arg.arg)[0]);
	splhi();
	if(scallnr != RFORK && (up->procctl || up->nnote))
		notify(ureg);		/* deliver any pending note */

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
	int frame;

	frame = argc * sizeof(char*);
	return STACKALIGN(stack - frame) + frame;
}

void*
sysexecregs(uintptr entry, ulong ssize, ulong nargs)
{
	uintptr *sp;
	Ureg *ureg;

	/* drop below args+Tos */
	sp = (uintptr*)(USTKTOP - ssize);
	*--sp = nargs;				/* push argc */

	/* set new program's PC and SP in ureg, for strap to restore */
	ureg = up->dbgreg;	/* saved at start of syscall(); on up->kstack */
	ureg->sp = PTR2UINT(sp);	/* sp is on user-mode stack */
	ureg->pc = entry;
	ureg->type = SYSCALLTYPE;	/* fiction for acid */

	/*
	 * return the address of kernel/user shared data (Tos)
	 * (e.g. clock stuff).  this becomes the argument to _main
	 * in libc's main9.s.
	 */
	return (void *)TOS(USTKTOP);
}

void
sysprocsetup(Proc* p)
{
	fpusysprocsetup(p);
}
