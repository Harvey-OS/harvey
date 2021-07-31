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
void	illegal(Ureg *);
void	fen(Ureg *);

char *regname[]={
	"type",	"a0",		"a1",
	"a2",		"R0",		"R1",
	"R2",		"R3",		"R4",
	"R5",		"R6",		"R7",
	"R8",		"R9",		"R10",
	"R11",	"R12",	"R13",
	"R14",	"R15",	"R19",
	"R20",	"R21",	"R22",
	"R23",	"R24",	"R25",
	"R26",	"R27",	"R28",
	"R30",	"status",	"PC",
	"R29",	"R16",	"R17",
	"R18",
};

static Lock vctllock;
static Vctl *vctl[256];

void
intrenable(int irq, void (*f)(Ureg*, void*), void* a, int tbdf, char *name)
{
	int vno;
	Vctl *v, *p;

	v = xalloc(sizeof(Vctl));
	v->isintr = 1;
	v->irq = irq;
	v->tbdf = tbdf;
	v->f = f;
	v->a = a;
	strncpy(v->name, name, NAMELEN-1);
	v->name[NAMELEN-1] = 0;

	ilock(&vctllock);
	vno = arch->intrenable(v);
	if(vno == -1){
		iunlock(&vctllock);
		print("intrenable: couldn't enable irq %d, tbdf 0x%uX for %s\n",
			irq, tbdf, v->name);
		if(p=vctl[vno]){
			print("intrenable: irq %d is already used by", irq);
			for(; p; p=p->next)
				print(" %s", p->name);
			print("\n");
		}
		xfree(v);
		return;
	}
	if(vctl[vno]){
		if(vctl[vno]->isr != v->isr || vctl[vno]->eoi != v->eoi)
			panic("intrenable: handler: %s %s %luX %luX %luX %luX\n",
				vctl[vno]->name, v->name,
				vctl[vno]->isr, v->isr, vctl[vno]->eoi, v->eoi);
		v->next = vctl[vno];
	}
	vctl[vno] = v;
	iunlock(&vctllock);
}

int
irqallocread(char *buf, long n, vlong offset)
{
	int vno;
	Vctl *v;
	long oldn;
	char str[11+1+NAMELEN+1], *p;
	int m;

	if(n < 0 || offset < 0)
		error(Ebadarg);

	oldn = n;
	for(vno=0; vno<nelem(vctl); vno++){
		for(v=vctl[vno]; v; v=v->next){
			m = snprint(str, sizeof str, "%11d %11d %.*s\n", vno, v->irq, NAMELEN, v->name);
			if(m <= offset)	/* if do not want this, skip entry */
				offset -= m;
			else{
				/* skip offset bytes */
				m -= offset;
				p = str+offset;
				offset = 0;

				/* write at most max(n,m) bytes */
				if(m > n)
					m = n;
				memmove(buf, p, m);
				n -= m;
				buf += m;

				if(n == 0)
					return oldn;
			}	
		}
	}
	return oldn - n;
}

typedef struct Mcheck Mcheck;
struct Mcheck
{
	ulong	len;
	ulong	inprogress;
	ulong	procoff;
	ulong	sysoff;
	ulong	code;
};

static char *
smcheck(ulong code)
{
	switch (code) {
	case 0x80: return "tag parity error";
	case 0x82: return "tag control parity error";
	case 0x84: return "generic hard error";
	case 0x86: return "correctable ECC error";
	case 0x88: return "uncorrectable ECC error";
	case 0x8a: return "OS-specific PAL bugcheck";
	case 0x90: return "callsys in kernel mode";
	case 0x96: return "i-cache read retryable error";
	case 0x98: return "processor detected hard error";

	case 0x203: return "system detected uncorrectable ECC error";
	case 0x205: return "parity error detected by CIA";
	case 0x207: return "non-existent memory error";
	case 0x209: return "PCI SERR detected";
	case 0x20b: return "PCI data parity error detected";
	case 0x20d: return "PCI address parity error detected";
	case 0x20f: return "PCI master abort error";
	case 0x211: return "PCI target abort error";
	case 0x213: return "scatter/gather PTE invalid error";
	case 0x215: return "flash ROM write error";
	case 0x217: return "IOA timeout detected";
	case 0x219: return "IOCHK#, EISA add-in board parity or other catastrophic error";
	case 0x21b: return "EISA fail-safe timer timeout";
	case 0x21d: return "EISA bus time-out";
	case 0x21f: return "EISA software generated NMI";
	case 0x221: return "unexpected ev5 IRQ[3] interrupt";
	default: return "unknown mcheck";
	}
}

void
mcheck(void *x)
{
	Mcheck *m;
	uvlong *data;
	int i, col;

	m = x;
	data = x;
	iprint("panic: Machine Check @%lux: %s (%lux) len %d\n",
			m, smcheck(m->code), m->code, m->len);
	iprint("proc offset %lux sys offset %lux\n", m->procoff, m->sysoff);
	for (i = 0, col = 0; i < m->len/8; i++) {
		iprint("%.3lux: %.16llux%s", 8*i, data[i], (col == 2) ? "\n" : "    ");
		if (col++ == 2)
			col = 0;
	}
	firmware();
}

void
intr(Ureg *ur)
{
	int i, vno;
	Vctl *ctl, *v;
	Mach *mach;

	vno = (ulong)ur->a1>>4;
	vno -= 0x80;
	if(vno < nelem(vctl) && (ctl = vctl[vno])){
		if(ctl->isintr){
			m->intr++;
			if(vno >= VectorPIC && vno <= MaxVectorPIC)
				m->lastintr = vno-VectorPIC;
		}

		if(ctl->isr)
			ctl->isr(vno);
		for(v = ctl; v != nil; v = v->next) {
			if(v->f)
				v->f(ur, v->a);
		}

		if(ctl->eoi)
			ctl->eoi(vno);

		/* 
		 *  preemptive scheduling.  to limit stack depth,
		 *  make sure process has a chance to return from
		 *  the current interrupt before being preempted a
		 *  second time.
		 */
		if(ctl->isintr)
		if(up && up->state == Running)
		if(anyhigher())
		if(up->preempted == 0)
		if(!active.exiting){
			up->preempted = 1;
			sched();
			splhi();
			up->preempted = 0;
			return;
		}
	}
	else if(vno >= VectorPIC && vno <= MaxVectorPIC){
		/*
		 * An unknown interrupt.
		 * Check for a default IRQ7. This can happen when
		 * the IRQ input goes away before the acknowledge.
		 * In this case, a 'default IRQ7' is generated, but
		 * the corresponding bit in the ISR isn't set.
		 * In fact, just ignore all such interrupts.
		 */
		iprint("cpu%d: spurious interrupt %d, last %d",
			m->machno, vno-VectorPIC, m->lastintr);
		for(i = 0; i < 32; i++){
			if(!(active.machs & (1<<i)))
				continue;
			mach = MACHP(i);
			if(m->machno == mach->machno)
				continue;
			iprint(": cpu%d: last %d", mach->machno, mach->lastintr);
		}
		iprint("\n");
		m->spuriousintr++;
		return;
	}
	else{
		dumpregs(ur);
		panic("unknown intr: %d\n", vno); /* */
	}
}

void
trap(Ureg *ur)
{
	char buf[ERRLEN];
	int user, x;

	m->intrts = fastticks(nil);
	user = ur->status&UMODE;

	if(user){
		up = m->proc;
		up->dbgreg = ur;
	}
	switch ((int)ur->type) {
	case 1:	/* arith */
		fptrap(ur);
		break;
	case 2:	/* bad instr or FEN */
		illegal(ur);
		break;

	case 3:	/* intr */
		m->intr++;
		switch ((int)ur->a0) {
		case 0:	/* interprocessor */
			panic("interprocessor intr");
			break;
		case 1:	/* clock */
			clock(ur);
			break;
		case 2:	/* machine check */
			mcheck((void*)(KZERO|(ulong)ur->a2));
			break;
		case 3:	/* device */
			intr(ur);
			break;
		case 4:	/* perf counter */
			panic("perf count");
			break;
		default:
			panic("bad intr");
			break;
		}
		break;

	case 4:	/* memory fault */
		if(up == 0)
			kernfault(ur, (ulong)ur->a1);

		x = up->insyscall;
		up->insyscall = 1;
		spllo();
		faultalpha(ur);
		up->insyscall = x;
		break;
	case 6:	/* alignment fault */
		ur->pc -= 4;
		sprint(buf, "trap: unaligned addr 0x%lux", (ulong)ur->a0);
		fataltrap(ur, buf);
		break;
	default:	/* cannot happen */
		panic("bad trap type %d", (int)ur->type);
		break;
	}

	if(user && (up->procctl || up->nnote)){
		splhi();
		notify(ur);
	}
}

void
trapinit(void)
{
	splhi();
	wrent(0, intr0);
	wrent(1, arith);
	wrent(2, fault0);
	wrent(3, illegal0);
	wrent(4, unaligned);
	wrent(5, syscall0);
}

void
fataltrap(Ureg *ur, char *reason)
{
	char buf[ERRLEN];

	if(ur->status&UMODE) {
		spllo();
		sprint(buf, "sys: %s", reason);
		postnote(up, 1, buf, NDebug);
		return;
	}
	print("kernel %s pc=%lux\n", reason, (ulong)ur->pc);
	dumpregs(ur);
	dumpstack();
	if(m->machno == 0)
		spllo();
	exit(1);
}

void
kernfault(Ureg *ur, int code)
{
	Label l;
	char *s;

	splhi();
	if (code == 0)
		s = "read";
	else if (code == 1)
		s = "write";
	else
		s = "ifetch";
	print("panic: kfault %s VA=0x%lux\n", s, (ulong)ur->a0);
	print("u=0x%lux status=0x%lux pc=0x%lux sp=0x%lux\n",
				up, (ulong)ur->status, (ulong)ur->pc, (ulong)ur->sp);
	dumpregs(ur);
	l.sp = ur->sp;
	l.pc = ur->pc;
	dumpstack();
	exit(1);
}

void
dumpstack(void)
{
	ulong l, sl, el, v, i, instr, op;
	extern ulong etext;

	l=(ulong)&l;
	if(l&4)
		l += 4;
	if(up == 0){
		el = (ulong)m+BY2PG;
		sl = el-KSTACK;
	}
	else{
		sl = (ulong)up->kstack;
		el = sl + KSTACK;
	}
	if(l > el || l < sl){
		iprint("dumpstack: l %lux sl %lux el %lux m %lux up %lux\n",
			l, sl, el, m, up);
		el = (ulong)m+BY2PG;
		sl = el-KSTACK;
	}
	iprint("dumpstack: l %lux sl %lux el %lux m %lux\n", l, sl, el, m);
	if(l > el || l < sl){
		return;
	}

	i = 0;
	for(; l<el; l+=8){
		v = *(ulong*)l - 4;
		if(KTZERO < v && v < (ulong)&etext && (v&3) == 0){
			/*
			 * Check for JSR/BSR
			 */
			instr = *(ulong*)v;
			op = (instr>>26);
			if(op == 26 || op == 52){
				iprint("%lux ", v);
				i++;
			}
		}
		if(i == 8){
			i = 0;
			iprint("\n");
		}
	}
}

void
callwithureg(void (*fn)(Ureg*))
{
	Ureg ureg;
	ureg.pc = getcallerpc(&fn);
	ureg.sp = (ulong)&fn;
	fn(&ureg);
}

void
dumpregs(Ureg *ur)
{
	int i, col;
	uvlong *l;

	spllo();
	prflush(); 
	if(up)
		iprint("registers for %s %d\n", up->text, up->pid);
	else
		iprint("registers for kernel\n");

	l = &ur->type;
	col = 0;
	for (i = 0; i < sizeof regname/sizeof(char*); i++, l++) {
		iprint("%-7s%.16llux%s", regname[i], *l, col == 2 ? "\n" : "     ");
		if (col++ == 2)
			col = 0;
	}
	iprint("\n");
//	prflush();
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

		sprint(n->msg+l, " pc=0x%lux", (ulong)ur->pc);
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

	if(!okaddr((ulong)up->notify, BY2WD, 0)
	|| !okaddr(sp-ERRLEN-6*BY2WD, sizeof(Ureg)+ERRLEN-6*BY2WD, 1)) {
		pprint("suicide: bad address or sp in notify\n");
print("suicide: bad address or sp in notify\n");
		qunlock(&up->debug);
		pexit("Suicide", 0);
	}

	memmove((Ureg*)sp, ur, sizeof(Ureg));
	*(Ureg**)(sp-BY2WD) = up->ureg;	/* word under Ureg is old up->ureg */
	up->ureg = (void*)sp;
	sp -= 2*BY2WD+ERRLEN;
	memmove((char*)sp, up->note[0].msg, ERRLEN);
	sp -= 4*BY2WD;
	*(ulong*)(sp+3*BY2WD) = sp+4*BY2WD;	/* arg 2 is string */
	ur->r0 = (ulong)up->ureg;		/* arg 1 (R0) is ureg* */
	*(ulong*)(sp+2*BY2WD) = (ulong)up->ureg;	/* arg 1 0(FP) is ureg* */
	*(ulong*)(sp+0*BY2WD) = 0;		/* arg 0 is pc */
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
	if((kstatus & 7) != (ustatus & 7))
		return 0;
	if((ustatus&UMODE) != UMODE)
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
print("call to noted() when not notified\n");
		pexit("Suicide", 0);
	}
	up->notified = 0;

	nur = up->ureg;

	oureg = (ulong)nur;
	if((oureg & (BY2V-1))
	|| !okaddr((ulong)oureg-BY2WD, BY2WD+sizeof(Ureg), 0)){
		pprint("bad ureg in noted or call to noted() when not notified\n");
print("bad ureg in noted or call to noted() when not notified\n");
		qunlock(&up->debug);
		pexit("Suicide", 0);
	}

	if(!validstatus(kur->status, nur->status)) {
		qunlock(&up->debug);
		pprint("bad noted ureg status %lux\n", (ulong)nur->status);
print("bad noted ureg status %lux\n", (ulong)nur->status);
		pexit("Suicide", 0);
	}

	memmove(*urp, up->ureg, sizeof(Ureg));
	switch(arg0) {
	case NCONT:
	case NRSTR:
		if(!okaddr(nur->pc, BY2WD, 0) || !okaddr(nur->usp, BY2WD, 0)){
			pprint("suicide: trap in noted\n");
print("suicide: trap in noted\n");
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
print("suicide: trap in noted\n");
			qunlock(&up->debug);
			pexit("Suicide", 0);
		}
		qunlock(&up->debug);
		sp = oureg-4*BY2WD-ERRLEN;
		splhi();
		(*urp)->sp = sp;
		((ulong*)sp)[1] = oureg;	/* arg 1 0(FP) is ureg* */
		((ulong*)sp)[0] = 0;			/* arg 0 is pc */
		(*urp)->r0 = oureg;		/* arg 1 is ureg* */
		rfnote(urp);
		break;

	default:
		pprint("unknown noted arg 0x%lux\n", arg0);
print("unknown noted arg 0x%lux\n", arg0);
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
	up = m->proc;
	up->insyscall = 1;
	ur = aur;
	up->pc = ur->pc;
	up->dbgreg = aur;
	ur->type = 5;		/* for debugging */

	up->scallnr = ur->r0;

	if(up->scallnr == RFORK && up->fpstate == FPactive){
		savefpregs(&up->fpsave);
		up->fpstate = FPinactive;
//print("SR=%lux+", up->fpsave.fpstatus);
	}
	spllo();

	sp = ur->sp;
	up->nerrlab = 0;
	ret = -1;
	if(waserror())
		goto error;

	if(up->scallnr >= nsyscall){
		pprint("bad sys call %d pc %lux\n",
			up->scallnr, (ulong)ur->pc);
print("bad sys call %d pc %lux\n", up->scallnr, (ulong)ur->pc);
		postnote(up, 1, "sys: bad sys call", NDebug);
		error(Ebadarg);
	}

	if(sp & (BY2WD-1)){	/* XXX too weak? */
		pprint("odd sp in sys call pc %lux sp %lux\n",
			(ulong)ur->pc, (ulong)ur->sp);
		postnote(up, 1, "sys: odd stack", NDebug);
		error(Ebadarg);
	}

	if(sp<(USTKTOP-BY2PG) || sp>(USTKTOP-sizeof(Sargs)))
		validaddr(sp, sizeof(Sargs), 0);

	up->s = *((Sargs*)(sp+2*BY2WD));
	up->psstate = sysctab[up->scallnr];

	ret = (*systab[up->scallnr])(up->s.args);
	poperror();

error:
	up->nerrlab = 0;
	up->psstate = 0;
	up->insyscall = 0;
	if(up->scallnr == NOTED)			/* ugly hack */
		noted(ur, &aur, *(ulong*)(sp+2*BY2WD));	/* doesn't return */

	if(up->scallnr!=RFORK && (up->procctl || up->nnote)){
		ur->r0 = ret;				/* load up for noted() */
		if(notify(ur))
			return ur->r0;
	}

	return ret;
}

void
forkchild(Proc *p, Ureg *ur)
{
	Ureg *cur;

	p->sched.sp = (ulong)p->kstack+KSTACK-(4*BY2WD+sizeof(Ureg));
	p->sched.pc = (ulong)forkret;

	cur = (Ureg*)(p->sched.sp+4*BY2WD);
	memmove(cur, ur, sizeof(Ureg));

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
	ur->pc = entry;
	return USTKTOP-BY2WD;			/* address of user-level clock */
}

ulong
userpc(void)
{
	Ureg *ur;

	ur = (Ureg*)up->dbgreg;
	return ur->pc;
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

/* Give enough context in the ureg to produce a kernel stack for
 * a sleeping process
 */
void
setkernur(Ureg *xp, Proc *p)
{
	xp->pc = p->sched.pc;
	xp->sp = p->sched.sp;
	xp->r26 = (ulong)sched;
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

void
illegal(Ureg *ur)
{
	switch ((int)ur->a0) {
	case 0:	/* breakpoint */
		ur->pc -= 4;
		fataltrap(ur, "breakpoint");
		break;
	case 1:	/* bugchk */
		fataltrap(ur, "trap: bugchk");
		break;
	case 2:	/* gentrap */
		fataltrap(ur, "trap: gentrap");
		break;
	case 3:	/* FEN */
		fen(ur);
		break;
	case 4:	/* opDEC */
		fataltrap(ur, "trap: illegal instruction");
		break;
	default:
		panic("illegal illegal %d", (int)ur->a0);
		break;
	}
}

void
fen(Ureg *ur)
{
	if(up){
		switch(up->fpstate){
		case FPinit:
			restfpregs(&initfp);
			up->fpstate = FPactive;
//print("EI=%lux+", initfp.fpstatus);
			return;

		case FPinactive:
			restfpregs(&up->fpsave);
			up->fpstate = FPactive;
//print("EIA=%lux+", up->fpsave.fpstatus);
			return;
		}
	}
	fataltrap(ur, "trap: floating enable");	/* should never happen */
}
