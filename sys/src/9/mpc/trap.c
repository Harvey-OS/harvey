#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"io.h"
#include	"../port/error.h"

enum 
{
	Maxhandler=	32+16,		/* max number of interrupt handlers */
};

typedef struct Handler	Handler;
struct Handler
{
	void	(*r)(Ureg*, void*);
	void	*arg;
	Handler	*next;
	int	edge;
	int	nintr;
	ulong	ticks;
	int	maxtick;
};

struct
{
	Handler	*ivec[128];
	Handler	h[Maxhandler];
	int	free;
} halloc;

void	faultpower(Ureg *ur, ulong addr, int read);
void	kernfault(Ureg*, int);
void	syscall(Ureg* ureg);
void	noted(Ureg*, ulong);
void	reset(void);

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
	"i/o controller interface error",
	"reserved B",
	"system call",
	"trace trap",
	"floating point assist",
	"reserved F",
	"software emulation",
	"ITLB miss",
	"DTLB miss",
	"ITLB error",
	"DTLB error",
	"reserved 15",
	"reserved 16",
	"reserved 17",
	"reserved 18",
	"reserved 19",
	"reserved 1A",
	"reserved 1B",
	"data breakpoint",
	"instruction breakpoint",
	"peripheral breakpoint",
	"development port",
	/* the following are made up on a program exception */
	"floating point exception",		/* 20: FPEXC */
	"illegal instruction",	/* 21 */
	"privileged instruction",	/* 22 */
	"trap",	/* 23 */
	"illegal operation",	/* 24 */
	"breakpoint",	/* 25 */
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
sethvec2(int v, void (*r)(void))
{
	ulong *vp;

	vp = (ulong*)KADDR(v);
	vp[0] = (18<<26)|(PADDR(r))|2;	/* ba */
	dcflush(vp, sizeof(*vp));
}

void
trap(Ureg *ur)
{
	int ecode;
	static struct {int callsched;} c = {1};
	int user = (ur->srr1 & MSR_PR) != 0;
	char buf[ERRLEN];

	m->intrts = fastticks(nil);
	ecode = (ur->cause >> 8) & 0xff;

if(ur->status & MSR_RI == 0)
print("double fault?: ecode = %d\n", ecode);
	switch(ecode){
	case CEI:
		intr(ur);
		break;

	case CDEC:
		clockintr(ur);
		break;
	case CIMISS:
		faultpower(ur, ur->pc, 1);
		break;
	case CITLBE:
		faultpower(ur, ur->pc, 1);
		break;
	case CDMISS:
if(0) print("CDMISS: %lux %lux\n", m->epn, m->cmp1);
		faultpower(ur, m->dar, 1);
		break;
	case CDTLBE:
		faultpower(ur, m->dar, !(m->dsisr & (1<<25)));
		break;

	case CEMU:
		print("CEMU: pc=#%lux op=#%8.8lux\n", ur->pc, *(ulong*)ur->pc);
		sprint(buf, "sys: trap: illegal op addr=0x%lux", ur->pc);
		postnote(up, 1, buf, NDebug);
		break;

	case CSYSCALL:
		syscall(ur);
		break;

	case CPROG:
		if(ur->status & (1<<19))
			ecode = 0x20;
		if(ur->status & (1<<18))
			ecode = 0x21;
		if(ur->status & (1<<17))
			ecode = 0x22;
		goto Default;

	Default:
	default:
		spllo();
		print("ecode = %d\n", ecode);
		if(ecode < 0 || ecode >= 0x1F)
			ecode = 0x1F;
		print("kernel %s pc=0x%lux\n", excname[ecode], ur->pc);
		dumpregs(ur);
		dumpstack();
		delay(100);
		reset();
	}

	splhi();
	if(user)
		notify(ur);
}

void
faultpower(Ureg *ureg, ulong addr, int read)
{
	int user, insyscall, n;
	char buf[ERRLEN];
	user = (ureg->srr1 & MSR_PR) != 0;
if(0)print("fault: pid=%ld pc = %lux, addr = %lux read = %d user = %d stack=%ulx\n", up->pid, ureg->pc, addr, read, user, &ureg);
	insyscall = up->insyscall;
	up->insyscall = 1;
	spllo();
	n = fault(addr, read);
	if(n < 0){
		if(!user){
			dumpregs(ureg);
			panic("fault: 0x%lux\n", addr);
		}
print("fault: pid=%ld pc = %lux, addr = %lux read = %d user = %d stack=%ulx\n", up->pid, ureg->pc, addr, read, user, &ureg);
dumpregs(ureg);
		sprint(buf, "sys: trap: fault %s addr=0x%lux",
			read? "read" : "write", addr);
		postnote(up, 1, buf, NDebug);
	}
	up->insyscall = insyscall;
}

void
spurious(Ureg *ur, void *a)
{
	USED(a);
	print("SPURIOUS interrupt pc=0x%lux cause=0x%lux\n",
		ur->pc, ur->cause);
	panic("bad interrupt");
}

#define	LEV(n)	(((n)<<1)|1)
#define	IRQ(n)	(((n)<<1)|0)

Lock	veclock;

void
trapinit(void)
{
	int i;
	IMM *io;


	io = m->iomem;
	io->sypcr &= ~(1<<3);	/* disable watchdog (821/823) */
	io->simask = 0;	/* mask all */
	io->siel = ~0;	/* edge sensitive, wake on all */
	io->cicr = 0;	/* disable CPM interrupts */
	io->cipr = ~0;	/* clear all interrupts */
	io->cimr = 0;	/* mask all events */
	io->cicr = (0xE1<<16)|(CPIClevel<<13)|(0x1F<<8);
	io->cicr |= 1 << 7;	/* enable */
	io->tbscr = 1;	/* TBE */
	io->simask |= 1<<(31-LEV(CPIClevel));	/* CPM's level */
	eieio();
	putdec(~0);

	/*
	 * set all exceptions to trap
	 */
	for(i = 0x0; i < 0x2000; i += 0x100)
		sethvec(i, trapvec);

	//sethvec(CEI<<8, intrvec);
	sethvec2(CIMISS<<8, itlbmiss);
	sethvec2(CDMISS<<8, dtlbmiss);
	sethvec2(CDTLBE<<8, dtlberror);
}

void
intrenable(int v, void (*r)(Ureg*, void*), void *arg, int)
{
	Handler *h;
	IMM *io;

	if(halloc.free >= Maxhandler)
		panic("out of interrupt handlers");
	v -= VectorPIC;
	if(v < 0 || v >= nelem(halloc.ivec))
		panic("intrenable(%d)", v+VectorPIC);
	ilock(&veclock);
	if(v < VectorCPIC || (h = halloc.ivec[v]) == nil){
		h = &halloc.h[halloc.free++];
		h->next = halloc.ivec[v];
		halloc.ivec[v] = h;
	}
	h->r = r;
	h->arg = arg;

	/*
	 * enable corresponding interrupt in SIU/CPM
	 */

	eieio();
	io = m->iomem;
	if(v >= VectorCPIC){
		v -= VectorCPIC;
		io->cimr |= 1<<(v&0x1F);
	}
	else if(v >= VectorIRQ)
		io->simask |= 1<<(31-IRQ(v&7));
	else
		io->simask |= 1<<(31-LEV(v));
	eieio();
	iunlock(&veclock);
}

void
intr(Ureg *ur)
{
	int b, v;
	IMM *io;
	Handler *h;
	long t0;

	ur->cause &= ~0xff;
	io = m->iomem;
	b = io->sivec>>2;
	v = b>>1;
	if(b & 1) {
		if(v == CPIClevel){
			io->civr = 1;
			eieio();
			v = VectorCPIC+(io->civr>>11);
		}
	}else
		v += VectorIRQ;
	ur->cause |= v;
	h = halloc.ivec[v];
	if(h == nil){
		print("unknown interrupt %d pc=0x%lux\n", v, ur->pc);
		uartwait();
		return;
	}
	if(h->edge){
		io->sipend |= 1<<(31-b);
		eieio();
	}
	/*
	 *  call the interrupt handlers
	 */
	do {
		h->nintr++;
		t0 = getdec();
		(*h->r)(ur, h->arg);
		t0 -= getdec();
		h->ticks += t0;
		if(h->maxtick < t0)
			h->maxtick = t0;
		h = h->next;
	} while(h != nil);
	if(v >= VectorCPIC)
		io->cisr |= 1<<(v-VectorCPIC);
	eieio();
}

int
intrstats(char *buf, int bsize)
{
	Handler *h;
	int i, n;

	n = 0;
	for(i=0; i<nelem(halloc.ivec) && n < bsize; i++)
		if((h = halloc.ivec[i]) != nil && h->nintr)
			n += snprint(buf+n, bsize-n, "%3d %ud %uld %ud\n", i, h->nintr, h->ticks, h->maxtick);
	return n;
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

#define KERNPC(x)	(KTZERO<(ulong)(x)&&(ulong)(x)<(ulong)etext)

void
kernfault(Ureg *ur, int code)
{
	Label l;

	print("panic: kfault %s dar=0x%lux\n", excname[code], getdar());
	print("u=0x%lux status=0x%lux pc=0x%lux sp=0x%lux\n",
				up, ur->status, ur->pc, ur->sp);
	dumpregs(ur);
	l.sp = ur->sp;
	l.pc = ur->pc;
	dumpstack();
	for(;;)
		sched();
}

void
dumpstack(void)
{
	ulong l, v;
	int i;

	if(up == 0)
		return;
	i = 0;
	for(l=(ulong)&l; l<(ulong)(up->kstack+KSTACK); l+=4){
		v = *(ulong*)l;
		if(KTZERO < v && v < (ulong)etext){
			print("%lux=%lux, ", l, v);
			if(i++ == 4){
				print("\n");
				i = 0;
			}
		}
	}
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
//print("linkproc %ulx\n", up);
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

int
isvalid_pc(ulong pc)
{
	return KERNPC(pc) && (pc&3) == 0;
}

void
dumplongs(char*, ulong*, int)
{
}

void
reset(void)
{
	IMM *io;

	io = m->iomem;
	io->plprcrk = KEEP_ALIVE_KEY;	// unlock
	io->plprcr |= IBIT(24);		// enable checkstop reset
	putmsr(getmsr() & ~(MSR_ME|MSR_RI));
predawn = 1;
print("reset = %ulx\n", getmsr());
	delay(1000);
	// cause checkstop -> causes reset
	*(uchar*)(0xdeadbeef) = 0;
	*(ulong*)(0) = 0;
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
	return USTKTOP-BY2WD;		/* address of user-level clock */
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

/*
 *  Syscall should be called directly from assembler without going through trap().
 */
void
syscall(Ureg* ureg)
{
	ulong	sp;
	long	ret;
	int	i, scallnr;

	if((ureg->srr1 & MSR_PR) == 0)
		panic("syscall: srr1 0x%4.4uX\n", ureg->srr1);

	m->syscall++;
	up->insyscall = 1;
	up->pc = ureg->pc;
	up->dbgreg = ureg;

	scallnr = ureg->r3;
//print("scall %s lr =%lux\n", sysctab[scallnr], ureg->lr);
	up->scallnr = scallnr;
	spllo();

	sp = ureg->usp;
	up->nerrlab = 0;
	ret = -1;
	if(!waserror()){
		if(scallnr >= nsyscall){
			pprint("bad sys call number %d pc %lux\n", scallnr, ureg->pc);
			postnote(up, 1, "sys: bad sys call", NDebug);
			error(Ebadarg);
		}

		if(sp<(USTKTOP-BY2PG) || sp>(USTKTOP-sizeof(Sargs)-BY2WD))
			validaddr(sp, sizeof(Sargs)+BY2WD, 0);

		up->s = *((Sargs*)(sp+BY2WD));
		up->psstate = sysctab[scallnr];

		ret = systab[scallnr](up->s.args);
		poperror();
	}
	if(up->nerrlab){
		print("bad errstack [%d]: %d extra\n", scallnr, up->nerrlab);
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

	if(scallnr!=RFORK && (up->procctl || up->nnote)){
		splhi();
		notify(ureg);
	}
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
		if(l > ERRLEN-15)	/* " pc=0x12345678\0" */
			l = ERRLEN-15;
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
	   !okaddr(sp-ERRLEN-4*BY2WD, sizeof(Ureg)+ERRLEN+4*BY2WD, 1)) {
		pprint("suicide: bad address or sp in notify\n");
		qunlock(&up->debug);
		pexit("Suicide", 0);
	}

	memmove((Ureg*)sp, ur, sizeof(Ureg));
	*(Ureg**)(sp-BY2WD) = up->ureg;	/* word under Ureg is old up->ureg */
	up->ureg = (void*)sp;
	sp -= BY2WD+ERRLEN;
	memmove((char*)sp, up->note[0].msg, ERRLEN);
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
		sp = oureg-4*BY2WD-ERRLEN;
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
