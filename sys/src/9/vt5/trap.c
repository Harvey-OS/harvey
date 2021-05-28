/* ppc440 traps */
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
};

extern int notify(Ureg*);
extern void syscall(Ureg*);

static struct {
	ulong off;
	char *name;
} intcause[] = {
	{ INT_CI,	"critical input" },
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
	"CAUSE","SRR1",
	"PC",	"GOK",
	"LR",	"CR",
	"XER",	"CTR",
	"R0",	"R1",
	"R2",	"R3",
	"R4",	"R5",
	"R6",	"R7",
	"R8",	"R9",
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

void	init0(void);

#define	SPR(v)	(((v&0x1f)<<16) | (((v>>5)&0x1f)<<11))

#define SPR_SPRG0	0x110		/* SPR General 0 */
#define SPR_SPRG2	0x112		/* SPR General 2 */
#define SPR_SPRG6W	0x116		/* SPR General 6; supervisor W  */

static void
vecgen(int v, void (*r)(void), int r0spr, int maskce)
{
	u32int *vp, o, ra;
	int i, d;

	vp = (u32int*)KADDR(v);
	vp[0] = 0x7c0003a6 | SPR(r0spr);	/* MOVW R0, SPR(r0spr) */
	i = 1;
	if(maskce){			/* clear CE?  this stops crit. intrs. */
		vp[i++] = 0x7c0000a6;	/* MOVW   MSR, R0 */
		vp[i++] = 0x540003da;	/* RLWNM $0, R0, ~MSR_CE, R0 */
		vp[i++] = 0x7c000124;	/* MOVW	 R0, MSR */
	}
	vp[i++] = 0x7c0802a6;			/* MOVW LR, R0 */
	vp[i++] = 0x7c0003a6 | SPR(SPR_SPRG2);	/* MOVW R0, SPR(SPRG2) */
	d = (uchar*)r - (uchar*)&vp[i];
	o = (u32int)d >> 25;
	if(o != 0 && o != 0x7F){
		/* a branch too far: running from ROM */
		ra = (u32int)r;
		vp[i++] = (15<<26)|(ra>>16);	/* MOVW $r&~0xFFFF, R0 */
		vp[i++] = (24<<26)|(ra&0xFFFF);	/* OR $r&0xFFFF, R0 */
		vp[i++] = 0x7c0803a6;		/* MOVW	R0, LR */
		vp[i++] = 0x4e800021;		/* BL (LR) */
	}else
		vp[i++] = (18<<26)|(d&0x3FFFFFC)|1;	/* bl (relative) */
	dcflush(PTR2UINT(vp), i*sizeof(u32int));
}

/* populate a normal vector */
static void
sethvec(int v, void (*r)(void))
{
	vecgen(v, r, SPR_SPRG0, 1);
}

/* populate a tlb-miss vector */
static void
sethvec2(int v, void (*r)(void))
{
	ulong *vp;
	long d;

	vp = (ulong*)KADDR(v);
	d = (uchar*)r - (uchar*)&vp[0];
	if (d >= (1<<26)) {
		uartlputc('?');
iprint("sethvec2: v %#x vp %#p r %#p d %ld\n", v, vp, r, d);
		iprint("tlb miss handler address too high\n");
	}
	vp[0] = (18<<26)|(d & 0x3FFFFFC);	/* b (relative) */
	dcflush(PTR2UINT(vp), sizeof *vp);
}

static void
faultpower(Ureg *ureg, ulong addr, int read)
{
	int user, insyscall;
	char buf[ERRMAX];

	/*
	 * There must be a user context.
	 * If not, the usual problem is causing a fault during
	 * initialisation before the system is fully up.
	 */
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
	insyscall = up->insyscall;
	up->insyscall = 1;
	if(fault(addr, read) < 0){
		/*
		 * It is possible to get here with !user if, for example,
		 * a process was in a system call accessing a shared
		 * segment but was preempted by another process which shrunk
		 * or deallocated the shared segment; when the original
		 * process resumes it may fault while in kernel mode.
		 * No need to panic this case, post a note to the process
		 * and unwind the error stack. There must be an error stack
		 * (up->nerrlab != 0) if this is a system call, if not then
		 * the game's a bogey.
		 */
		if(!user && (!insyscall || up->nerrlab == 0)){
			dumpregs(ureg);
			panic("fault: %#lux", addr);
		}
		sprint(buf, "sys: trap: fault %s addr=%#lux",
			read? "read": "write", addr);
		postnote(up, 1, buf, NDebug);
		if(insyscall)
			error(buf);
	}
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
syncall(void)
{
	sync();
	isync();
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
	u32int esr, mcsr;
	char buf[ERRMAX];

	if (ur == nil)
		panic("trap: nil ur");
	v = ur->cause;
	ur->cause &= 0xFFE0;
	ecode = ur->cause;

	esr = getesr();
	mcsr = getmcsr();
	clrmchk();

	lightstate(Ledtrap);
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
				panic("trap: recursive probe on mcheck");
			break;		/* continue at next instruction */
		}
		if(esr & ESR_MCI){
			iprint("mcheck-mci %lux\n", ur->pc);
			faultpower(ur, ur->pc, 1);
			break;
		}
		iprint("mcheck %#lux esr=%#ux mcsr=%#ux dear=%#ux\n",
			ur->pc, esr, mcsr, getdear());
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

	case INT_CI:
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

	case INT_DEBUG:
		putdbsr(~0);		/* extinguish source */
		print("debug interrupt at pc %#lux\n", ur->pc);
		break;

	case INT_DEBUG + VECSIZE:
		panic("reset");
		break;

	case INT_FPA:
	case INT_FPU:
		if(fpuavail(ur))
			break;
		esr |= ESR_PFP;
		/* fall through */
	case INT_PROG:
		if(esr & ESR_PFP){	/* floating-point enabled exception */
			fputrap(ur, user);
			break;
		}
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
			if(ecode == INT_ALIGN)
				sprint(buf+strlen(buf), " ea=%#ux", getdear());
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
	vecgen(VECBASE + INT_CI, critintrvec, SPR_SPRG6W, 0);
	sethvec(VECBASE + INT_MCHECK, trapmvec);
	sethvec(VECBASE + INT_DSI, trapvec);
	sethvec(VECBASE + INT_ISI, trapvec);
	sethvec(VECBASE + INT_EI, trapvec);
	sethvec(VECBASE + INT_ALIGN, trapvec);
	sethvec(VECBASE + INT_PROG, trapvec);
	sethvec(VECBASE + INT_FPU, trapvec);
	sethvec(VECBASE + INT_DEC, trapvec);
	sethvec(VECBASE + INT_SYSCALL, trapvec);
	sethvec(VECBASE + INT_TRACE, trapvec);
	sethvec(VECBASE + INT_FPA, trapvec);
	sethvec(VECBASE + INT_APU, trapvec);
	sethvec(VECBASE + INT_PIT, trapvec);
//	sethvec(VECBASE + INT_FIT, trapvec);
	vecgen(VECBASE + INT_WDT, critintrvec, SPR_SPRG6W, 0);
	sethvec2(VECBASE + INT_DMISS, dtlbmiss);
	sethvec2(VECBASE + INT_IMISS, itlbmiss);
	vecgen(VECBASE + INT_DEBUG, critintrvec, SPR_SPRG6W, 0);
	sync();

	putevpr(KZERO | VECBASE);
	sync();

	putmsr(getmsr() | MSR_ME | MSR_DE);
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

static Lock mchklck;	/* only allow one probe or poke at a time */

vlong
probeaddr(uintptr addr)
{
	vlong v;

	ilock(&mchklck);
	trapped = 0;
	probing = 1;

	syncall();
	putmsr(getmsr() & ~MSR_ME);	/* disable machine check traps */
	syncall();
	clrmchk();
	syncall();

	if (getesr() & ESR_MCI)		/* machine check happened? */
		iprint("probeaddr: mcheck before probe\n");
	v = *(ulong *)addr;		/* this may cause a machine check */
	syncall();

	if (getesr() & ESR_MCI)		/* machine check happened? */
		trapped = 1;
	syncall();
	clrmchk();
	syncall();
	putmsr(getmsr() | MSR_ME);	/* enable machine check traps */
	syncall();

	probing = 0;
	if (trapped)
		v = -1;
//iprint("probeaddr: trapped=%d for addr %lux\n", trapped, addr);
	iunlock(&mchklck);
	return v;
}

vlong
pokeaddr(uintptr addr, uint v1, uint v2)
{
	vlong v;
	ulong *p;

//iprint("probing %#lux...", addr);
	ilock(&mchklck);
	trapped = 0;
	probing = 1;

	syncall();
	putmsr(getmsr() & ~MSR_ME);	/* disable machine check traps */
	syncall();
	clrmchk();
	syncall();

	if (getesr() & ESR_MCI)		/* machine check happened? */
		iprint("pokeaddr: mcheck before probe\n");

	p = (ulong *)addr;
	*p = v1;
	syncall();
	v = *p;				/* this may cause a machine check */
	syncall();
	if (getesr() & ESR_MCI || v != v1)	/* machine check happened? */
		trapped = 1;

	*p = v2;
	syncall();
	v = *p;				/* this may cause a machine check */
	syncall();
	if (getesr() & ESR_MCI || v != v2)	/* machine check happened? */
		trapped = 1;

	syncall();
	clrmchk();
	syncall();
	putmsr(getmsr() | MSR_ME);	/* enable machine check traps */
	syncall();

	probing = 0;
	if (trapped)
		v = -1;
//iprint("pokeaddr: trapped=%d for addr %lux\n", trapped, addr);
	iunlock(&mchklck);
	return v;
}
