#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "../port/error.h"

#include "/sys/src/libc/9syscall/sys.h"
#include "linuxsystab.h"

#include <tos.h>
#include "amd64.h"
#include "ureg.h"

/* linux calling convention is callee-save. We are caller-save. 
 * But that issue is covered in trap, which saves everything. 
 * so we only need to know the calling conventions. 
 * when we call a(1,2,3,4,5,6). NO on-stack params.
	movl	$6, %r9d
	movl	$5, %r8d
	movl	$4, %r10
	movl	$3, %edx
	movl	$2, %esi
	movl	$1, %edi
 * syscall is in %ax however. 
 * return is in %ax
 */

/* this gets a little ugly. On entry, we saved r15-13 onto user stack. 
 * We need to pull them off and move them into ureg. It's very hard to
 * do this in the assembly, very easy to do it here.
 */
void
popr15r14r13(Ureg* ureg)
{
	u64int *r15r14r13 = (u64int *)ureg->sp;
	ureg->r13 = *r15r14r13++;
	ureg->r14 = *r15r14r13++;
	ureg->r15 = *r15r14r13++;
	ureg->sp = (uintptr) r15r14r13;
}
void
linuxsyscall(unsigned int, Ureg* ureg)
{
	void noted(Ureg*, uintptr);
	unsigned int scallnr;
	void notify(Ureg *);
	char *e;
	uintptr	sp;
	int s;
	Ar0 ar0;
	static Ar0 zar0;
	int i;
	uintptr linuxargs[6];

//print("linuxsyscall: wrong %d\n", wrong);
//dumpstack();
	if(!userureg(ureg))
		panic("syscall: cs %#llux\n", ureg->cs);

	cycles(&up->kentry);

	popr15r14r13(ureg);
	m->syscall++;
	up->nsyscall++;
	up->nqsyscall++;
	up->insyscall = 1;
	up->pc = ureg->ip;
	up->dbgreg = ureg;

	if(up->procctl == Proc_tracesyscall){
		up->procctl = Proc_stopme;
		procctl(up);
	}
	scallnr = ureg->ax;
#undef BREAKINCASEOFEMERGENCY
#ifdef BREAKINCASEOFEMERGENCY
	//up->attr = 0xff;
	print("(%d)# %d:", up->pid, scallnr);
	print("%#ullx %#ullx %#ullx %#ullx %#ullx %#ullx \n", 
		ureg->di,
		ureg->si,
		ureg->dx,
		ureg->r10,
		ureg->r8,
		ureg->r9);
#endif
	up->scallnr = scallnr;

	if(scallnr == 56)
		fpusysrfork(ureg);
	spllo();

	sp = ureg->sp;
	up->nerrlab = 0;
	ar0 = zar0;
	if(!waserror()){
		int printarg;
		char *name = scallnr < nlinuxsyscall ? linuxsystab[scallnr].n : "Unknown";

		linuxargs[0] = ureg->di;
		linuxargs[1] = ureg->si;
		linuxargs[2] = ureg->dx;
		linuxargs[3] = ureg->r10;
		linuxargs[4] = ureg->r8;
		linuxargs[5] = ureg->r9;

		if(scallnr >= nlinuxsyscall){
			pprint("bad linux sys call number %d(%s) pc %#ullx max %d\n",
				scallnr, name, ureg->ip, nlinuxsyscall);
			postnote(up, 1, "sys: bad sys call", NDebug);
			error(Ebadarg);
		}

		if(linuxsystab[scallnr].f == nil){
			char badsys[ERRMAX];
			pprint("bad linux sys call number %d(%s) pc %#ullx max %d\n",
				scallnr, name, ureg->ip, nlinuxsyscall);
			sprint(badsys, "linux:%d %#ullx %#ullx %#ullx %#ullx %#ullx %#ullx", 
				scallnr,
				linuxargs[0], linuxargs[1], linuxargs[2],
				linuxargs[3], linuxargs[4], linuxargs[5]);
			postnote(up, 1, badsys, NDebug);
			error(Ebadarg);
		}

		if(sp < (USTKTOP-BIGPGSZ) || sp > (USTKTOP-sizeof(up->arg)-BY2SE))
			validaddr(UINT2PTR(sp), sizeof(up->arg)+BY2SE, 0);

		up->psstate = linuxsystab[scallnr].n;

		if (up->attr & 16) {print("%d:linux: %s: pc %#p ", up->pid, linuxsystab[scallnr].n,(void *)ureg->ip);
			for(printarg = 0; printarg < linuxsystab[scallnr].narg; printarg++)
				print("%p ", (void *)linuxargs[printarg]);
			print("\n");
		}
		if (up->attr&32) dumpregs(ureg);
		linuxsystab[scallnr].f(&ar0, (va_list)linuxargs);
		if (up->attr & 64){print("AFTER: ");dumpregs(ureg);}
		poperror();
	}else{
		/* failure: save the error buffer for errstr */
		if (up->attr & 16){
			int i;
			print("Error path in linuxsyscall: %#ux, %s\n", scallnr, up->syserrstr ? up->syserrstr : "no errstr");
			for(i = 0; i < nelem(linuxargs); i++)
				print("%d: %#p\n", i, linuxargs[i]);
			dumpregs(ureg);
		}
		e = up->syserrstr;
		up->syserrstr = up->errstr;
		up->errstr = e;
		if (scallnr < nlinuxsyscall)
			ar0 = linuxsystab[scallnr].r;
		else
			ar0.p = (uintptr)-1;
	}

	/* normal amd64 kernel does not have this; remove? */
	if(up->nerrlab){
		print("bad errstack [%d]: %d extra\n", scallnr, up->nerrlab);
		for(i = 0; i < NERR; i++)
			print("sp=%#ullx pc=%#ullx\n",
				up->errlab[i].sp, up->errlab[i].pc);
		panic("error stack");
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
	if (up->attr & 16)print("%d:Ret from syscall %#ullx\n", up->pid,  (u64int) ar0.p);
	if (up->attr & 128) dumpregs(ureg);
	if(up->procctl == Proc_tracesyscall){
		up->procctl = Proc_stopme;
		s = splhi();
		procctl(up);
		splx(s);
	}else if(up->procctl == Proc_totc || up->procctl == Proc_toac)
		procctl(up);


	up->insyscall = 0;
	up->psstate = 0;

	if(scallnr == 1024)
		noted(ureg, linuxargs[0]);

	splhi();

	if(scallnr != 56 && (up->procctl || up->nnote))
		notify(ureg);
	/* if we delayed sched because we held a lock, sched now */
	if(up->delaysched){
		sched();
		splhi();
	}
	kexit(ureg);
}

void*
linuxsysexecregs(uintptr entry, ulong ssize, ulong nargs)
{
	int i;
	uvlong *l;
	Ureg *ureg;
	uintptr *sp;

	if(!up->attr)
		panic("linuxsysexecregs: up->attr %d\n", up->attr);

	/* need to figure out linux exec conventions :-( */
	sp = (uintptr*)(USTKTOP - ssize);
	*--sp = nargs;

	ureg = up->dbgreg;
	l = &ureg->bp;
	print("Starting linux proc pc %#ullx sp %p nargs %ld\n",
		ureg->ip, sp+1, nargs);

	/* set up registers for linux */
	/* we are dying in getenv. */
	/* because glibc does not follow the PPC ABI. */
	/* you have to push the env, then the args. */
	/* so to do this, well, we'll push an empty env on stack, i.e. shift
	 * the args down one. stack grows down. We already made space
	 * when we pushed nargs. 
	 */
	memmove(sp, sp+1, nargs * sizeof(*sp));
	sp[nargs] = 0;
	*--sp = nargs;
	for(i = 7; i < 16; i++)
		*l++ = 0xdeadbeef + (i*0x110);

	ureg->sp = PTR2UINT(sp);
	ureg->ip = entry;
	print("Starting linux proc pc %#ullx\n", ureg->ip);

	/*
	 */
	return UINT2PTR(nargs);
}
