/*
 * power pc 440 traps
 */
#include "include.h"

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

void	intr(Ureg *ur);

static int probing, trapped;
static jmp_buf probenv;

static void
sethvec(uintptr v, void (*r)(void))
{
	u32int *vp;
	ulong o, ra;
	long d;

	vp = UINT2PTR(v);
	vp[0] = 0x7c1043a6;			/* MOVW R0, SPR(SPRG0) */
	vp[1] = 0x7c0802a6;			/* MOVW LR, R0 */
	vp[2] = 0x7c1243a6;			/* MOVW R0, SPR(SPRG2) */
	d = (uchar*)r - (uchar*)&vp[3];
	o = (ulong)d >> 25;
	if(o != 0 && o != 0x7F){
		/* a branch too far: running from ROM */
		ra = (ulong)r;
		vp[3] = (15<<26)|(ra>>16);	/* MOVW $r&~0xFFFF, R0 */
		vp[4] = (24<<26)|(ra&0xFFFF);	/* OR $r&0xFFFF, R0 */
		vp[5] = 0x7c0803a6;		/* MOVW	R0, LR */
		vp[6] = 0x4e800021;		/* BL (LR) */
	}
	else
		vp[3] = (18<<26)|(d&0x3FFFFFC)|1;	/* BL (relative) */
	dcflush(PTR2UINT(vp), 8*sizeof(u32int));
}

static void
sethvec2(uintptr v, void (*r)(void))
{
	u32int *vp;
	long d;

	vp = UINT2PTR(v);
	d = (uchar*)r - (uchar*)&vp[0];
	vp[0] = (18<<26)|(d & 0x3FFFFFC);	/* B (relative) */
	dcflush(PTR2UINT(vp), sizeof(*vp));
}

extern uintptr vectorbase;

void
trapinit(void)
{
	uintptr e, s;

	putesr(0);				/* clears machine check */
	coherence();

	/*
	 * set all exceptions to trap
	 */
	s = vectorbase + 0x100;		/* Mach is at vectorbase */
	/* 0x2000 is last documented 405 vector */
	for(e = s+0x2100; s < e; s += 0x100)
		sethvec(s, trapvec);
	coherence();

	/*
	 * set exception handlers
	 */
//	sethvec(vectorbase + INT_RESET, trapcritvec);
//	sethvec(vectorbase + INT_MCHECK, trapcritvec);
	sethvec(vectorbase + INT_DSI, trapvec);
	sethvec(vectorbase + INT_ISI, trapvec);
	sethvec(vectorbase + INT_EI, trapvec); /* external non-critical intrs */
	sethvec(vectorbase + INT_ALIGN, trapvec);
	sethvec(vectorbase + INT_PROG, trapvec);
	sethvec(vectorbase + INT_FPU, trapvec);
	sethvec(vectorbase + INT_SYSCALL, trapvec);
	sethvec(vectorbase + INT_APU, trapvec);
	sethvec(vectorbase + INT_PIT, trapvec);
	sethvec(vectorbase + INT_FIT, trapvec);
//	sethvec(vectorbase + INT_WDT, trapcritvec);
//	sethvec2(vectorbase + INT_DMISS, dtlbmiss);
//	sethvec2(vectorbase + INT_IMISS, itlbmiss);
//	sethvec(vectorbase + INT_DEBUG, trapcritvec);
	syncall();

	/* l.s already set EVPR */
	putmsr(getmsr() | MSR_CE | MSR_ME);	/* no MSR_EE here */
	coherence();
}

void
trapdisable(void)
{
	putmsr(getmsr() & ~( MSR_EE | MSR_CE | MSR_ME)); /* MSR_DE as well? */
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

void
dumpstack(void)
{
	/* sorry.  stack is in dram, not Mach now */
}

void
dumpregs(Ureg *ureg)
{
	int i;
	uintptr *l;

	iprint("cpu%d: registers for boot\n", m->machno);

	l = &ureg->cause;
	for(i = 0; i < nelem(regname); i += 2, l += 2)
		iprint("%s\t%.8p\t%s\t%.8p\n", regname[i], l[0], regname[i+1], l[1]);
}

static void
faultpower(Ureg *ureg, ulong addr, int read)
{
	USED(read);
	dumpregs(ureg);
	panic("boot fault: 0x%lux", addr);
}

void
trap(Ureg *ur)
{
	int ecode;
	u32int esr;

	ur->cause &= 0xFFE0;
	ecode = ur->cause;
	if(ecode < 0 || ecode >= 0x2000)
		ecode = 0x3000;
	esr = getesr();
	putesr(0);

	switch(ecode){
	case INT_SYSCALL:
		panic("syscall in boot: srr1 %#4.4luX pc %#p\n",
			ur->srr1, ur->pc);

	case INT_PIT:
		clockintr(ur);
		break;

	case INT_MCHECK:
		if (probing) {
			trapped++;
			longjmp(probenv, 1);
		}
		if(esr & ESR_MCI){
			//iprint("mcheck-mci %lux\n", ur->pc);
			faultpower(ur, ur->pc, 1);
			break;
		}
		//iprint("mcheck %#lux esr=%#ux dear=%#ux\n", ur->pc, esr, getdear());
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
		intr(ur);
		break;

	case INT_PROG:
	default:
		print("boot %s pc=%#lux\n", excname(ecode, esr), ur->pc);
		dumpregs(ur);
		dumpstack();
		if(m->machno == 0)
			spllo();
		exit(1);
	}
	splhi();
}

static Lock fltlck;

vlong
probeaddr(uintptr addr)
{
	vlong v;

	ilock(&fltlck);
	probing = 1;
	/*
	 * using setjmp & longjmp is a sleazy hack; see ../../9k/vt4/trap.c
	 * for a less sleazy hack.
	 */
	if (setjmp(probenv) == 0)
		v = *(ulong *)addr;	/* this may cause a fault */
	else
		v = -1;
	probing = 0;
	iunlock(&fltlck);
	return v;
}

vlong
qtmprobeaddr(uintptr addr)
{
	int i;
	vlong v, junk;
	vlong *ptr;

	ilock(&fltlck);
	probing = 1;
	trapped = 0;
	v = 0;
	/*
	 * using setjmp & longjmp is a sleazy hack; see ../../9k/vt4/trap.c
	 * for a less sleazy hack.
	 */
	if (setjmp(probenv) == 0) {
		syncall();
		clrmchk();
		syncall();

		/* write entire cache lines, unless we get a bus error */
		ptr = (vlong *)(addr & ~(DCACHELINESZ - 1));
		for (i = 0; !trapped && !(getesr() & ESR_MCI) &&
		    i < DCACHELINESZ/sizeof *ptr; i++) {
			ptr[i] = 0;
			coherence();
		}
		junk = ptr[0];
		USED(junk);
	} else
		v = -1;

	syncall();
	if(getesr() & ESR_MCI)
		v = -1;
	syncall();
	clrmchk();
	syncall();
	probing = 0;
	iunlock(&fltlck);
	return v;
}
