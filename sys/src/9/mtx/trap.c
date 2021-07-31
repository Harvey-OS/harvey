#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"io.h"
#include	"tos.h"
#include	"../port/error.h"

static Lock vctllock;
static Vctl *vctl[256];

void
hwintrinit(void)
{
	i8259init();
	mpicenable(0, nil);	/* 8259 interrupts are routed through MPIC intr 0 */
}

static int
hwintrenable(Vctl *v)
{
	int vec, irq;

	irq = v->irq;
	if(BUSTYPE(v->tbdf) == BusPCI) {	/* MPIC? */
		if(irq > 15) {
			print("intrenable: pci irq %d out of range\n", v->irq);
			return -1;
		}
		vec = irq;
		mpicenable(vec, v);
	}
	else {
		if(irq > MaxIrqPIC) {
			print("intrenable: irq %d out of range\n", v->irq);
			return -1;
		}
		vec = irq+VectorPIC;
		if(i8259enable(v) == -1)
			return -1;
	}
	return vec;
}

static int
hwintrdisable(Vctl *v)
{
	int vec, irq;

	irq = v->irq;
	if(BUSTYPE(v->tbdf) == BusPCI) {	/* MPIC? */
		if(irq > 15) {
			print("intrdisable: pci irq %d out of range\n", v->irq);
			return -1;
		}
		vec = irq;
		mpicdisable(vec);
	}
	else {
		if(irq > MaxIrqPIC) {
			print("intrdisable: irq %d out of range\n", v->irq);
			return -1;
		}
		vec = irq+VectorPIC;
		if(i8259disable(irq) == -1)
			return -1;
	}
	return vec;
}

static int
hwvecno(int irq, int tbdf)
{
	if(BUSTYPE(tbdf) == BusPCI) 	/* MPIC? */
		return irq;
	else
		return irq+VectorPIC;
}

void
intrenable(int irq, void (*f)(Ureg*, void*), void* a, int tbdf, char *name)
{
	int vno;
	Vctl *v;

	if(f == nil){
		print("intrenable: nil handler for %d, tbdf 0x%uX for %s\n",
			irq, tbdf, name);
		return;
	}

	v = xalloc(sizeof(Vctl));
	v->isintr = 1;
	v->irq = irq;
	v->tbdf = tbdf;
	v->f = f;
	v->a = a;
	strncpy(v->name, name, KNAMELEN-1);
	v->name[KNAMELEN-1] = 0;

	ilock(&vctllock);
	vno = hwintrenable(v);
	if(vno == -1){
		iunlock(&vctllock);
		print("intrenable: couldn't enable irq %d, tbdf 0x%uX for %s\n",
			irq, tbdf, v->name);
		xfree(v);
		return;
	}
	if(vctl[vno]){
		if(vctl[vno]->isr != v->isr || vctl[vno]->eoi != v->eoi)
			panic("intrenable: handler: %s %s %#p %#p %#p %#p",
				vctl[vno]->name, v->name,
				vctl[vno]->isr, v->isr, vctl[vno]->eoi, v->eoi);
		v->next = vctl[vno];
	}
	vctl[vno] = v;
	iunlock(&vctllock);
}

void
intrdisable(int irq, void (*f)(Ureg *, void *), void *a, int tbdf, char *name)
{
	Vctl **pv, *v;
	int vno;

	vno = hwvecno(irq, tbdf);
	ilock(&vctllock);
	pv = &vctl[vno];
	while (*pv && 
		  ((*pv)->irq != irq || (*pv)->tbdf != tbdf || (*pv)->f != f || (*pv)->a != a ||
		   strcmp((*pv)->name, name)))
		pv = &((*pv)->next);
	assert(*pv);

	v = *pv;
	*pv = (*pv)->next;	/* Link out the entry */
	
	if(vctl[vno] == nil)
		hwintrdisable(v);
	iunlock(&vctllock);
	xfree(v);
}

void	syscall(Ureg*);
void	noted(Ureg*, ulong);
static void _dumpstack(Ureg*);

char *excname[] =
{
	"reserved 0",
	"system reset",
	"machine check",
	"data access",
	"instruction access",
	"external interrupt",
	"alignment",
	"program exception",
	"floating-point unavailable",
	"decrementer",
	"reserved A",
	"reserved B",
	"system call",
	"trace trap",
	"floating point assist",
	"reserved F",
	"reserved 10",
	"reserved 11",
	"reserved 12",
	"instruction address breakpoint",
	"system management interrupt",
};

char *fpcause[] =
{
	"inexact operation",
	"division by zero",
	"underflow",
	"overflow",
	"invalid operation",
};
char	*fpexcname(Ureg*, ulong, char*);
#define FPEXPMASK	0xfff80300		/* Floating exception bits in fpscr */


char *regname[]={
	"CAUSE",	"SRR1",
	"PC",		"GOK",
	"LR",		"CR",
	"XER",	"CTR",
	"R0",		"R1",
	"R2",		"R3",
	"R4",		"R5",
	"R6",		"R7",
	"R8",		"R9",
	"R10",	"R11",
	"R12",	"R13",
	"R14",	"R15",
	"R16",	"R17",
	"R18",	"R19",
	"R20",	"R21",
	"R22",	"R23",
	"R24",	"R25",
	"R26",	"R27",
	"R28",	"R29",
	"R30",	"R31",
};

void
trap(Ureg *ureg)
{
	ulong dsisr;
	int ecode, user;
	char buf[ERRMAX], *s;

	ecode = (ureg->cause >> 8) & 0xff;
	user = (ureg->srr1 & MSR_PR) != 0;
	if(user)
		up->dbgreg = ureg;

	if(ureg->status & MSR_RI == 0)
		print("double fault?: ecode = %d\n", ecode);

	switch(ecode) {
	case CEI:
		intr(ureg);
		break;
	case CDEC:
		clockintr(ureg);
		break;
	case CSYSCALL:
		if(!user)
			panic("syscall in kernel: srr1 0x%4.4luX", ureg->srr1);
		syscall(ureg);
		return;		/* syscall() calls notify itself, don't do it again */
	case CFPU:
		if(!user || up == nil) {
			dumpregs(ureg);
			panic("floating point in kernel");
		}
		switch(up->fpstate){
		case FPinit:
			fprestore(&initfp);
			up->fpstate = FPactive;
			break;
		case FPinactive:
			fprestore(&up->fpsave);
			up->fpstate = FPactive;
			break;
		default:
			panic("fpstate");
		}
		ureg->srr1 |= MSR_FP;
		break;
	case CISI:
		faultpower(ureg, ureg->pc, 1);
		break;
	case CDSI:
		dsisr = getdsisr();
		if(dsisr & BIT(6))
			faultpower(ureg, getdar(), 0);
		else
			faultpower(ureg, getdar(), 1);
		break;
	case CPROG:
		if(ureg->status & (1<<19))
			s = "floating point exception";
		else if(ureg->status & (1<<18))
			s = "illegal instruction";
		else if(ureg->status & (1<<17))
			s = "privileged instruction";
		else
			s = "undefined program exception";
		if(user){
			spllo();
			sprint(buf, "sys: trap: %s", s);
			postnote(up, 1, buf, NDebug);
			break;
		}
		dumpregs(ureg);
		panic(s);
		break;
	default:
		if(ecode < nelem(excname) && user){
			spllo();
			sprint(buf, "sys: trap: %s", excname[ecode]);
			postnote(up, 1, buf, NDebug);
			break;
		}
		dumpregs(ureg);
		if(ecode < nelem(excname))
			panic("%s", excname[ecode]);
		panic("unknown trap/intr: %d", ecode);
	}

	/* restoreureg must execute at high IPL */
	splhi();

	/* delaysched set because we held a lock or because our quantum ended */
	if(up && up->delaysched && ecode == CDEC){
		sched();
		splhi();
	}

	if(user) {
		notify(ureg);
		if(up->fpstate == FPinactive)
			ureg->srr1 &= ~MSR_FP;
		kexit(ureg);
	}
}

void
faultpower(Ureg *ureg, ulong addr, int read)
{
	int user, insyscall, n;
	char buf[ERRMAX];

	user = (ureg->srr1 & MSR_PR) != 0;
	insyscall = up->insyscall;
	up->insyscall = 1;
	n = fault(addr, read);
	if(n < 0){
		if(!user){
			dumpregs(ureg);
			panic("fault: 0x%lux", addr);
		}
		sprint(buf, "sys: trap: fault %s addr=0x%lux", read? "read" : "write", addr);
		postnote(up, 1, buf, NDebug);
	}
	up->insyscall = insyscall;
}

void
sethvec(int v, void (*r)(void))
{
	ulong *vp, pa, o;

	vp = KADDR(v);
	vp[0] = 0x7c1043a6;	/* MOVW R0, SPR(SPRG0) */
	vp[1] = 0x7c0802a6;	/* MOVW LR, R0 */
	vp[2] = 0x7c1243a6;	/* MOVW R0, SPR(SPRG2) */
	pa = PADDR(r);
	o = pa >> 25;
	if(o != 0 && o != 0x7F){
		/* a branch too far */
		vp[3] = (15<<26)|(pa>>16);	/* MOVW $r&~0xFFFF, R0 */
		vp[4] = (24<<26)|(pa&0xFFFF);	/* OR $r&0xFFFF, R0 */
		vp[5] = 0x7c0803a6;	/* MOVW	R0, LR */
		vp[6] = 0x4e800021;	/* BL (LR) */
	}else
		vp[3] = (18<<26)|(pa&0x3FFFFFC)|3;	/* bla */
	dcflush(vp, 8*sizeof(ulong));
}

void
trapinit(void)
{
	int i;

	/*
	 * set all exceptions to trap
	 */
	for(i = 0; i < 0x2000; i += 0x100)
		sethvec(i, trapvec);

	putmsr(getmsr() & ~MSR_IP);
}

void
intr(Ureg *ureg)
{
	int vno;
	Vctl *ctl, *v;

	vno = mpicintack();
	if(vno == 0) {			/* 8259, wired through MPIC vec 0 */
		vno = i8259intack();
		mpiceoi(0);
	}

	if(vno > nelem(vctl) || (ctl = vctl[vno]) == 0) {
		panic("spurious intr %d", vno);
		return;
	}

	if(ctl->isr)
		ctl->isr(vno);
	for(v = ctl; v != nil; v = v->next){
		if(v->f)
			v->f(ureg, v->a);
	}
	if(ctl->eoi)
		ctl->eoi(vno);

	if(up)
		preempted();
}

char*
fpexcname(Ureg *ur, ulong fpscr, char *buf)
{
	int i;
	char *s;
	ulong fppc;

	fppc = ur->pc;
	s = 0;
	fpscr >>= 3;		/* trap enable bits */
	fpscr &= (fpscr>>22);	/* anded with exceptions */
	for(i=0; i<5; i++)
		if(fpscr & (1<<i))
			s = fpcause[i];
	if(s == 0)
		return "no floating point exception";
	sprint(buf, "%s fppc=0x%lux", s, fppc);
	return buf;
}

/*
 * Fill in enough of Ureg to get a stack trace, and call a function.
 * Used by debugging interface rdb.
 */

static void
getpcsp(ulong *pc, ulong *sp)
{
	*pc = getcallerpc(&pc);
	*sp = (ulong)&pc-4;
}

void
callwithureg(void (*fn)(Ureg*))
{
	Ureg ureg;

	getpcsp((ulong*)&ureg.pc, (ulong*)&ureg.sp);
	ureg.lr = getcallerpc(&fn);
	fn(&ureg);
}

static void
_dumpstack(Ureg *ureg)
{
	ulong l, sl, el, v;
	int i;

	l = (ulong)&l;
	if(up == 0){
		el = (ulong)m+BY2PG;
		sl = el-KSTACK;
	}
	else{
		sl = (ulong)up->kstack;
		el = sl + KSTACK;
	}
	if(l > el || l < sl){
		el = (ulong)m+BY2PG;
		sl = el-KSTACK;
	}
	if(l > el || l < sl)
		return;
	print("ktrace /kernel/path %.8lux %.8lux %.8lux\n", ureg->pc, ureg->sp, ureg->lr);
	i = 0;
	for(; l < el; l += 4){
		v = *(ulong*)l;
		if(KTZERO < v && v < (ulong)etext){
			print("%.8lux=%.8lux ", l, v);
			if(i++ == 4){
				print("\n");
				i = 0;
			}
		}
	}
}

void
dumpstack(void)
{
	callwithureg(_dumpstack);
}

void
dumpregs(Ureg *ur)
{
	int i;
	ulong *l;

	if(up) {
		print("registers for %s %ld\n", up->text, up->pid);
		if(ur->srr1 & MSR_PR == 0)
		if(ur->usp < (ulong)up->kstack || ur->usp > (ulong)up->kstack+KSTACK)
			print("invalid stack ptr\n");
	}
	else
		print("registers for kernel\n");

	print("dsisr\t%.8lux\tdar\t%.8lux\n", getdsisr(), getdar());
	l = &ur->cause;
	for(i=0; i<sizeof regname/sizeof(char*); i+=2, l+=2)
		print("%s\t%.8lux\t%s\t%.8lux\n", regname[i], l[0], regname[i+1], l[1]);
}

static void
linkproc(void)
{
	spllo();
	(*up->kpfun)(up->kparg);
	pexit("", 0);
}

void
kprocchild(Proc *p, void (*func)(void*), void *arg)
{
	p->sched.pc = (ulong)linkproc;
	p->sched.sp = (ulong)p->kstack+KSTACK;

	p->kpfun = func;
	p->kparg = arg;
}

/*
 * called in sysfile.c
 */
void
evenaddr(ulong addr)
{
	if(addr & 3){
		postnote(up, 1, "sys: odd address", NDebug);
		error(Ebadarg);
	}
}

long
execregs(ulong entry, ulong ssize, ulong nargs)
{
	ulong *sp;
	Ureg *ureg;

	sp = (ulong*)(USTKTOP - ssize);
	*--sp = nargs;

	ureg = up->dbgreg;
	ureg->usp = (ulong)sp;
	ureg->pc = entry;
	ureg->srr1 &= ~MSR_FP;
	return USTKTOP-sizeof(Tos);		/* address of kernel/user shared data */
}

void
forkchild(Proc *p, Ureg *ur)
{
	Ureg *cur;

	p->sched.sp = (ulong)p->kstack+KSTACK-UREGSIZE;
	p->sched.pc = (ulong)forkret;

	cur = (Ureg*)(p->sched.sp+2*BY2WD);
	memmove(cur, ur, sizeof(Ureg));
	cur->r3 = 0;
	
	/* Things from bottom of syscall we never got to execute */
	p->psstate = 0;
	p->insyscall = 0;
}

ulong
userpc(void)
{
	Ureg *ureg;

	ureg = (Ureg*)up->dbgreg;
	return ureg->pc;
}


/* This routine must save the values of registers the user is not 
 * permitted to write from devproc and then restore the saved values 
 * before returning
 */
void
setregisters(Ureg *xp, char *pureg, char *uva, int n)
{
	ulong status;

	status = xp->status;
	memmove(pureg, uva, n);
	xp->status = status;
}

/* Give enough context in the ureg to produce a kernel stack for
 * a sleeping process
 */
void
setkernur(Ureg* ureg, Proc* p)
{
	ureg->pc = p->sched.pc;
	ureg->sp = p->sched.sp+4;
}

ulong
dbgpc(Proc *p)
{
	Ureg *ureg;

	ureg = p->dbgreg;
	if(ureg == 0)
		return 0;

	return ureg->pc;
}

/*
 *  system calls
 */
#include "../port/systab.h"

/* TODO: make this trap a separate asm entry point, like on other RISC architectures */
void
syscall(Ureg* ureg)
{
	int i;
	char *e;
	long	ret;
	ulong sp, scallnr;

	m->syscall++;
	up->insyscall = 1;
	up->pc = ureg->pc;
	up->dbgreg = ureg;

	scallnr = ureg->r3;
	up->scallnr = ureg->r3;
	spllo();

	sp = ureg->usp;
	up->nerrlab = 0;
	ret = -1;
	if(!waserror()){
		if(scallnr >= nsyscall || systab[scallnr] == nil){
			pprint("bad sys call number %lud pc %lux\n",
				scallnr, ureg->pc);
			postnote(up, 1, "sys: bad sys call", NDebug);
			error(Ebadarg);
		}

		if(sp<(USTKTOP-BY2PG) || sp>(USTKTOP-sizeof(Sargs)-BY2WD))
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
	}
	if(up->nerrlab){
		print("bad errstack [%uld]: %d extra\n", scallnr, up->nerrlab);
		print("scall %s lr =%lux\n", sysctab[scallnr], ureg->lr);
		for(i = 0; i < NERR; i++)
			print("sp=%lux pc=%lux\n", up->errlab[i].sp, up->errlab[i].pc);
		panic("error stack");
	}

	up->insyscall = 0;
	up->psstate = 0;

	/*
	 *  Put return value in frame.  On the x86 the syscall is
	 *  just another trap and the return value from syscall is
	 *  ignored.  On other machines the return value is put into
	 *  the results register by caller of syscall.
	 */
	ureg->r3 = ret;

	if(scallnr == NOTED)
		noted(ureg, *(ulong*)(sp+BY2WD));

	/* restoreureg must execute at high IPL */
	splhi();
	if(scallnr!=RFORK)
		notify(ureg);
	if(up->fpstate == FPinactive)
		ureg->srr1 &= ~MSR_FP;
}

/*
 *  Call user, if necessary, with note.
 *  Pass user the Ureg struct and the note on his stack.
 */
int
notify(Ureg* ur)
{
	int l;
	ulong s, sp;
	Note *n;

	if(up->procctl)
		procctl(up);
	if(up->nnote == 0)
		return 0;

	s = spllo();
	qlock(&up->debug);
	up->notepending = 0;
	n = &up->note[0];
	if(strncmp(n->msg, "sys:", 4) == 0){
		l = strlen(n->msg);
		if(l > ERRMAX-15)	/* " pc=0x12345678\0" */
			l = ERRMAX-15;
		sprint(n->msg+l, " pc=0x%.8lux", ur->pc);
	}

	if(n->flag!=NUser && (up->notified || up->notify==0)){
		if(n->flag == NDebug)
			pprint("suicide: %s\n", n->msg);
		qunlock(&up->debug);
		pexit(n->msg, n->flag!=NDebug);
	}

	if(up->notified) {
		qunlock(&up->debug);
		splhi();
		return 0;
	}

	if(!up->notify) {
		qunlock(&up->debug);
		pexit(n->msg, n->flag!=NDebug);
	}
	sp = ur->usp & ~(BY2V-1);
	sp -= sizeof(Ureg);

	if(!okaddr((ulong)up->notify, BY2WD, 0) ||
	   !okaddr(sp-ERRMAX-4*BY2WD, sizeof(Ureg)+ERRMAX+4*BY2WD, 1)) {
		pprint("suicide: bad address or sp in notify\n");
		qunlock(&up->debug);
		pexit("Suicide", 0);
	}

	memmove((Ureg*)sp, ur, sizeof(Ureg));
	*(Ureg**)(sp-BY2WD) = up->ureg;	/* word under Ureg is old up->ureg */
	up->ureg = (void*)sp;
	sp -= BY2WD+ERRMAX;
	memmove((char*)sp, up->note[0].msg, ERRMAX);
	sp -= 3*BY2WD;
	*(ulong*)(sp+2*BY2WD) = sp+3*BY2WD;	/* arg 2 is string */
	ur->r1 = (long)up->ureg;		/* arg 1 is ureg* */
	((ulong*)sp)[1] = (ulong)up->ureg;	/* arg 1 0(FP) is ureg* */
	((ulong*)sp)[0] = 0;			/* arg 0 is pc */
	ur->usp = sp;
	ur->pc = (ulong)up->notify;
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

	nureg = up->ureg;	/* pointer to user returned Ureg struct */

	/* sanity clause */
	oureg = (ulong)nureg;
	if(!okaddr((ulong)oureg-BY2WD, BY2WD+sizeof(Ureg), 0)){
		pprint("bad ureg in noted or call to noted when not notified\n");
		qunlock(&up->debug);
		pexit("Suicide", 0);
	}

	memmove(ureg, nureg, sizeof(Ureg));

	switch(arg0){
	case NCONT:
	case NRSTR:
		if(!okaddr(nureg->pc, 1, 0) || !okaddr(nureg->usp, BY2WD, 0)){
			pprint("suicide: trap in noted\n");
			qunlock(&up->debug);
			pexit("Suicide", 0);
		}
		up->ureg = (Ureg*)(*(ulong*)(oureg-BY2WD));
		qunlock(&up->debug);
		break;

	case NSAVE:
		if(!okaddr(nureg->pc, BY2WD, 0)
		|| !okaddr(nureg->usp, BY2WD, 0)){
			pprint("suicide: trap in noted\n");
			qunlock(&up->debug);
			pexit("Suicide", 0);
		}
		qunlock(&up->debug);
		sp = oureg-4*BY2WD-ERRMAX;
		splhi();
		ureg->sp = sp;
		((ulong*)sp)[1] = oureg;	/* arg 1 0(FP) is ureg* */
		((ulong*)sp)[0] = 0;		/* arg 0 is pc */
		break;

	default:
		pprint("unknown noted arg 0x%lux\n", arg0);
		up->lastnote.flag = NDebug;
		/* fall through */
		
	case NDFLT:
		if(up->lastnote.flag == NDebug)
			pprint("suicide: %s\n", up->lastnote.msg);
		qunlock(&up->debug);
		pexit(up->lastnote.msg, up->lastnote.flag!=NDebug);
	}
}
