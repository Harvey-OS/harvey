#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"io.h"
#include	"../port/error.h"

void	(*vmevec[256])(int);
void	noted(Ureg**, ulong);
void	rfnote(Ureg**);

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
	"trap: undefined 13",
	"trap: undefined 14",
	"trap: undefined 15",			/* used as sys call for debugger */
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

char *regname[]={
	"STATUS",	"PC",
	"SP",		"CAUSE",
	"BADADDR",	"TLBVIRT",
	"HI",		"LO",
	"R31",		"R30",
	"R28",		"R27",
	"R26",		"R25",
	"R24",		"R23",
	"R22",		"R21",
	"R20",		"R19",
	"R18",		"R17",
	"R16",		"R15",
	"R14",		"R13",
	"R12",		"R11",
	"R10",		"R9",
	"R8",		"R7",
	"R6",		"R5",
	"R4",		"R3",
	"R2",		"R1",
};

void
trap(Ureg *ur)
{
	int ecode;
	ulong fcr31;
	int user, cop, x, fpchk;
	char buf[2*ERRLEN], buf1[ERRLEN], *fpexcep;

m->ur = ur;

	ecode = (ur->cause>>2)&0xf;
	user = ur->status&KUP;
	if(user)
		u->dbgreg = ur;

	fpchk = 0;
	if(u && u->p->fpstate == FPactive) {
		if((ur->status&CU1) == 0)		/* Paranoid */
			panic("FPactive but no CU1");
		u->p->fpstate = FPinactive;
		ur->status &= ~CU1;
		savefpregs(&u->fpsave);
		fptrap(ur);
		fpchk = 1;
	}

	switch(ecode){
	case CINT:
		if(ur->cause&INTR3) {			/* FP trap */
			clrfpintr();
			ur->cause &= ~INTR3;
		}
		intr(ur);
		break;

	case CTLBM:
	case CTLBL:
	case CTLBS:
		if(u == 0)
			panic("fault u==0 pc %lux addr %lux", ur->pc, ur->badvaddr);

		x = u->p->insyscall;
		u->p->insyscall = 1;

		spllo();
		faultmips(ur, user, ecode);
		u->p->insyscall = x;
		break;

	case CCPU:
		cop = (ur->cause>>28)&3;
		if(user && u && cop == 1) {
			if(u->p->fpstate == FPinit) {
				u->p->fpstate = FPinactive;
				fcr31 = u->fpsave.fpstatus;
				u->fpsave = initfp;
				u->fpsave.fpstatus = fcr31;
				break;
			}
			if(u->p->fpstate == FPinactive)
				break;
		}
		/* Fallthrough */

	default:
		if(user) {
			spllo();
			sprint(buf, "sys: %s", excname[ecode]);
			postnote(u->p, 1, buf, NDebug);
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
		fcr31 = u->fpsave.fpstatus;
		if((fcr31>>12) & ((fcr31>>7)|0x20) & 0x3f) {
			spllo();
			fpexcep	= fpexcname(ur, fcr31, buf1);
			sprint(buf, "sys: fp: %s", fpexcep);
			postnote(u->p, 1, buf, NDebug);
		}
	}

	splhi();
	if(!user)
		return;

	notify(ur);
	if(u->p->fpstate == FPinactive) {
		restfpregs(&u->fpsave, u->fpsave.fpstatus&~FPEXPMASK);
		u->p->fpstate = FPactive;
		ur->status |= CU1;
	}
}

void
intr(Ureg *ur)
{
	long v;
	int i, any;
	ulong cause;
	static int bogies;
	uchar pend, xxx;

	m->intr++;
	cause = ur->cause&(INTR5|INTR4|INTR3|INTR2|INTR1);

	if(cause & INTR1){
		duartintr();
		cause &= ~INTR1;
	}

	while(cause & INTR5) {
		any = 0;
		if(!(*MPBERR1 & (1<<8))){
			print("MP bus error %lux %lux\n", *MPBERR0, *MPBERR1);
			print("PC %lux R31 %lux\n", ur->pc, ur->r31);
			*MPBERR0 = 0;
			i = *SBEADDR;
			USED(i);
			any = 1;
		}

		/*
		 *  directions from IO2 manual
		 *  1. clear all IO2 masks
		 */
		*IO2CLRMASK = 0xff000000;

		/*
		 *  2. wait for interrupt in progress
		 */
		while(!(*INTPENDREG & (1<<5)))
			;

		/*
		 *  3. read pending interrupts
		 */
		pend = SBCCREG->fintpending;

		/*
		 *  4. clear pending register
		 */
		i = SBCCREG->flevel;
		USED(i);

		/*
		 *  4a. attempt to fix problem
		 */
		if(!(*INTPENDREG & (1<<5)))
			print("pause again\n");
		while(!(*INTPENDREG & (1<<5)))
			;
		xxx = SBCCREG->fintpending;
		if(xxx){
			print("new pend %ux\n", xxx);
			i = SBCCREG->flevel;
			USED(i);
		}

		/*
		 *  5a. process lance, scsi
		 */
		if(pend & 1) {
			v = INTVECREG->i[0].vec;
			if(!(v & (1<<12))){
				print("ioberr %lux %lux\n", *MPBERR0, *MPBERR1);
				print("PC %lux R31 %lux\n", ur->pc, ur->r31);
				*MPBERR0 = 0;
				any = 1;
			}
			if(ioid < IO3R1){
				if(!(v & 7))
					any = 1;
				if(!(v & (1<<2)))
					lanceintr();
				if(!(v & (1<<1)))
					lanceparity();
				if(!(v & (1<<0)))
					print("SCSI interrupt\n");
			}else{
				if(v & 7)
					any = 1;
				if(v & (1<<2))
					lanceintr();
				if(v & (1<<1))
					scsiintr(1);
				if(v & (1<<0))
					scsiintr(0);
			}
		}
		/*
		 *  5b. process vme
		 *  i can guess your level
		 */
		for(i=1; pend>>=1; i++){
			if(pend & 1) {
				v = INTVECREG->i[i].vec;
				if(!(v & (1<<12))){
					print("io2 mp bus error %d %lux %lux\n",
						i, *MPBERR0, *MPBERR1);
					*MPBERR0 = 0;
				}
				v &= 0xff;
				(*vmevec[v])(v);
				any = 1;
			}
		}
		/*
		 *  6. re-enable interrupts
		 */
		*IO2SETMASK = 0xff000000;
		if(any == 0)
			cause &= ~INTR5;
	}

	if(cause & (INTR2|INTR4)) {
		LEDON(LEDclock);
		clock(ur);
		LEDOFF(LEDclock);
		cause &= ~(INTR2|INTR4);
	}

	if(cause)
		panic("cause %lux %lux\n", u, cause);
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

void
dumpstack(void)
{
	ulong l, v;
	extern ulong etext;

	if(u == 0)
		return;

	for(l=(ulong)&l; l<USERADDR+BY2PG; l+=4){
		v = *(ulong*)l;
		if(KTZERO < v && v < (ulong)&etext){
			print("%lux=%lux\n", l, v);
			delay(100);
		}
	}
}

void
dumpregs(Ureg *ur)
{
	int i;
	ulong *l;

	if(u)
		print("registers for %s %d\n", u->p->text, u->p->pid);
	else
		print("registers for kernel\n");

	l = &ur->status;
	for(i=0; i<sizeof regname/sizeof(char*); i+=2, l+=2){
		print("%s\t%.8lux\t%s\t%.8lux\n", regname[i], l[0], regname[i+1], l[1]);
		prflush();
	}
}

int
notify(Ureg *ur)
{
	int l;
	ulong sp;
	Note *n;

	if(u->p->procctl)
		procctl(u->p);
	if(u->nnote == 0)
		return 0;

	spllo();
	qlock(&u->p->debug);
	u->p->notepending = 0;
	n = &u->note[0];
	if(strncmp(n->msg, "sys:", 4) == 0) {
		l = strlen(n->msg);
		if(l > ERRLEN-15)	/* " pc=0x12345678\0" */
			l = ERRLEN-15;

		sprint(n->msg+l, " pc=0x%lux", ur->pc);
	}

	if(n->flag != NUser && (u->notified || u->notify==0)) {
		if(n->flag == NDebug)
			pprint("suicide: %s\n", n->msg);

		qunlock(&u->p->debug);
		pexit(n->msg, n->flag!=NDebug);
	}

	if(u->notified) {
		qunlock(&u->p->debug);
		splhi();
		return 0;
	}
		
	if(!u->notify) {
		qunlock(&u->p->debug);
		pexit(n->msg, n->flag!=NDebug);
	}
	u->svstatus = ur->status;
	sp = ur->usp - sizeof(Ureg);

	if(sp&0x3 || !okaddr((ulong)u->notify, BY2WD, 0)
	|| !okaddr(sp-ERRLEN-3*BY2WD, sizeof(Ureg)+ERRLEN+3*BY2WD, 1)) {
		pprint("suicide: bad address or sp in notify\n");
		qunlock(&u->p->debug);
		pexit("Suicide", 0);
	}

	u->ureg = (void*)sp;
	memmove((Ureg*)sp, ur, sizeof(Ureg));
	sp -= ERRLEN;
	memmove((char*)sp, u->note[0].msg, ERRLEN);
	sp -= 3*BY2WD;
	*(ulong*)(sp+2*BY2WD) = sp+3*BY2WD;	/* arg 2 is string */
	u->svr1 = ur->r1;			/* save away r1 */
	ur->r1 = (ulong)u->ureg;		/* arg 1 is ureg* */
	*(ulong*)(sp+0*BY2WD) = 0;		/* arg 0 is pc */
	ur->usp = sp;
	ur->pc = (ulong)u->notify;
	u->notified = 1;
	u->nnote--;
	memmove(&u->lastnote, &u->note[0], sizeof(Note));
	memmove(&u->note[0], &u->note[1], u->nnote*sizeof(Note));

	qunlock(&u->p->debug);
	splhi();
	return 1;
}

/*
 * Return user to state before notify()
 */
void
noted(Ureg **urp, ulong arg0)
{
	Ureg *nur;

	nur = u->ureg;
	if(nur->status != u->svstatus) {
		pprint("bad noted ureg status %lux\n", nur->status);
		pexit("Suicide", 0);
	}
	qlock(&u->p->debug);
	if(!u->notified) {
		qunlock(&u->p->debug);
		pprint("call to noted() when not notified\n");
		pexit("Suicide", 0);
	}
	u->notified = 0;
	memmove(*urp, u->ureg, sizeof(Ureg));
	(*urp)->r1 = u->svr1;
	switch(arg0) {
	case NCONT:
		if(!okaddr(nur->pc, 1, 0) || !okaddr(nur->usp, BY2WD, 0)){
			pprint("suicide: trap in noted\n");
			qunlock(&u->p->debug);
			pexit("Suicide", 0);
		}
		splhi();
		qunlock(&u->p->debug);
		rfnote(urp);
		break;

	default:
		pprint("unknown noted arg 0x%lux\n", arg0);
		u->lastnote.flag = NDebug;
		/* fall through */
		
	case NDFLT:
		if(u->lastnote.flag == NDebug)
			pprint("suicide: %s\n", u->lastnote.msg);
		qunlock(&u->p->debug);
		pexit(u->lastnote.msg, u->lastnote.flag!=NDebug);
	}
}


#include "../port/systab.h"

long
syscall(Ureg *aur)
{
	long ret;
	ulong sp;
	Ureg *ur;
	Proc *p;

	m->syscall++;
	p = u->p;
	p->insyscall = 1;
	ur = aur;
	p->pc = ur->pc;
	u->dbgreg = aur;
	ur->cause = 15<<2;		/* for debugging: system call is undef 15; */

	if(p->fpstate == FPactive) {
		if((ur->status&CU1) == 0)
			panic("syscall: FPactive but no CU1");
		u->fpsave.fpstatus = fcr31();
		p->fpstate = FPinit;
		ur->status &= ~CU1;
	}

	spllo();

	if(p->procctl)
		procctl(p);

	u->scallnr = ur->r1;
	u->nerrlab = 0;
	sp = ur->sp;
	ret = -1;
	if(!waserror()){
		if(u->scallnr >= sizeof systab/sizeof systab[0]) {
			pprint("bad sys call number %d pc %lux\n", u->scallnr, ur->pc);
			postnote(p, 1, "sys: bad sys call", NDebug);
			error(Ebadarg);
		}

		if(sp & (BY2WD-1)){
			pprint("odd sp in sys call pc %lux sp %lux\n", ur->pc, ur->sp);
			postnote(p, 1, "sys: odd stack", NDebug);
			error(Ebadarg);
		}

		if(sp<(USTKTOP-BY2PG) || sp>(USTKTOP-sizeof(Sargs)))
			validaddr(sp, sizeof(Sargs), 0);

		u->s = *((Sargs*)(sp+BY2WD));
		p->psstate = sysctab[u->scallnr];

		ret = (*systab[u->scallnr])(u->s.args);
		poperror();
	}
	ur->pc += 4;
	u->nerrlab = 0;
	p->psstate = 0;
	p->insyscall = 0;
	if(u->scallnr == NOTED)				/* ugly hack */
		noted(&aur, *(ulong*)(sp+BY2WD));	/* doesn't return */

	splhi();
	if(u->scallnr!=RFORK && (p->procctl || u->nnote)){
		ur->r1 = ret;			/* load up for noted() */
		if(notify(ur))
			return ur->r1;
	}

	return ret;
}

long
execregs(ulong entry, ulong ssize, ulong nargs)
{
	ulong *sp;

	sp = (ulong*)(USTKTOP - ssize);
	*--sp = nargs;
	((Ureg*)UREGADDR)->usp = (ulong)sp;
	((Ureg*)UREGADDR)->pc = entry - 4;	/* syscall advances it */
	u->fpsave.fpstatus = initfp.fpstatus;
	return USTKTOP-BY2WD;			/* address of user-level clock */
}

ulong
userpc(void)
{
	return ((Ureg*)UREGADDR)->pc;
}

void
novme(int v)
{
	static count = 0;

	print("vme intr 0x%.2x\n", v);
	count++;
	if(count >= 10)
		panic("too many vme intr");
}

void
setvmevec(int v, void (*f)(int))
{
	void (*g)(int);

	v &= 0xff;
	g = vmevec[v];
	if(g && g != novme && g != f)
		print("second setvmevec to 0x%.2x\n", v);
	vmevec[v] = f;
}

/* This routine must save the values of registers the user is not permitted to write
 * from devproc and then restore the saved values before returning
 */
void
setregisters(Ureg *xp, char *pureg, char *uva, int n)
{
	ulong status;

	status = xp->status;
	memmove(pureg, uva, n);
	xp->status = status;
}
