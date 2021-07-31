#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"io.h"
#include	"../port/error.h"

void	noted(Ureg*, Ureg**, ulong);
void	rfnote(Ureg**);
void	kernfault(Ureg*, int);

char *excname[] =
{
	"trap: external interrupt",
	"trap: TLB modification",
	"trap: TLB miss (load or fetch)",
	"trap: TLB miss (store)",
	"trap: address error (load or fetch)",
	"trap: address error (store)",
	"trap: bus error (fetch)",
	"trap: bus error (data load or store)",
	"trap: system call",
	"breakpoint",
	"trap: reserved instruction",
	"trap: coprocessor unusable",
	"trap: arithmetic overflow",
	"trap: TRAP exception",
	"trap: VCE (instruction)",
	"trap: floating-point exception",
	"trap: undefined 16",	/* used as sys call for debugger */
	"trap: undefined 17",
	"trap: undefined 18",
	"trap: undefined 19",
	"trap: undefined 20",
	"trap: undefined 21",
	"trap: undefined 22",
	"trap: WATCH exception",
	"trap: undefined 24",
	"trap: undefined 25",
	"trap: undefined 26",
	"trap: undefined 27",
	"trap: undefined 28",
	"trap: undefined 29",
	"trap: undefined 30",
	"trap: VCE (data)",
};

char *fpcause[] =
{
	"inexact operation",
	"underflow",
	"overflow",
	"division by zero",
	"invalid operation",
};
char	*fpexcname(Ureg*, ulong, char*);
#define FPEXPMASK	(0x3f<<12)		/* Floating exception bits in fcr31 */
void (*eisadma[8])(void);			/* Eisa dma chain vectors */


struct
{
	char	*name;
	int	off;
} regname[]={
	"STATUS",	Ureg_status,
	"PC",		Ureg_pc,
	"SP",		Ureg_sp,
	"CAUSE",	Ureg_cause,
	"BADADDR",	Ureg_badvaddr,
	"TLBVIRT",	Ureg_tlbvirt,
	"HI",		Ureg_hi,
	"LO",		Ureg_lo,
	"R31",		Ureg_r31,
	"R30",		Ureg_r30,
	"R28",		Ureg_r28,
	"R27",		Ureg_r27,
	"R26",		Ureg_r26,
	"R25",		Ureg_r25,
	"R24",		Ureg_r24,
	"R23",		Ureg_r23,
	"R22",		Ureg_r22,
	"R21",		Ureg_r21,
	"R20",		Ureg_r20,
	"R19",		Ureg_r19,
	"R18",		Ureg_r18,
	"R17",		Ureg_r17,
	"R16",		Ureg_r16,
	"R15",		Ureg_r15,
	"R14",		Ureg_r14,
	"R13",		Ureg_r13,
	"R12",		Ureg_r12,
	"R11",		Ureg_r11,
	"R10",		Ureg_r10,	
	"R9",		Ureg_r9,
	"R8",		Ureg_r8,
	"R7",		Ureg_r7,
	"R6",		Ureg_r6,
	"R5",		Ureg_r5,
	"R4",		Ureg_r4,
	"R3",		Ureg_r3,
	"R2",		Ureg_r2,
	"R1",		Ureg_r1,
};

static char *
ptlb(ulong phys)
{
	static char buf[4][32];
	static int k;
	char *p;

	k = (k+1)&3;
	p = buf[k];
	p += sprint(p, "(0x%lux %ld ", (phys<<6) & ~(BY2PG-1), (phys>>3)&7);
	if(phys & 4)
		*p++ = 'd';
	if(phys & 2)
		*p++ = 'v';
	if(phys & 1)
		*p++ = 'g';
	*p++ = ')';
	*p = 0;
	return buf[k];
}

static void
kpteprint(Ureg *ur)
{
	ulong i, tlbstuff[3];
	KMap *k;

	i = (ur->badvaddr & ~(2*BY2PG-1)) | TLBPID(tlbvirt());
	print("tlbvirt=0x%lux\n", i);
	i = gettlbp(i, tlbstuff);
	print("i=%ld v=0x%lux p0=%s p1=%s\n",
		i, tlbstuff[0], ptlb(tlbstuff[1]), ptlb(tlbstuff[2]));

	i = (ur->badvaddr & ~KMAPADDR)>>15;
	if(i > KPTESIZE){
		print("kpte index = 0x%lux ?\n", i);
		return;
	}
	k = &kpte[i];
	print("i=%ld, &k=0x%lux, k={v=0x%lux, p0=%s, p1=%s, pg=0x%lux}\n",
		i, k, k->virt, ptlb(k->phys0), ptlb(k->phys1), k->pg);
	print("pg={pa=0x%lux, va=0x%lux}\n", k->pg->pa, k->pg->va);
}

void
kvce(Ureg *ur, int ecode)
{
	char c;
	Pte **p;
	Page **pg;
	Segment *s;
	ulong addr, soff;

	c = 'D';
	if(ecode == CVCEI)
		c = 'I';
	print("Trap: VCE%c: addr=0x%lux\n", c, ur->badvaddr);
	if((ur->badvaddr & KSEGM) == KSEG3) {
		kpteprint(ur);
		return;
	}
	if(up && !(ur->badvaddr & KSEGM)) {
		addr = ur->badvaddr;
		s = seg(up, addr, 0);
		if(s == 0){
			print("no s\n");
			for(;;);
		}
		addr &= ~(BY2PG-1);
		soff = addr-s->base;
		p = &s->map[soff/PTEMAPMEM];
		if(*p){
			pg = &(*p)->pages[(soff&(PTEMAPMEM-1))/BY2PG];
			if(*pg) {
				print("pa=0x%lux, va=0x%lux\n",
					(*pg)->pa, (*pg)->va);
			} else
				print("no *pg\n");
		}else
			print("no *p\n");
	}
}

void
trap(Ureg *ur)
{
	int ecode;
	ulong fpfcr31;
	int user, cop, x, fpchk;
	char buf[2*ERRLEN], buf1[ERRLEN], *fpexcep;

	m->intrts = fastticks(nil);

	/*
	 * Retry failed probes of legal addresses from tstbadvaddr
	 * workaround for the R4400 >2.3
	 */
	if(m->vaddrtst) {
		m->vaddrtst = 0;
		return;
	}

	ecode = (ur->cause>>2)&EXCMASK;
	user = ur->status&KUSER;

	fpchk = 0;
	if(user){
		up->dbgreg = ur;
		if(up && up->fpstate == FPactive) {
			if((ur->status&CU1) == 0)		/* Paranoid */
				panic("FPactive but no CU1");
			ur->status &= ~CU1;
			up->fpstate = FPinactive;
			savefpregs(&up->fpsave);
		}
	}

	switch(ecode){
	case CINT:
		intr(ur);
		break;

	case CFPE:
		fptrap(ur);
		clrfpintr();
		fpchk = 1;
		break;

	case CTLBM:
	case CTLBL:
	case CTLBS:
		if(up == 0)
			kernfault(ur, ecode);

		if(!user && (ur->badvaddr & KMAPMASK) == KMAPADDR) {
			kfault(ur);
			break;
		}

		x = up->insyscall;
		up->insyscall = 1;
		spllo();
		faultmips(ur, user, ecode);
		up->insyscall = x;
		break;

	case CVCEI:
	case CVCED:
		kvce(ur, ecode);
		goto Default;

	case CCPU:
		cop = (ur->cause>>28)&3;
		if(user && up && cop == 1) {
			if(up->fpstate == FPinit) {
				up->fpstate = FPinactive;
				fpfcr31 = up->fpsave.fpstatus;
				up->fpsave = initfp;
				up->fpsave.fpstatus = fpfcr31;
				break;
			}
			if(up->fpstate == FPinactive)
				break;
		}
		/* Fallthrough */

	Default:
	default:
		if(user) {
			spllo();
			sprint(buf, "sys: %s", excname[ecode]);
			postnote(up, 1, buf, NDebug);
			break;
		}
		print("kernel %s pc=%lux\n", excname[ecode], ur->pc);
		dumpregs(ur);
		dumpstack();
		if(m->machno == 0)
			spllo();
		exit(1);
	}

	if(fpchk) {
		fpfcr31 = up->fpsave.fpstatus;
		if((fpfcr31>>12) & ((fpfcr31>>7)|0x20) & 0x3f) {
			spllo();
			fpexcep	= fpexcname(ur, fpfcr31, buf1);
			sprint(buf, "sys: fp: %s", fpexcep);
			postnote(up, 1, buf, NDebug);
		}
	}

	splhi();
	if(!user)
		return;

	notify(ur);
	if(up->fpstate == FPinactive) {
		restfpregs(&up->fpsave, up->fpsave.fpstatus&~FPEXPMASK);
		up->fpstate = FPactive;
		ur->status |= CU1;
	}
}

void
seteisadma(int ch, void (*func)(void))
{
	if(ch < 0 || ch > 7)
		panic("seteisadma");
	if(eisadma[ch] != 0)
		print("EISA dma%d: intr used twice", ch);

	eisadma[ch] = func;
}

void
eisadmaintr(void)
{
	int i;
	uchar isr;
	void (*f)(void);

	isr = EISAINB(Eisadmaintr) & ~(1<<4);
	for(i = 0; i < 8; i++) {
		if(isr & 1) {
			f = eisadma[i];
			if(f == 0)
				print("EISAdma%d: stray intr %.2ux\n", i, isr);
			else
				f();
		}
		isr >>= 1;
	}
}

void
intr(Ureg *ur)
{
	static uchar devint;
	ulong cause = ur->cause;
	ulong isr;

	m->intr++;
	cause &= INTR7|INTR6|INTR5|INTR4|INTR3|INTR2|INTR1|INTR0;
	if(cause & INTR3) {
		devint = IO(uchar, Intcause);
		switch(devint) {
		default:
			panic("unknown devint=#%lux", devint);
		case 0x28:		/* Serial 1 */
			ns16552intr(0);
			break;
		case 0x24:		/* Serial 2 */
			ns16552intr(1);
			break;
/*
		case 0x10:
			fiberint(0);
			break;
 */
		case 0x14:
			etherintr();
			break;
		case 0x1C:
			kbdintr();
			break;
		case 0x20:
			mouseintr();
			break;
		}
		cause &= ~INTR3;
	}

	if(cause & INTR2) {
		isr = IO(ulong, R4030Isr);
		if(isr) {
			print("R4030 Interrupt PC %lux R31 %lux\n", ur->pc, ur->r31);
			print(" ISR #%lux\n", IO(ulong, R4030Isr));
			print(" ET  #%lux\n", IO(ulong, R4030Et));
			print(" RFA #%lux\n", IO(ulong, R4030Rfa));
			print(" MFA #%lux\n", IO(ulong, R4030Mfa));
		}
		cause &= ~INTR2;
	}

	if(cause & INTR5)
		panic("EISA NMI\n");


	if(cause & INTR4) {
		devint = IO(uchar, I386ack);
		switch(devint) {
		default:
			print("i386ACK #%ux\n", devint);
			break;
		case 7:
			audiosbintr();
			break;
		case 13:
			eisadmaintr();
			break;
		}
		EISAOUTB(Int0ctl, EOI);
		if(devint > 7)
			EISAOUTB(Int1ctl, EOI);
		cause &= ~INTR4;
	}

	if(cause & INTR7) {
		clock(ur);
		return;
	}

	if(cause) {
		print("cause %lux\n", cause);
		exit(1);
	}

	/* preemptive scheduling */
	if(up && up->state == Running && anyhigher())
		sched();
}

char*
fpexcname(Ureg *ur, ulong fcr31, char *buf)
{
	int i;
	char *s;
	ulong fppc;

	fppc = ur->pc;
	if(ur->cause & (1<<31))	/* branch delay */
		fppc += 4;
	s = 0;
	if(fcr31 & (1<<17))
		s = "unimplemented operation";
	else{
		fcr31 >>= 7;		/* trap enable bits */
		fcr31 &= (fcr31>>5);	/* anded with exceptions */
		for(i=0; i<5; i++)
			if(fcr31 & (1<<i))
				s = fpcause[i];
	}

	if(s == 0)
		return "no floating point exception";

	sprint(buf, "%s fppc=0x%lux", s, fppc);
	return buf;
}

#define KERNPC(x)	(KTZERO<(ulong)(x)&&(ulong)(x)<(ulong)&etext)

void
kernfault(Ureg *ur, int code)
{
	print("kfault %s badvaddr=0x%lux\n", excname[code], ur->badvaddr);
	print("UP=0x%lux SR=0x%lux PC=0x%lux R31=%lux SP=0x%lux\n",
				up, ur->status, ur->pc, ur->r31, ur->sp);

	dumpregs(ur);
	panic("kfault");
}

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
	ureg.r31 = getcallerpc(&fn);
	fn(&ureg);
}

static void
_dumpstack(Ureg *ureg)
{
	ulong l, v, top, i;
	extern ulong etext;

	if(up == 0)
		return;

	print("ktrace /kernel/path %.8lux %.8lux %.8lux\n", ureg->pc, ureg->sp, ureg->r31);
	top = (ulong)up->kstack + KSTACK;
	i = 0;
	for(l=ureg->sp; l < top; l += BY2WD) {
		v = *(ulong*)l;
		if(KTZERO < v && v < (ulong)&etext) {
			print("%.8lux=%.8lux ", l, v);
			if((++i%4) == 0){
				print("\n");
				delay(200);
			}
		}
	}
	print("\n");
}

void
dumpstack(void)
{
	callwithureg(_dumpstack);
}

ulong
R(Ureg *ur, int i)
{
	uchar *s;

	s = (uchar*)ur;
	return *(ulong*)(s+regname[i].off-Uoffset);
}

void
dumpregs(Ureg *ur)
{
	int i;

	if(up)
		print("registers for %s %ld\n", up->text, up->pid);
	else
		print("registers for kernel\n");

	for(i=0; i<nelem(regname); i+=2)
		print("%s\t0x%.8lux\t%s\t0x%.8lux\n",
				regname[i].name, R(ur, i),
				regname[i+1].name, R(ur, i+1));
}

int
notify(Ureg *ur)
{
	int l;
	ulong sp;
	Note *n;

	if(up->procctl)
		procctl(up);
	if(up->nnote == 0)
		return 0;

	spllo();
	qlock(&up->debug);
	up->notepending = 0;
	n = &up->note[0];
	if(strncmp(n->msg, "sys:", 4) == 0) {
		l = strlen(n->msg);
		if(l > ERRLEN-15)	/* " pc=0x12345678\0" */
			l = ERRLEN-15;

		sprint(n->msg+l, " pc=0x%lux", ur->pc);
	}

	if(n->flag != NUser && (up->notified || up->notify==0)) {
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
	ur->hr1 = 0;
	if(ur->r1 < 0)
		ur->hr1 = ~0;
	((ulong*)sp)[1] = (ulong)up->ureg;	/* arg 1 0(FP) is ureg* */
	((ulong*)sp)[0] = 0;			/* arg 0 is pc */
	ur->usp = sp;
	ur->pc = (ulong)up->notify;
	up->notified = 1;
	up->nnote--;
	memmove(&up->lastnote, &up->note[0], sizeof(Note));
	memmove(&up->note[0], &up->note[1], up->nnote*sizeof(Note));

	qunlock(&up->debug);
	splhi();
	return 1;
}

/*
 * Check that status is OK to return from note.
 */
int
validstatus(ulong kstatus, ulong ustatus)
{
	if((kstatus & (INTMASK|KX|SX|UX)) != (ustatus & (INTMASK|KX|SX|UX)))
		return 0;
	if((ustatus&(KSU|ERL|EXL|IE)) != (KUSER|EXL|IE))
		return 0;
	if(ustatus & (0xFFFF0000&~CU1))	/* no CU3, CU2, CU0, RP, FR, RE, DS */
		return 0;
	return 1;
}

/*
 * Return user to state before notify()
 */
void
noted(Ureg *kur, Ureg **urp, ulong arg0)
{
	Ureg *nur;
	ulong oureg, sp;

	qlock(&up->debug);
	if(arg0!=NRSTR && !up->notified) {
		qunlock(&up->debug);
		pprint("call to noted() when not notified\n");
		pexit("Suicide", 0);
	}
	up->notified = 0;

	nur = up->ureg;

	oureg = (ulong)nur;
	if((oureg & (BY2V-1))
	|| !okaddr((ulong)oureg-BY2WD, BY2WD+sizeof(Ureg), 0)){
		pprint("bad ureg in noted or call to noted() when not notified\n");
		qunlock(&up->debug);
		pexit("Suicide", 0);
	}

	if(!validstatus(kur->status, nur->status)) {
		qunlock(&up->debug);
		pprint("bad noted ureg status %lux\n", nur->status);
		pexit("Suicide", 0);
	}

	memmove(*urp, up->ureg, sizeof(Ureg));
	switch(arg0) {
	case NCONT:
	case NRSTR:
		if(!okaddr(nur->pc, BY2WD, 0) || !okaddr(nur->usp, BY2WD, 0)){
			pprint("suicide: trap in noted\n");
			qunlock(&up->debug);
			pexit("Suicide", 0);
		}
		up->ureg = (Ureg*)(*(ulong*)(oureg-BY2WD));
		qunlock(&up->debug);
		splhi();
		rfnote(urp);
		break;

	case NSAVE:
		if(!okaddr(nur->pc, BY2WD, 0) || !okaddr(nur->usp, BY2WD, 0)){
			pprint("suicide: trap in noted\n");
			qunlock(&up->debug);
			pexit("Suicide", 0);
		}
		qunlock(&up->debug);
		sp = oureg-4*BY2WD-ERRLEN;
		splhi();
		(*urp)->sp = sp;
		((ulong*)sp)[1] = oureg;	/* arg 1 0(FP) is ureg* */
		((ulong*)sp)[0] = 0;			/* arg 0 is pc */
		(*urp)->r1 = oureg;		/* arg 1 is ureg* */
		(*urp)->hr1 = 0;
		if(oureg < 0)
			(*urp)->hr1 = ~0;
		rfnote(urp);
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

#include "../port/systab.h"

long
syscall(Ureg *aur)
{
	long ret;
	ulong sp;
	Ureg *ur;

	m->syscall++;
	up->insyscall = 1;
	ur = aur;
	up->pc = ur->pc;
	up->dbgreg = aur;
	ur->cause = 16<<2;	/* for debugging: system call is undef 16 */

	if(up->fpstate == FPactive) {
		if((ur->status&CU1) == 0)
			panic("syscall: FPactive but no CU1");
		up->fpsave.fpstatus = fcr31();
		up->fpstate = FPinit;
		ur->status &= ~CU1;
	}
	spllo();

	if(up->procctl)
		procctl(up);

	up->scallnr = ur->r1;
	up->nerrlab = 0;
	sp = ur->sp;
	ret = -1;
	if(waserror())
		goto error;

	if(up->scallnr >= nsyscall){
		pprint("bad sys call %d pc %lux\n", up->scallnr, ur->pc);
		postnote(up, 1, "sys: bad sys call", NDebug);
		error(Ebadarg);
	}

	if(sp & (BY2WD-1)){
		pprint("odd sp in sys call pc %lux sp %lux\n", ur->pc, ur->sp);
		postnote(up, 1, "sys: odd stack", NDebug);
		error(Ebadarg);
	}

	if(sp<(USTKTOP-BY2PG) || sp>(USTKTOP-sizeof(Sargs)))
		validaddr(sp, sizeof(Sargs), 0);

	up->s = *((Sargs*)(sp+BY2WD));
	up->psstate = sysctab[up->scallnr];

	ret = systab[up->scallnr](up->s.args);
	poperror();

error:
	ur->pc += 4;
	up->nerrlab = 0;
	up->psstate = 0;
	up->insyscall = 0;
	if(up->scallnr == NOTED)		/* ugly hack */
		noted(ur, &aur, *(ulong*)(sp+BY2WD));	/* doesn't return */

	splhi();
	if(up->scallnr!=RFORK && (up->procctl || up->nnote)){
		ur->r1 = ret;			/* load up for noted() */
		ur->hr1 = 0;
		if(ur->r1 < 0)
			ur->hr1 = ~0;
		if(notify(ur))
			return ur->r1;
	}
	return ret;
}

void
forkchild(Proc *p, Ureg *ur)
{
	Ureg *cur;

	p->sched.sp = (ulong)p->kstack+KSTACK-UREGSIZE;
	p->sched.pc = (ulong)forkret;

	cur = (Ureg*)(p->sched.sp+2*BY2WD);
	memmove(cur, ur, sizeof(Ureg));

	cur->pc += 4;

	/* Things from bottom of syscall we never got to execute */
	p->psstate = 0;
	p->insyscall = 0;
}

static
void
linkproc(void)
{
	spllo();
	up->kpfun(up->kparg);
}

void
kprocchild(Proc *p, void (*func)(void*), void *arg)
{
	p->sched.pc = (ulong)linkproc;
	p->sched.sp = (ulong)p->kstack+KSTACK;

	p->kpfun = func;
	p->kparg = arg;
}

long
execregs(ulong entry, ulong ssize, ulong nargs)
{
	Ureg *ur;
	ulong *sp;

	sp = (ulong*)(USTKTOP - ssize);
	*--sp = nargs;

	ur = (Ureg*)up->dbgreg;
	ur->usp = (ulong)sp;
	ur->pc = entry - 4;		/* syscall advances it */
	up->fpsave.fpstatus = initfp.fpstatus;
	return USTKTOP-BY2WD;		/* address of user-level clock */
}

ulong
userpc(void)
{
	Ureg *ur;

	ur = (Ureg*)up->dbgreg;
	return ur->pc;
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
setkernur(Ureg *xp, Proc *p)
{
	xp->pc = p->sched.pc;
	xp->sp = p->sched.sp;
	xp->r31 = (ulong)sched;
}

ulong
dbgpc(Proc *p)
{
	Ureg *ur;

	ur = p->dbgreg;
	if(ur == 0)
		return 0;

	return ur->pc;
}
