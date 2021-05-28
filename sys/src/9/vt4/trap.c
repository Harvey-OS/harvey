#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

#include	<tos.h>
#include	"ureg.h"

#include	"io.h"

enum {
	VECSIZE = 0x100,
	VECBASE = PHYSSRAM,
};

static struct {
	ulong off;
	char *name;
} intcause[] = {
	{ INT_RESET,	"system reset" },
	{ INT_MCHECK,	"machine check" },
	{ INT_DSI,	"data access" },
	{ INT_ISI,	"instruction access" },
	{ INT_EI,	"external interrupt" },
	{ INT_ALIGN,	"alignment" },
	{ INT_PROG,	"program exception" },
	{ INT_FPU,	"floating-point unavailable" },
	{ INT_DEC,	"decrementer" },
	{ INT_SYSCALL,	"system call" },
	{ INT_TRACE,	"trace trap" },
	{ INT_FPA,	"floating point unavailable" },
	{ INT_APU,	"auxiliary processor unavailable" },
	{ INT_PIT,	"programmable interval timer interrrupt" },
	{ INT_FIT,	"fixed interval timer interrupt" },
	{ INT_WDT,	"watch dog timer interrupt" },
	{ INT_DMISS,	"data TLB miss" },
	{ INT_IMISS,	"instruction TLB miss" },
	{ INT_DEBUG,	"debug interrupt" },
	{ INT_DEBUG+VECSIZE, "system reset" },
	{ 0,		"unknown interrupt" }
};

static char *excname(ulong, u32int);

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

static int probing, trapped;

/* populate a normal vector */
static void
sethvec(int v, void (*r)(void))
{
	ulong *vp, pa, o;

	vp = (ulong*)KADDR(v);
	vp[0] = 0x7c1043a6;			/* MOVW R0, SPR(SPRG0) */
	vp[1] = 0x7c0802a6;			/* MOVW LR, R0 */
	vp[2] = 0x7c1243a6;			/* MOVW R0, SPR(SPRG2) */
	pa = PADDR(r);
	o = pa >> 25;
	if(o != 0 && o != 0x7F){
		/* a branch too far: running from ROM */
		vp[3] = (15<<26)|(pa>>16);	/* MOVW $r&~0xFFFF, R0 */
		vp[4] = (24<<26)|(pa&0xFFFF);	/* OR $r&0xFFFF, R0 */
		vp[5] = 0x7c0803a6;		/* MOVW	R0, LR */
		vp[6] = 0x4e800021;		/* BL (LR) */
		if (v == VECBASE + INT_PIT || v == VECBASE + INT_FIT) {
			uartlputc('?');
			uartlputc('v');
			print("branch too far for vector %#x!\n", v);
		}
	}else
		vp[3] = (18<<26)|(pa&0x3FFFFFC)|3;	/* bla */
	dcflush(PTR2UINT(vp), 8*BY2WD);
}

/* populate a tlb-miss vector */
static void
sethvec2(int v, void (*r)(void))
{
	ulong *vp;
	char msg[128];

	if (((ulong)r & ~KSEGM) >= (1<<26)) {
		uartlputc('?');
		uartlputc('t');
		snprint(msg, sizeof msg,
			"tlb miss handler address %#p too high\n", r);
		uartlputs(msg);
	}
	vp = (ulong*)KADDR(v);
	vp[0] = (18<<26)|((ulong)r&~KSEGM)|2;	/* ba */
	dcflush(PTR2UINT(vp), sizeof *vp);
}

static void
faultpower(Ureg *ureg, ulong addr, int read)
{
	int user, insyscall, n;
	char buf[ERRMAX];

	user = (ureg->srr1 & MSR_PR) != 0;
	if(!user){
//		if(vmapsync(addr))
//			return;
//		if(addr >= USTKTOP)
//			panic("kernel fault: bad address pc=%.8#lux addr=%.8#lux",
//				ureg->pc, addr);
		if(up == nil)
			panic("kernel fault: no user process pc=%.8#lux addr=%.8#lux",
				ureg->pc, addr);
	}
	if(up == nil)
		panic("user fault: up=0 pc=%.8#lux addr=%.8#lux", ureg->pc, addr);
	insyscall = 0;
	if (up) {
		insyscall = up->insyscall;
		up->insyscall = 1;
	}
	n = fault(addr, read);		/* repair user-mode fault */
	if(n < 0){
		if(!user){
			dumpregs(ureg);
			panic("fault: addr %#lux", addr);
		}else if(0)
			dumpregs(ureg);
		seprint(buf, buf + sizeof buf, "sys: trap: fault %s addr=%#lux",
			read? "read" : "write", addr);
		postnote(up, 1, buf, NDebug);
	}
	if (up)
		up->insyscall = insyscall;
}

static void
setlights(int user)
{
	if (up == nil)
		lightstate(Ledidle);
	else
		lightstate(user == 0? Ledkern: Leduser);
}

void
kexit(Ureg*)
{
	uvlong t;
	Tos *tos;

	/* precise time accounting, kernel exit */
	tos = (Tos*)(USTKTOP-sizeof(Tos));
	cycles(&t);
	tos->kcycles += t - up->kentry;
	tos->pcycles = up->pcycles;
	tos->pid = up->pid;
// surely only need to set tos->pid on rfork and exec?
}

void
trap(Ureg *ur)
{
	int ecode, user, v;
	u32int esr;
	char buf[ERRMAX];

	if (ur == nil)
		panic("trap: nil ur");
	v = ur->cause;
	lightstate(Ledtrap);
	ur->cause &= 0xFFE0;
	ecode = ur->cause;
	esr = getesr();
	clrmchk();

	user = (ur->srr1 & MSR_PR) != 0;
	if(user){
		cycles(&up->kentry);
		up->dbgreg = ur;
	}
	switch(ecode){
	case INT_SYSCALL:
		if(!user)
			panic("syscall in kernel: srr1 %#4.4luX pc %#p",
				ur->srr1, ur->pc);
		syscall(ur);
		setlights(user);
		return;		/* syscall() calls notify itself */

	case INT_PIT:
		m->intr++;
		clockintr(ur);
		break;

	case INT_WDT:
		puttsr(~0);
		panic("watchdog timer went off at pc %#lux", ur->pc);
		break;

	case INT_MCHECK:
		if (probing && !user) {
			if (trapped++ > 0)
				panic("trap: recursive probe");
			break;		/* continue at next instruction */
		}
		if(esr & ESR_MCI){
			iprint("mcheck-mci %lux\n", ur->pc);
			faultpower(ur, ur->pc, 1);
			break;
		}
		iprint("mcheck %#lux esr=%#ux dear=%#ux\n", ur->pc, esr, getdear());
		ur->pc -= 4;	/* back up to faulting instruction */
		/* fall through */
	case INT_DSI:
	case INT_DMISS:
		faultpower(ur, getdear(), !(esr&ESR_DST));
		break;

	case INT_ISI:
	case INT_IMISS:
		faultpower(ur, ur->pc, 1);
		break;

	case INT_EI:
		m->intr++;
		intr(ur);
		break;

	case 0:
		puttsr(~0);
		if (v == 0)
			panic("watchdog reset? probable jump via "
				"zeroed pointer; pc %#lux lr %#lux",
				ur->pc, ur->lr);
		else
			panic("watchdog reset? interrupt at vector zero; "
				"pc %#lux lr %#lux", ur->pc, ur->lr);
		break;

	case INT_DEBUG + VECSIZE:
		panic("reset");
		break;

	case INT_PROG:
		if(esr & ESR_PIL && user){
			if(fpuemu(ur))
				break;
			/* otherwise it's an illegal instruction */
		}
		/* fall through */
	default:
		if(user){
			spllo();
			sprint(buf, "sys: trap: %s", excname(ecode, esr));
			postnote(up, 1, buf, NDebug);
			break;
		}
		splhi();
		print("kernel %s; vector=%#ux pc=%#lux\n",
			excname(ecode, esr), ecode, ur->pc);
		if (ecode == 0)
			print("probable jump via zeroed pointer; pc %#lux lr %#lux\n",
				ur->pc, ur->lr);
		dumpregs(ur);
		dumpstack();
		if(m->machno == 0)
			spllo();
		exit(1);
	}
	splhi();
	setlights(user);

	/* delaysched set because we held a lock or because our quantum ended */
	if(up && up->delaysched && ecode == INT_PIT){
		sched();
		splhi();
		setlights(user);
	}

	if(user){
		if(up->procctl || up->nnote)
			notify(ur);
		kexit(ur);
	}
}

void
trapinit(void)
{
	int i;

	clrmchk();
	intrinit();

	/*
	 * set all exceptions to trap by default
	 */
	for(i = 0; i < INT_DEBUG + VECSIZE; i += VECSIZE)
		sethvec(VECBASE + i, trapvec);

	/*
	 * set exception handlers
	 */
	sethvec(VECBASE + INT_RESET, trapcritvec);
	sethvec(VECBASE + INT_MCHECK, trapcritvec);
	sethvec(VECBASE + INT_DSI, trapvec);
	sethvec(VECBASE + INT_ISI, trapvec);
	sethvec(VECBASE + INT_EI, trapvec);
	sethvec(VECBASE + INT_ALIGN, trapvec);
	sethvec(VECBASE + INT_PROG, trapvec);
	sethvec(VECBASE + INT_FPU, trapvec);
	sethvec(VECBASE + INT_SYSCALL, trapvec);
	sethvec(VECBASE + INT_APU, trapvec);
	sethvec(VECBASE + INT_PIT, trapvec);
	sethvec(VECBASE + INT_FIT, trapvec);
	sethvec(VECBASE + INT_WDT, trapcritvec);
	sethvec2(VECBASE + INT_DMISS, dtlbmiss);
	sethvec2(VECBASE + INT_IMISS, itlbmiss);
	sethvec(VECBASE + INT_DEBUG, trapcritvec);
	/*
	 * the last word of sram (0xfffffffc) contains a branch
	 * to the start of sram beyond the vectors (0xfffe2100),
	 * which initially will be the start of our bootstrap loader.
	 * overwrite it so that we can get control if the machine should
	 * reset.
	 */
	sethvec(VECBASE + INT_DEBUG + VECSIZE, trapcritvec);
	sethvec(0, trapcritvec);
	sync();

	putevpr(VECBASE);
	sync();

	putmsr(getmsr() | MSR_ME);
	sync();
}

static char*
excname(ulong ivoff, u32int esr)
{
	int i;

	if(ivoff == INT_PROG){
		if(esr & ESR_PIL)
			return "illegal instruction";
		if(esr & ESR_PPR)
			return "privileged";
		if(esr & ESR_PTR)
			return "trap with successful compare";
		if(esr & ESR_PEU)
			return "unimplemented APU/FPU";
		if(esr & ESR_PAP)
			return "APU exception";
		if(esr & ESR_U0F)
			return "data storage: u0 fault";
	}
	for(i=0; intcause[i].off != 0; i++)
		if(intcause[i].off == ivoff)
			break;
	return intcause[i].name;
}

/*
 * Fill in enough of Ureg to get a stack trace, and call a function.
 * Used by debugging interface rdb.
 */
void
callwithureg(void (*fn)(Ureg*))
{
	Ureg ureg;

	ureg.pc = getcallerpc(&fn);
	ureg.sp = PTR2UINT(&fn);
	fn(&ureg);
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
			iprint("%lux=%lux, ", l, v);
			if(i++ == 4){
				iprint("\n");
				i = 0;
			}
		}
	}
}

void
dumpregs(Ureg *ureg)
{
	int i;
	uintptr *l;

	splhi();		/* prevent recursive dumps */
	if(up != nil)
		iprint("cpu%d: registers for %s %ld\n",
			m->machno, up->text, up->pid);
	else
		iprint("cpu%d: registers for kernel\n", m->machno);

	if (ureg == nil) {
		iprint("nil ureg, no dump\n");
		return;
	}
	l = &ureg->cause;
	for(i = 0; i < nelem(regname); i += 2, l += 2)
		iprint("%s\t%.8p\t%s\t%.8p\n", regname[i], l[0], regname[i+1], l[1]);
}

static void
linkproc(void)
{
	spllo();
	up->kpfun(up->kparg);
	pexit("", 0);
}

void
kprocchild(Proc* p, void (*func)(void*), void* arg)
{
	p->sched.pc = PTR2UINT(linkproc);
	p->sched.sp = PTR2UINT(p->kstack+KSTACK);
	p->sched.sp = STACKALIGN(p->sched.sp);

	p->kpfun = func;
	p->kparg = arg;
}

uintptr
userpc(void)
{
	Ureg *ureg = up->dbgreg;
	return ureg->pc;
}

/*
 * This routine must save the values of registers the user is not
 * permitted to write from devproc and then restore the saved values
 * before returning
 */
void
setregisters(Ureg *ureg, char *pureg, char *uva, int n)
{
	u32int status;

	status = ureg->status;
	memmove(pureg, uva, n);
	ureg->status = status;
}

/*
 * Give enough context in the ureg to produce a kernel stack for
 * a sleeping process
 */
void
setkernur(Ureg* ureg, Proc* p)
{
	ureg->pc = p->sched.pc;
	ureg->sp = p->sched.sp+BY2SE;
}

uintptr
dbgpc(Proc* p)
{
	Ureg *ureg;

	ureg = p->dbgreg;
	if(ureg == 0)
		return 0;
	return ureg->pc;
}

vlong
probeaddr(uintptr addr)
{
	vlong v;
	static Lock fltlck;

	ilock(&fltlck);
	trapped = 0;
	probing = 1;
	barriers();

	v = *(ulong *)addr;	/* this may cause a fault */
	USED(probing);
	barriers();

	probing = 0;
	barriers();
	if (trapped)
		v = -1;
	iunlock(&fltlck);
	return v;
}
