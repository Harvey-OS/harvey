#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"io.h"
#include	"../port/error.h"

void	(*midiintr)(void);

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

static char *
ptlb(ulong phys)
{
	static char buf[4][32];
	static int k;
	char *p;

	k = (k+1)&3;
	p = buf[k];
	p += sprint(p, "(0x%ux %d ", (phys<<6) & ~(BY2PG-1), (phys>>3)&7);
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
ktlbprint(Ureg *ur)
{
	ulong i, tlbstuff[3];
	KMap *k;
	Page *pg;

	i = (ur->badvaddr & ~(2*BY2PG-1)) | TLBPID(tlbvirt());
	print("tlbvirt=0x%ux\n", i);
	i = gettlbp(i, tlbstuff);
	print("i=%d v=0x%ux p0=%s p1=%s\n",
		i, tlbstuff[0], ptlb(tlbstuff[1]), ptlb(tlbstuff[2]));

	i = (ur->badvaddr & ~KMAPADDR)>>15;
	if(i > KTLBSIZE){
		print("ktlb index = 0x%ux ?\n", i);
		return;
	}
	k = &ktlb[i];
	print("i=%d, &k=0x%ux, k={v=0x%ux, p0=%s, p1=%s, pg=0x%ux}\n",
		i, k, k->virt, ptlb(k->phys0), ptlb(k->phys1),
		k->next);
	pg = (void *)k->next;
	print("pg={pa=0x%ux, va=0x%ux}\n", pg->pa, pg->va);
}

void
trap(Ureg *ur)
{
	int ecode;
	ulong fcr31;
	int user, cop, x, fpchk;
	char buf[2*ERRLEN], buf1[ERRLEN], *fpexcep;

	ecode = (ur->cause>>2)&EXCMASK;
	user = ur->status&KUSER;
	if(u)
		u->dbgreg = ur;

	fpchk = 0;
	if(u && u->p->fpstate == FPactive){
		if((ur->status&CU1) == 0)		/* Paranoid */
			panic("FPactive but no CU1");
		u->p->fpstate = FPinactive;
		ur->status &= ~CU1;
		savefpregs(&u->fpsave);
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
		if(u == 0)
			kernfault(ur, ecode);
		x = u->p->insyscall;
		u->p->insyscall = 1;

		spllo();
		faultmips(ur, user, ecode);
		u->p->insyscall = x;
		break;

	case CVCEI:
	case CVCED:
		print("trap: VCE%c: addr=0x%ux\n",
			(ecode==CVCEI)?'I':'D', ur->badvaddr);
		if((ur->badvaddr & KSEGM) == KSEG3)
			ktlbprint(ur);
		else if(u && !(ur->badvaddr & KSEGM)){
			ulong addr, soff;
			Segment *s;
			Page **pg;
			Pte **p;

			addr = ur->badvaddr;
			s = seg(u->p, addr, 0);
			addr &= ~(BY2PG-1);
			soff = addr-s->base;
			p = &s->map[soff/PTEMAPMEM];
			if(*p){
				pg = &(*p)->pages[(soff&(PTEMAPMEM-1))/BY2PG];
				if(*pg)
					print("pa=0x%ux, va=0x%ux\n",
						(*pg)->pa, (*pg)->va);
				else
					print("no *pg\n");
			}else
				print("no *p\n");
		}
		if(user){
			dumpregs(ur);
			dumpstack();
		}
		goto Default;

	case CBUSD:
		if(ur->pc != 4+(ulong)busprobe)
			goto Default;
		ur->r1 = -1;
		ur->pc += 8;
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

	Default:
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
	ulong x;

	m->intr++;
	if(ur->cause & INTR5){
		wrcompare(rdcount()+50000000);
	}
	if(ur->cause & INTR4){
		kprint("GIO bus error interrupt\n");
		x = *IO(ulong, GIO_ERR_ADDR);
		kprint("addr = 0x%ux\n", x);
		x = *IO(ulong, GIO_ERR_STAT);
		kprint("status = 0x%ux\n", x);
		*IO(uchar, GIO_ERR_STAT) = 0;
	}
	if(ur->cause & INTR3){
		*IO(uchar, TIMER_ACK) = 0x02;
		if(midiintr)
			(*midiintr)();
	}
	if(ur->cause & INTR1){
		x = *IO(uchar, LIO_1_ISR) & *IO(uchar, LIO_1_MASK);
		if(x & LIO_DSP){
			audiointr();
			x &= ~LIO_DSP;
		}
		if(x)
			panic("lio1 intr 0x%.2x", x);
	}
	if(ur->cause & INTR0){
		x = *IO(uchar, LIO_0_ISR) & *IO(uchar, LIO_0_MASK);
		if(x & LIO_DUART){
			sccintr();
			x &= ~LIO_DUART;
		}
		if(x & LIO_ENET){
			m->nettime = m->ticks;
			seqintr();
			x &= ~LIO_ENET;
		}
		if(x & LIO_SCSI){
			scsiintr();
			x &= ~LIO_SCSI;
		}
		if(x & LIO_CENTR){
			lptintr();
			x &= ~LIO_CENTR;
		}
		if(x & LIO_GIO_1){
			devportintr();
			x &= ~LIO_GIO_1;
		}
		if(x)
			panic("lio0 intr 0x%.2x", x);
	}
	if(ur->cause & INTR2){	/* check this one last */
		*IO(uchar, TIMER_ACK) = 0x01;
		clock(ur);
	}
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
	Label l;

	print("panic: kfault %s badvaddr=0x%lux\n", excname[code], ur->badvaddr);
	ktlbprint(ur);
	print("u=0x%lux status=0x%lux pc=0x%lux sp=0x%lux\n",
				u, ur->status, ur->pc, ur->sp);
	dumpregs(ur);
	l.sp = ur->sp;
	l.pc = ur->pc;
	dumpstack();
	exit(1);
}

void
dumpstack(void)
{
	ulong l, v;
	extern ulong etext;
	int i;

	if(!u)
		return;
	l = (ulong)&l;
	if(l < USERADDR+sizeof(User)){
		print("bad stack address %lux\n", l);
		l = USERADDR+sizeof(User);
	}
	i = 0;
	for(; l<USERADDR+BY2PG; l+=4){
		v = *(ulong*)l;
		if(KTZERO < v && v < (ulong)&etext){
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
	if(u)
		print("registers for %s %d\n", u->p->text, u->p->pid);
	else
		print("registers for kernel\n");

	l = &ur->status;
	for(i=0; i<sizeof regname/sizeof(char*); i+=2, l+=2)
		print("%s\t%.8lux\t%s\t%.8lux\n", regname[i], l[0], regname[i+1], l[1]);
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

	sp = ur->usp & ~(BY2V-1);
	sp -= sizeof(Ureg);

	if(sp&(BY2WD-1) || !okaddr((ulong)u->notify, BY2WD, 0)
	|| !okaddr(sp-ERRLEN-4*BY2WD, sizeof(Ureg)+ERRLEN+4*BY2WD, 1)) {
		qunlock(&u->p->debug);
		pprint("suicide: bad address or sp in notify\n");
		pexit("Suicide", 0);
	}

	memmove((Ureg*)sp, ur, sizeof(Ureg));
	*(Ureg**)(sp-BY2WD) = u->ureg;	/* word under Ureg is old u->ureg */
	u->ureg = (void*)sp;
	sp -= BY2WD+ERRLEN;
	memmove((char*)sp, u->note[0].msg, ERRLEN);
	sp -= 3*BY2WD;
	*(ulong*)(sp+2*BY2WD) = sp+3*BY2WD;	/* arg 2 is string */
	ur->r1 = (ulong)u->ureg;		/* arg 1 (R1) is ureg* */
	*(ulong*)(sp+1*BY2WD) = (ulong)u->ureg;	/* arg 1 0(FP) is ureg* */
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
 * Check that status is OK to return from note.
 */
int
validstatus(ulong kstatus, ulong ustatus)
{
	if((kstatus & (INTMASK|KX|SX|UX)) != (ustatus & (INTMASK|KX|SX|UX)))
		return 0;
	if((ustatus&(KSU|ERL|EXL|IE)) != (KUSER|EXL|IE))
		return 0;
	if(ustatus & (0xFFFF0000&~(CU1|CM|PE)))	/* no CU3, CU2, CU0, BEV, TS, PZ, SWC, ISC */
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
	if(arg0!=NRSTR && !u->notified) {
		qunlock(&u->p->debug);
		pprint("call to noted() when not notified\n");
		pexit("Suicide", 0);
	}
	u->notified = 0;

	nur = u->ureg;

	oureg = (ulong)nur;
	if((oureg & (BY2V-1))
	|| !okaddr((ulong)oureg-BY2WD, BY2WD+sizeof(Ureg), 0)){
		qunlock(&u->p->debug);
		pprint("bad ureg in noted or call to noted() when not notified\n");
		pexit("Suicide", 0);
	}

	if(!validstatus(kur->status, nur->status)) {
		qunlock(&u->p->debug);
		pprint("bad noted ureg status %lux\n", nur->status);
		pexit("Suicide", 0);
	}

	memmove(*urp, u->ureg, sizeof(Ureg));
	switch(arg0) {
	case NCONT:
	case NRSTR:
		if(!okaddr(nur->pc, BY2WD, 0) || !okaddr(nur->usp, BY2WD, 0)){
			qunlock(&u->p->debug);
			pprint("suicide: trap in noted\n");
			pexit("Suicide", 0);
		}
		u->ureg = (Ureg*)(*(ulong*)(oureg-BY2WD));
		qunlock(&u->p->debug);
		splhi();
		rfnote(urp);
		break;

	case NSAVE:
		if(!okaddr(nur->pc, BY2WD, 0) || !okaddr(nur->usp, BY2WD, 0)){
			qunlock(&u->p->debug);
			pprint("suicide: trap in noted\n");
			pexit("Suicide", 0);
		}
		qunlock(&u->p->debug);
		sp = oureg-4*BY2WD-ERRLEN;
		splhi();
		(*urp)->sp = sp;
		((ulong*)sp)[1] = oureg;	/* arg 1 0(FP) is ureg* */
		((ulong*)sp)[0] = 0;			/* arg 0 is pc */
		(*urp)->r1 = oureg;		/* arg 1 is ureg* */
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
	Proc *p;

	m->syscall++;
	p = u->p;
	p->insyscall = 1;
	ur = aur;
	p->pc = ur->pc;
	u->dbgreg = aur;
	ur->cause = 16<<2;	/* for debugging: system call is undef 16 */

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
	if(!waserror()) {
		if(u->scallnr >= sizeof systab/sizeof systab[0]){
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
		noted(ur, &aur, *(ulong*)(sp+BY2WD));	/* doesn't return */

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
