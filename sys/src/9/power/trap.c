#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"io.h"
#include	"../port/error.h"

/*
 *  vme interrupt routines
 */
void	(*vmevec[256])(int);

void	noted(Ureg**, ulong);
void	rfnote(Ureg**);

#define	LSYS	0x01
#define	LUSER	0x02
/*
 * CAUSE register
 */

#define	EXCCODE(c)	((c>>2)&0x0F)
#define	FPEXC		16

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
	/* the following is made up */
	"trap: floating point exception"	/* FPEXC */
};
char	*fpexcname(Ureg*, ulong, char*);

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

long	ticks;

void
trap(Ureg *ur)
{
	int ecode;
	int user, x;
	ulong fcr31;
	char buf[2*ERRLEN], buf1[ERRLEN];

	SET(fcr31);
	ecode = EXCCODE(ur->cause);
	LEDON(ecode);
	user = ur->status&KUP;
	if(u)
		u->dbgreg = ur;

	switch(ecode){
	case CINT:
		if(u && u->p->state==Running){
			if(u->p->fpstate == FPactive) {
				if(ur->cause & INTR3){	/* FP trap */
					fcr31 = clrfpintr();
					if(user && fptrap(ur, fcr31))
						goto Return;
					ecode = FPEXC;
					goto Default;
				}
				savefpregs(&u->fpsave);
				u->p->fpstate = FPinactive;
				ur->status &= ~CU1;
			}
		}
		intr(ur);
		break;

	case CTLBM:
	case CTLBL:
	case CTLBS:
		if(u == 0)
			panic("fault u==0 pc %lux addr %lux", ur->pc, ur->badvaddr);
		if(u->p->fpstate == FPactive) {
			savefpregs(&u->fpsave);
			u->p->fpstate = FPinactive;
			ur->status &= ~CU1;
		}
		spllo();
		x = u->p->insyscall;
		u->p->insyscall = 1;
		faultmips(ur, user, ecode);
		u->p->insyscall = x;
		break;

	case CCPU:
		if(u && u->p && u->p->fpstate == FPinit) {
			restfpregs(&initfp, u->fpsave.fpstatus);
			u->p->fpstate = FPactive;
			ur->status |= CU1;
			break;
		}
		if(u && u->p && u->p->fpstate == FPinactive) {
			restfpregs(&u->fpsave, u->fpsave.fpstatus);
			u->p->fpstate = FPactive;
			ur->status |= CU1;
			break;
		}
		goto Default;

	default:
		if(u && u->p && u->p->fpstate == FPactive){
			savefpregs(&u->fpsave);
			u->p->fpstate = FPinactive;
			ur->status &= ~CU1;
		}
	Default:
		/*
		 * This isn't good enough; can still deadlock because we may
		 * hold print's locks in this processor.
		 */
		if(user){
			spllo();
			if(ecode == FPEXC)
				sprint(buf, "sys: fp: %s", fpexcname(ur, fcr31, buf1));
			else
				sprint(buf, "sys: %s", excname[ecode]);

			postnote(u->p, 1, buf, NDebug);
		}else{
			print("%s %s pc=%lux\n", user? "user": "kernel", excname[ecode], ur->pc);
			if(ecode == FPEXC)
				print("fp: %s\n", fpexcname(ur, fcr31, buf1));
			dumpregs(ur);
			dumpstack();
			if(m->machno == 0)
				spllo();
			exit(1);
		}
	}

	splhi();
    Return:
	if(user) {
		notify(ur);
		if(u->p->fpstate == FPinactive) {
			restfpregs(&u->fpsave, u->fpsave.fpstatus);
			u->p->fpstate = FPactive;
			ur->status |= CU1;
		}
	}
	LEDOFF(ecode);
}

void
intr(Ureg *ur)
{
	int i, any;
	uchar xxx;
	uchar pend, npend;
	long v;
	ulong cause;
	static int bogies;

	m->intr++;
	cause = ur->cause&(INTR5|INTR4|INTR3|INTR2|INTR1);
	if(cause & (INTR2|INTR4)){
		LEDON(LEDclock);
		clock(ur);
		LEDOFF(LEDclock);
		cause &= ~(INTR2|INTR4);
	}
	if(cause & INTR1){
		duartintr();
		cause &= ~INTR1;
	}
	if(cause & INTR5){
		any = 0;
		if(!(*MPBERR1 & (1<<8))){
			print("MP bus error %lux %lux\n", *MPBERR0, *MPBERR1);
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
		npend = pend;

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
			npend = pend |= xxx;
			i = SBCCREG->flevel;
			USED(i);
		}

		/*
		 *  5a. process lance, scsi
		 */
		if(pend & 1) {
			v = INTVECREG->i[0].vec;
			if(!(v & (1<<12))){
				print("io2 mp bus error %d %lux %lux\n", 0,
					*MPBERR0, *MPBERR1);
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
					print("SCSI 1 interrupt\n");
				if(v & (1<<0))
					print("SCSI 0 interrupt\n");
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
		 *  if nothing else, what the hell?
		 */
		if(!any && bogies++<10){
			if(0)
			print("bogus intr lvl 5 pend %lux on %d\n", npend, m->machno);
			USED(npend);
			delay(100);
		}
		/*
		 *  6. re-enable interrupts
		 */
		*IO2SETMASK = 0xff000000;
		cause &= ~INTR5;
	}
	if(cause)
		panic("cause %lux %lux\n", u, cause);
}

char*
fpexcname(Ureg *ur, ulong fcr31, char *buf)
{
	static char *str[]={
		"inexact operation",
		"underflow",
		"overflow",
		"division by zero",
		"invalid operation",
	};
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
				s = str[i];
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

	if(u)
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

/*
 * Call user, if necessary, with note
 */
int
notify(Ureg *ur)
{
	int l, sent;
	ulong s, sp;
	Note *n;

	if(u->p->procctl)
		procctl(u->p);
	if(u->nnote == 0)
		return 0;

	s = spllo();
	qlock(&u->p->debug);
	u->p->notepending = 0;
	n = &u->note[0];
	if(strncmp(n->msg, "sys:", 4) == 0){
		l = strlen(n->msg);
		if(l > ERRLEN-15)	/* " pc=0x12345678\0" */
			l = ERRLEN-15;
		sprint(n->msg+l, " pc=0x%lux", ur->pc);
	}
	if(n->flag!=NUser && (u->notified || u->notify==0)){
		if(n->flag == NDebug)
			pprint("suicide: %s\n", n->msg);
    Die:
		qunlock(&u->p->debug);
		pexit(n->msg, n->flag!=NDebug);
	}
	sent = 0;
	if(!u->notified){
		if(!u->notify)
			goto Die;
		sent = 1;
		u->svstatus = ur->status;
		sp = ur->usp;
		sp -= sizeof(Ureg);
		if(!okaddr((ulong)u->notify, 1, 0)
		|| !okaddr(sp-ERRLEN-3*BY2WD, sizeof(Ureg)+ERRLEN-3*BY2WD, 0)){
			pprint("suicide: bad address in notify\n");
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
	}
	qunlock(&u->p->debug);
	splx(s);
	return sent;
}

/*
 * Return user to state before notify()
 */
void
noted(Ureg **urp, ulong arg0)
{
	Ureg *nur;

	nur = u->ureg;
	if(nur->status!=u->svstatus){
		pprint("bad noted ureg status %lux\n", nur->status);
    Die:
		pexit("Suicide", 0);
	}
	qlock(&u->p->debug);
	if(!u->notified){
		qunlock(&u->p->debug);
		pprint("call to noted() when not notified\n");
		goto Die;
	}
	u->notified = 0;
	memmove(*urp, u->ureg, sizeof(Ureg));
	(*urp)->r1 = u->svr1;
	switch(arg0){
	case NCONT:
		if(!okaddr(nur->pc, 1, 0) || !okaddr(nur->usp, BY2WD, 0)){
			pprint("suicide: trap in noted\n");
			qunlock(&u->p->debug);
			goto Die;
		}
		splhi();
		qunlock(&u->p->debug);
		rfnote(urp);
		break;
		/* never returns */

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
	ur->cause = 15<<2;		/* for debugging: system call is undef 15;
	/*
	 * since the system call interface does not
	 * guarantee anything about registers, we can
	 * smash them.  but we must save fpstatus.
	 */
	if(p->fpstate == FPactive) {
		u->fpsave.fpstatus = fcr31();
		p->fpstate = FPinit;
		ur->status &= ~CU1;
	}
	spllo();

	if(p->procctl)
		procctl(p);

	u->scallnr = ur->r1;
	sp = ur->sp;
	u->nerrlab = 0;
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
		ur->r1 = ret;				/* load up for noted() */
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
