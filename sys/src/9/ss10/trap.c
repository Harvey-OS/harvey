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
int	domuldiv(ulong, Ureg*);

extern	Label catch;
extern	void traplink(void);
extern	void syslink(void);

	long	ticks;
static	char	excbuf[64];	/* BUG: not reentrant! */

char *trapname[]={
	"reset",
	"instruction access exception",
	"illegal instruction",
	"privileged instruction",
	"fp: disabled",
	"window overflow",
	"window underflow",
	"unaligned address",
	"fp: exception",
	"data access exception",
	"tag overflow",
	"watchpoint detected",
};

char *fptrapname[]={
	"none",
	"IEEE 754 exception",
	"unfinished FP op",
	"unimplemented FP op",
	"sequence error",
	"hardware error",
	"invalid FP register",
	"reserved",
};

char*
excname(ulong tbr)
{
	char xx[64];
	char *t;
	ulong fsr;

	switch(tbr){
	case 8:
		if(u == 0)
			panic("fptrap in kernel\n");
		else{
			if(m->fpunsafe==0 && u->p->fpstate!=FPactive)
				panic("fptrap not active\n");
			fsr = u->fpsave.fsr;
			sprint(excbuf, "fp: %s fppc=0x%lux",
				fptrapname[(fsr>>14)&7],
				u->fpsave.q[0].a, fsr);
		}
		return excbuf;
	case 36:
		return "trap: cp disabled";
	case 37:
		return "trap: unimplemented instruction";
	case 40:
		return "trap: cp exception";
	case 42:
		return "trap: divide by zero";
	case 43:
		return "trap: data store error (store buffer)";
	case 128:
		return "syscall";
	case 129:
		return "breakpoint";
	}
	t = 0;
	if(tbr < sizeof trapname/sizeof(char*))
		t = trapname[tbr];
	if(t == 0){
		if(tbr >= 130)
			sprint(xx, "trap instruction %d", tbr-128);
		else if(17<=tbr && tbr<=31)
			sprint(xx, "interrupt level %d", tbr-16);
		else
			sprint(xx, "unknown trap %d", tbr);
		t = xx;
	}
	if(strncmp(t, "fp: ", 4) == 0)
		strcpy(excbuf, t);
	else
		sprint(excbuf, "trap: %s", t);
	return excbuf;
}

void
trap(Ureg *ur)
{
	int user, x;
	char buf[64];
	ulong tbr, iw;

	tbr = (ur->tbr&0xFFF)>>4;
	/*
	 * Hack to catch bootstrap fault during probe
	 */
	if(catch.pc)
		gotolabel(&catch);

	if(u)
		u->dbgreg = ur;

	user = !(ur->psr&PSRPSUPER);
	if(tbr > 16){			/* interrupt */
		if(u && u->p->state==Running){
			/* if active, FPop at head of Q is probably an excep */
			if(u->p->fpstate == FPactive){
				fpquiet();
				savefpregs(&u->fpsave);
				u->p->fpstate = FPinactive;
			}
		}
		switch(tbr-16){
		case 15:			/* asynch mem err */
			faultasync(ur);
			break;
		case 14:			/* counter 1 */
			clock(ur);
			break;
		case 12:			/* keyboard and mouse */
			sccintr();
			break;
		case 11:			/* floppy drive */
			floppyintr(ur);
			break;
		case 6:			/* lance */
			lanceintr();
			break;
		case 4:			/* SCSI */
			scsiintr();
			break;
		default:
			goto Error;
		}
	}else{
		switch(tbr){
		case 1:				/* instr. access */
		case 9:				/* data access */
			if(u && u->p->fpstate==FPactive) {
				fpquiet();
				savefpregs(&u->fpsave);
				u->p->fpstate = FPinactive;
			}

			if(u){
				x = u->p->insyscall;
				u->p->insyscall = 1;
				faultsparc(ur);
				u->p->insyscall = x;
			}else {
				faultsparc(ur);
			}
			goto Return;
		case 2:				/* illegal instr, maybe mul */
			iw = *(ulong*)ur->pc;
			if((iw&0xC1500000) == 0x80500000){
				if(domuldiv(iw, ur))
					goto Return;
				tbr = ur->tbr;
			}
			break;
		case 4:				/* floating point disabled */
			if(u && u->p){
				if(u->p->fpstate == FPinit)
					restfpregs(initfpp, u->fpsave.fsr);
				else if(u->p->fpstate == FPinactive)
					restfpregs(&u->fpsave, u->fpsave.fsr);
				else
					break;
				u->p->fpstate = FPactive;
				ur->psr |= PSREF;
				return;
			}
			break;
		case 8:				/* floating point exception */
			/* if unsafe, trap happened shutting down FPU; just return */
			if(m->fpunsafe){
				m->fptrap = (fptrap()==0);
				return;
			}
			if(fptrap())
				goto Return;	/* handled the problem */
			break;
		default:
			break;
		}
    Error:
		if(user){
			spllo();
			sprint(buf, "sys: %s", excname(tbr));
			postnote(u->p, 1, buf, NDebug);
		}else
			panic("kernel trap: %s pc=0x%lux\n", excname(tbr), ur->pc);
	}
    Return:
	if(user){
		notify(ur);
		if(u->p->fpstate == FPinactive) {
			restfpregs(&u->fpsave, u->fpsave.fsr);
			u->p->fpstate = FPactive;
			ur->psr |= PSREF;
		}
	}
}

void
trapinit(void)
{
	int i;
	long t, a;

	a = ((ulong)traplink-TRAPS)>>2;
	a += 0x40000000;			/* CALL traplink(SB) */
	t = TRAPS;
	for(i=0; i<256; i++){
		/* ss2: must flush cache line for table */
		*(ulong*)(t+0) = a;		/* CALL traplink(SB) */
		*(ulong*)(t+4) = 0xa7480000;	/* MOVW PSR, R19 */
		a -= 16/4;
		t += 16;
	}
	/*
	 * Vector 128 goes directly to syslink
	 */
	t = TRAPS+128*16;
	a = ((ulong)syslink-t)>>2;
	a += 0x40000000;
	*(ulong*)t = a;			/* CALL syscall(SB) */
	*(ulong*)(t+4) = 0xa7480000;	/* MOVW PSR, R19 */
	puttbr(TRAPS);
	setpsr(getpsr()|PSRET|SPL(15));	/* enable traps, not interrupts */
}

void
mulu(ulong u1, ulong u2, ulong *lop, ulong *hip)
{
	ulong lo1, lo2, hi1, hi2, lo, hi, t1, t2, t;

	lo1 = u1 & 0xffff;
	lo2 = u2 & 0xffff;
	hi1 = u1 >> 16;
	hi2 = u2 >> 16;

	lo = lo1 * lo2;
	t1 = lo1 * hi2;
	t2 = lo2 * hi1;
	hi = hi1 * hi2;
	t = lo;
	lo += t1 << 16;
	if(lo < t)
		hi++;
	t = lo;
	lo += t2 << 16;
	if(lo < t)
		hi++;
	hi += (t1 >> 16) + (t2 >> 16);
	*lop = lo;
	*hip = hi;
}

void
muls(long l1, long l2, long *lop, long *hip)
{
	ulong t, lo, hi;
	ulong mlo, mhi;
	int sign;

	sign = 0;
	if(l1 < 0){
		sign ^= 1;
		l1 = -l1;
	}
	if(l2 < 0){
		sign ^= 1;
		l2 = -l2;
	}
	mulu(l1, l2, &mlo, &mhi);
	lo = mlo;
	hi = mhi;
	if(sign){
		t = lo = ~lo;
		hi = ~hi;
		lo++;
		if(lo < t)
			hi++;
	}
	*lop = lo;
	*hip = hi;
}

int
domuldiv(ulong iw, Ureg *ur)
{
	long op1, op2;
	long *regp;
	long *regs;

	regs = (long*)ur;
	if(iw & (1<<13)){	/* signed immediate */
		op2 = iw & 0x1FFF;
		if(op2 & 0x1000)
			op2 |= ~0x1FFF;
	}else
		op2 = regs[iw&0x1F];
	op1 = regs[(iw>>14)&0x1F];
	regp = &regs[(iw>>25)&0x1F];

	if(iw & (4<<19)){	/* divide */
		if(ur->y!=0 && ur->y!=~0){
	unimp:
			ur->tbr = 37;	/* "unimplemented instruction" */
			return 0;	/* complex Y is too hard */
		}
		if(op2 == 0){
			ur->tbr = 42;	/* "zero divide" */
			return 0;
		}
		if(iw & (1<<19)){
			if(ur->y && (op1&(1<<31))==0)
				goto unimp;	/* Y not sign extension */
			*regp = op1 / op2;
		}else{
			if(ur->y)
				goto unimp;
			*regp = (ulong)op1 / (ulong)op2;
		}
	}else{
		if(iw & (1<<19))
			muls(op1, op2, regp, (long*)&ur->y);
		else
			mulu(op1, op2, (ulong*)regp, &ur->y);
	}
	if(iw & (16<<19)){	/* set CC */
		ur->psr &= ~(0xF << 20);
		if(*regp & (1<<31))
			ur->psr |= 8 << 20;	/* N */
		if(*regp == 0)
			ur->psr |= 4 << 20;	/* Z */
		/* BUG: don't get overflow right on divide */
	}
	ur->pc += 4;
	ur->npc = ur->pc+4;
	return 1;
}

void
dumpstack(void)
{
	ulong l, v;
	int i;
	extern ulong etext;
	static int dumping;

	if(dumping == 1){
		print("no dumpstack\n");
		return;
	}

	dumping = 1;
	if(u){
		i = 0;
		for(l=(ulong)&l; l<USERADDR+BY2PG; l+=4){
			v = *(ulong*)l;
			if(KTZERO < v && v < (ulong)&etext){
				print("%lux=%lux  ", l, v);
				++i;
				if((i&3) == 0)
					print("\n");
			}
		}
	}
	print("\n");
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
	print("PSR=%ux PC=%lux TBR=%lux\n", ur->psr, ur->pc, ur->tbr);
	l = &ur->r0;
	for(i=0; i<32; i+=2, l+=2)
		print("R%d\t%.8lux\tR%d\t%.8lux\n", i, l[0], i+1, l[1]);
	dumpstack();
}

int
notify(Ureg *ur)
{
	int l;
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
		qunlock(&u->p->debug);
		if(n->flag == NDebug){
			qunlock(&u->p->debug);
			pprint("suicide: %s\n", n->msg);
		}else
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

	sp = ur->usp;
	sp -= sizeof(Ureg)+ERRLEN+4*BY2WD;
	sp &= ~7;	/* SP must be 8-byte aligned */
	sp += ERRLEN+4*BY2WD;
	if(!okaddr((ulong)u->notify, BY2WD, 0)
	|| !okaddr(sp-ERRLEN-4*BY2WD, sizeof(Ureg)+ERRLEN+4*BY2WD, 1)){
		qunlock(&u->p->debug);
		pprint("suicide: bad address in notify\n");
		pexit("Suicide", 0);
	}

	memmove((Ureg*)sp, ur, sizeof(Ureg));
	*(Ureg**)(sp-BY2WD) = u->ureg;	/* word under Ureg is old u->ureg */
	u->ureg = (void*)sp;
	sp -= BY2WD+ERRLEN;
	memmove((char*)sp, u->note[0].msg, ERRLEN);
	sp -= 3*BY2WD;
	*(ulong*)(sp+2*BY2WD) = sp+3*BY2WD;	/* arg 2 is string */
	*(ulong*)(sp+1*BY2WD) = (ulong)u->ureg;	/* arg 1 is ureg* (compat) */
	ur->r7 = (ulong)u->ureg;		/* arg 1 is ureg* */
	*(ulong*)(sp+1*BY2WD) = (ulong)u->ureg;	/* arg 1 0(FP) is ureg* */
	*(ulong*)(sp+0*BY2WD) = 0;		/* arg 0 is pc */
	ur->usp = sp;
	ur->pc = (ulong)u->notify;
	ur->npc = (ulong)u->notify+4;
	u->notified = 1;
	u->nnote--;
	memmove(&u->lastnote, &u->note[0], sizeof(Note));
	memmove(&u->note[0], &u->note[1], u->nnote*sizeof(Note));
	qunlock(&u->p->debug);
	splx(s);
	return 1;
}

/*
 * Check that status is OK to return from note.
 */
int
validstatus(ulong kpsr, ulong upsr)
{
	if((kpsr & (PSRIMPL|PSRVER|PSRCWP|PSRPIL)) != (upsr & (PSRIMPL|PSRVER|PSRCWP|PSRPIL)))
		return 0;
	if((upsr & (PSRRESERVED|PSREC|PSRSUPER|PSRPSUPER|PSRET)) != PSRSUPER)
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

	qlock(&u->p->debug);
	if(arg0!=NRSTR && !u->notified){
		qunlock(&u->p->debug);
		pprint("call to noted() when not notified\n");
		pexit("Suicide", 0);
	}
	u->notified = 0;

	nur = u->ureg;

	oureg = (ulong)nur;
	if((oureg & (BY2WD-1))
	|| !okaddr((ulong)oureg-BY2WD, BY2WD+sizeof(Ureg), 0)){
		qunlock(&u->p->debug);
		pprint("suicide: bad u->p->ureg in noted\n");
		pexit("Suicide", 0);
	}

	if(!validstatus(kur->psr, nur->psr)) {
		qunlock(&u->p->debug);
		pprint("bad noted ureg psr %lux\n", nur->psr);
		pexit("Suicide", 0);
	}

	memmove(*urp, u->ureg, sizeof(Ureg));
	switch(arg0){
	case NCONT:
	case NRSTR:
		if(!okaddr(nur->pc, 1, 0) || !okaddr(nur->usp, BY2WD, 0)){
			qunlock(&u->p->debug);
			pprint("suicide: trap in noted\n");
			pexit("Suicide", 0);
		}
		u->ureg = (Ureg*)(*(ulong*)(oureg-BY2WD));
		qunlock(&u->p->debug);
		splhi();
		rfnote(urp);
		break;
		/* never returns */

	case NSAVE:
		if(!okaddr(nur->pc, BY2WD, 0) || !okaddr(nur->usp, BY2WD, 0)){
			qunlock(&u->p->debug);
			pprint("suicide: trap in noted\n");
			pexit("Suicide", 0);
		}
		qunlock(&u->p->debug);
		sp = oureg-4*BY2WD-ERRLEN;
		splhi();
		(*urp)->usp = sp;
		((ulong*)sp)[1] = oureg;	/* arg 1 0(FP) is ureg* */
		((ulong*)sp)[0] = 0;			/* arg 0 is pc */
		(*urp)->r7 = oureg;		/* arg 1 is ureg* */
		rfnote(urp);
		break;

	default:
		u->lastnote.flag = NDebug;
		qunlock(&u->p->debug);
		pprint("unknown noted arg 0x%lux\n", arg0);
		pprint("suicide: %s\n", u->lastnote.msg);
		pexit(u->lastnote.msg, 0);
		
	case NDFLT:
		if(u->lastnote.flag == NDebug){
			qunlock(&u->p->debug);
			pprint("suicide: %s\n", u->lastnote.msg);
		}else
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

	ur = aur;
	if(ur->psr & PSRPSUPER)
		panic("recursive system call");
	u->p->insyscall = 1;
	u->p->pc = ur->pc;

	if(u->p->fpstate == FPactive) {
		fpquiet();
		u->p->fpstate = FPinit;
		u->fpsave.fsr = getfsr();
		ur->psr &= ~PSREF;
	}
	spllo();

	u->scallnr = ur->r7;
	sp = ur->usp;

	u->nerrlab = 0;
	ret = -1;
	if(!waserror()){
		if(u->scallnr >= sizeof systab/BY2WD){
			pprint("bad sys call number %d pc %lux\n", u->scallnr, ur->pc);
			postnote(u->p, 1, "sys: bad sys call", NDebug);
			error(Ebadarg);
		}

		if(sp & (BY2WD-1)){
			pprint("odd sp in sys call pc %lux sp %lux\n", ur->pc, ur->sp);
			postnote(u->p, 1, "sys: odd stack", NDebug);
			error(Ebadarg);
		}

		if(sp<(USTKTOP-BY2PG) || sp>(USTKTOP-sizeof(Sargs)))
			validaddr(sp, sizeof(Sargs), 0);

		u->s = *((Sargs*)(sp+1*BY2WD));
		u->p->psstate = sysctab[u->scallnr];

		ret = (*systab[u->scallnr])(u->s.args);
		poperror();
	}

	ur->pc += 4;
	ur->npc = ur->pc+4;
	u->nerrlab = 0;
	if(u->p->procctl)
		procctl(u->p);

	u->p->insyscall = 0;
	u->p->psstate = 0;
	if(u->scallnr == NOTED)	/* ugly hack */
		noted(ur, &aur, *(ulong*)(sp+1*BY2WD));	/* doesn't return */

	splhi();
	if(u->scallnr!=RFORK && (u->p->procctl || u->nnote)){
		ur->r7 = ret;				/* load up for noted() */
		if(notify(ur))
			return ur->r7;
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
	u->fpsave.fsr = initfpp->fsr;
	return USTKTOP-BY2WD;			/* address of user-level clock */
}

ulong
userpc(void)
{
	return ((Ureg*)UREGADDR)->pc;
}

/* This routine must save the values of registers the user is not permitted to write
 * from devproc and the restore the saved values before returning
 */
void
setregisters(Ureg *xp, char *pureg, char *uva, int n)
{
	ulong psr;

	psr = xp->psr;
	memmove(pureg, uva, n);
	xp->psr = psr;
}
