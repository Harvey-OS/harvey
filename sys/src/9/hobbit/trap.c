#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "ureg.h"
#include "../port/error.h"
#include "io.h"

#include "machine.h"

static char *events[16] = {
	"syscall",
	"exception",
	"niladic trap",
	"unimplemented instruction",
	"non-maskable interrupt",
	"interrupt 1",
	"interrupt 2",
	"interrupt 3",
	"interrupt 4",
	"interrupt 5",
	"interrupt 6",
	"timer 1 interrupt",
	"timer 2 interrupt",
	"FP exception",
	"event 0x0E",
	"event 0x0F",
};

static char *exceptions[16] = {
	"trap: exception 0x00",
	"trap: integer zero-divide",
	"trap: trace",
	"trap: illegal instruction",
	"trap: alignment fault",
	"trap: privilege violation",
	"trap: unimplemented register",
	"trap: fetch fault",
	"trap: read fault",
	"trap: write fault",
	"trap: text fetch I/O bus error",
	"trap: data access I/O bus error",
	"trap: exception 0x0C",
	"trap: exception 0x0D",
	"trap: exception 0x0E",
	"breakpoint",			/* "trap: exception 0x0F", */
};

void
registers(Ureg *ur)
{
	print("\n%8.8s %.8ux", "PSW", ur->psw);
	print("%8.8s %.8ux", "PC", ur->pc);
	print("%8.8s %.8ux\n", "MSP", ur->msp);
	print("%8.8s %.8ux", "SP", ur->sp);
	print("%8.8s %.8ux", "FAULT", ur->fault);
	print("%8.8s %.8ux\n", "ID", ur->id);
	print("%8.8s %.8ux", "XXX", ur->user);
	print("%8.8s %.8ux", "CAUSE", ur->cause);
	print("%8.8s %.8ux\n", "ISP", ur);
}

void
dumpstack(void)
{
}

static int zero;

static void
reboot(Ureg *ur)
{

	splhi();
	if(zero++)
		softreset();
	if(ur->cause == 0x01)
		print("\npanic: %s\n", exceptions[ur->id & 0xF]);
	else
		print("\npanic: %s\n", events[ur->cause & 0xF]);
	registers(ur);
	if(u){
		print("\n%8.8s %.8ux", "proc", u->p);
		if(u->p->mmustb)
			print("%8.8s %.8ux", "mmu", u->p->mmustb->pa);
		print("%8.8s %.8ux", "upage", u->p->upage->pa);
		print(" %8.8s\n", u->p->text);
		while(++ur < (Ureg*)(UREGADDR & ~0x0F))
			registers(ur);
	}
	exit(1);
}

void
faulthobbit(Ureg *ur, ulong addr)
{
	short insyscall;
	char buf[ERRLEN];

	if(u == 0)
		reboot(ur);
	spllo();
	insyscall = u->p->insyscall;
	u->p->insyscall = 1;
	if(fault(addr, ur->id != EXC_WRITE) == 0){
		u->p->insyscall = insyscall;
		return;
	}
	if((ur->psw & PSW_X) == 0)
		reboot(ur);
	u->p->insyscall = insyscall;
	sprint(buf, "sys: %s addr=0x%lux", exceptions[ur->id], addr);
	postnote(u->p, 1, buf, NDebug);
}

static Vector *vector[NIRQ];
static ulong irqmask;

void
setvector(Vector *v)
{
	Irq *irq = IRQADDR;

	vector[v->mask] = v;
	irqmask |= 1<<v->mask;

	irq->mask0 |= irqmask & 0xFF;
	irq->mask1 |= (irqmask>>8) & 0xFF;
}

void
interrupt(Ureg *ur)
{
	unsigned status;
	Vector *v;

	status = irqmask & (((IRQADDR->status1 & 0xFF)<<8)|(IRQADDR->status0 & 0xFF));

	if(status & (1<<IRQINCON)){
		v = vector[IRQINCON];
		(*v->handler)(v->ctlrno, ur);
		status &= ~(1<<IRQINCON);
	}

	if(status & (1<<IRQSONIC)){
		v = vector[IRQSONIC];
		(*v->handler)(v->ctlrno, ur);
		status &= ~(1<<IRQSONIC);
	}

	if(status & (1<<IRQSCSI)){
		v = vector[IRQSCSI];
		(*v->handler)(v->ctlrno, ur);
		status &= ~(1<<IRQSCSI);
	}

	if(status & (1<<IRQSCSIDMA)){
		v = vector[IRQSCSIDMA];
		(*v->handler)(v->ctlrno, ur);
		status &= ~(1<<IRQSCSIDMA);
	}
		
	if(status & (1<<IRQQUART)){
		v = vector[IRQQUART];
		(*v->handler)(v->ctlrno, ur);
		status &= ~(1<<IRQQUART);
	}

	if(status)
		panic("interrupt: #%4.4lux\n", status);
}

/*
 * this is just an efficiency hack such that 'buf'
 * doesn't appear on the stack in time-critical code
 * like trap() and syscall().
 */
static void
hobbitpostnote(Proc *p, int dolock, char *n, int flag)
{
	char buf[ERRLEN];

	sprint(buf, "sys: %s", n);
	postnote(p, dolock, buf, flag);
}

void
trap(Ureg *ur)
{
	ulong addr, *ptep;
	extern void _cret(void), _vector2(void), _vector3(void);

	if(ur->psw & PSW_X)
		u->dbgreg = ur;
	switch(ur->cause){

	case 0x01:				/* exception */
		addr = ur->fault;
		switch(ur->id){

		case EXC_FETCH:			/* fetch fault */
			/*
			 * Hobbit Mask3 bug.
			 * if CRET fails due to the target-PC being in
			 * an invalid page, the PC saved in the exception
			 * frame is that of the CRET instruction, not the
			 * target-PC.
			 * the return-to-user-mode code in l.s stashes
			 * the target-PC away in a stack armpit before
			 * trying the CRET.
			 */
			if(ur->pc == (ulong)_cret)
				ur->pc = *(ulong*)(USERADDR+SPOFFSET);
			addr = ur->pc;
			/*
			 * catch the unimplemented instruction sequence from
			 * user mode which faults trying to fetch the vector.
			 * turn it into an illegal instruction.
			 */
			if(addr == (ulong)_vector2 || addr == (ulong)_vector3){
				ur->pc = *(ulong*)ur->sp;
				ur->id = EXC_ILLINS;
				goto buggery;
			}
			/*
			 * a faulting instruction may span a page boundary.
			 * check for the condition whereby the fault happened
			 * in a valid page and the faulting pc is within
			 * the maximum instruction length of the end of the
			 * page. if true, process the fault using the va of
			 * the beginning of the following page.
			 */
			if((addr & PGOFFSET) > (BY2PG-10)){
				ptep = mmuwalk(u->p, addr, 0);
				if(ptep && (*ptep & PTE_V))
					addr = ROUNDUP(addr, BY2PG);
			}
			faulthobbit(ur, addr);
			break;

		case EXC_READ:			/* read fault */
			/*
			 * CATCH/ENTER will generate a read fault when
			 * checking the new SP/MSP values.
			 * turn this into a write fault to save on a
			 * real write fault when we attempt a store
			 * into the stack page.
			 */
			if(addr < (USTKTOP-USTKSIZE) || addr >= USTKTOP){
				faulthobbit(ur, addr);
				break;
			}
			ur->id = EXC_WRITE;
			/*FALLTHROUGH*/
		case EXC_WRITE:			/* write fault */
			faulthobbit(ur, addr);
			break;

		case EXC_PRIV:			/* privilege violation */
			/*
			 * if tracing, turn it into a breakpoint
			 */
			if(u->p->procctl == Proc_traceme)
				ur->id = 0x0F;
			/*FALLTHROUGH*/
		case EXC_ZDIV:			/* integer zero divide */
		case EXC_TRACE:			/* trace */
		case EXC_ILLINS:		/* illegal instruction */
		case EXC_ALIGN:			/* alignment fault */
		case EXC_REG:			/* unimplemented register */
		case EXC_TEXTBERR:		/* text fetch I/O bus error */
		case EXC_DATABERR:		/* data access I/O bus error */
		buggery:
		default:
			if((ur->psw & PSW_X) == 0)
				reboot(ur);
			spllo();
			hobbitpostnote(u->p, 1, exceptions[ur->id], NDebug);
		}
		break;

	default:
		reboot(ur);
	}
	splhi();
	if(ur->psw & PSW_X)
		notify(ur);
}

void
noted(Ureg *ur, ulong arg0)
{
	if((u->noteur->psw & ~(PSW_E|PSW_V|PSW_C|PSW_F)) != u->notepsw){
		pprint("bad noted ureg psw #%lux\n", u->noteur->psw);
    Die:
		pexit("Suicide", 0);
	}
	qlock(&u->p->debug);
	if(u->notified == 0){
		pprint("call to noted() when not notified\n");
		qunlock(&u->p->debug);
		return;
	}
	u->notified = 0;
	memmove(ur, u->noteur, sizeof(Ureg));
	switch(arg0){

	case NCONT:
		if(!okaddr(ur->pc, 1, 0) || !okaddr(ur->usp, BY2WD, 0)){
			pprint("suicide: trap in noted\n");
			qunlock(&u->p->debug);
			goto Die;
		}
		qunlock(&u->p->debug);
		return;

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
	if(n->flag != NUser && (u->notified || u->notify == 0)){
		if(n->flag == NDebug)
			pprint("suicide: %s\n", n->msg);
    Die:
		qunlock(&u->p->debug);
		pexit(n->msg, n->flag!=NDebug);
	}
	if (u->notified == 0) {
		if (u->notify == 0)
			goto Die;
		u->notepsw = ur->psw & ~(PSW_E|PSW_V|PSW_C|PSW_F);
		sp = ur->sp;
		sp -= QUADALIGN(sizeof(Ureg));
		if(!okaddr((ulong)u->notify, 1, 0)
		|| !okaddr(sp-ERRLEN-3*BY2WD, sizeof(Ureg)+ERRLEN-3*BY2WD, 0)){
			pprint("suicide: bad address in notify\n");
			qunlock(&u->p->debug);
			pexit("Suicide", 0);
		}
		u->noteur = (void*)sp;
		memmove((Ureg*)sp, ur, sizeof(Ureg));
		sp -= QUADALIGN(ERRLEN);
		memmove((char*)sp, u->note[0].msg, ERRLEN);
		sp -= QUADALIGN(3*BY2WD);
		*(ulong*)(sp+2*BY2WD) = sp+QUADALIGN(3*BY2WD);
		*(ulong*)(sp+1*BY2WD) = (ulong)u->noteur;
		*(ulong*)(sp+0*BY2WD) = 0;
		ur->sp = sp;
		ur->pc = (ulong)u->notify;
		u->notified = 1;
		u->nnote--;
		memmove(&u->lastnote, &u->note[0], sizeof(Note));
		memmove(&u->note[0], &u->note[1], u->nnote*sizeof(Note));
	}
	qunlock(&u->p->debug);
	splx(s);
	return 1;
}

#include "../port/systab.h"

long
syscall(Ureg *ur)
{
	Proc *p = u->p;
	long ret;

	m->syscall++;
	p->insyscall = 1;
	p->pc = ur->pc;
	u->dbgreg = ur;
	spllo();

	u->scallnr = ur->id;

	/*
	 * usurp syscall 0 to be a breakpoint trap (was sysr1()).
	 * wind back the PC.
	 * this is necessary as we need a single-parcel instruction
	 * and all other such are either valid with fixed addressing
	 * modes or take the unimplemented instruction sequence
	 * which trashes R0.
	 * and we can't use a privileged instruction ([CK]RET)
	 * due to chip bugs.
	 */
	if(u->scallnr == 0 && p->procctl == Proc_traceme){
		ur->pc -= 2;
		hobbitpostnote(p, 1, "breakpoint", NDebug);
		splhi();
		notify(ur);
		return 0;
	}

	if(p->procctl)
		procctl(p);
	u->nerrlab = 0;
	ret = -1;

	if(waserror() == 0){
		if(u->scallnr >= sizeof(systab)/sizeof(systab[0])){
			pprint("bad sys call number %d pc %lux\n", u->scallnr, ur->pc);
			hobbitpostnote(p, 1, "bad sys call", NDebug);
			error(Ebadarg);
		}

		u->s = *((Sargs*)(ur->sp+BY2WD));
		p->psstate = sysctab[u->scallnr];

		ret = (*systab[u->scallnr])(u->s.args);
		poperror();
	}

	u->nerrlab = 0;
	p->psstate = 0;
	p->insyscall = 0;

	if(u->scallnr != NOTED)				/* barf */
		*(ulong*)(ur->sp+BY2WD) = ret;
 	else						/* ugly hack */
		noted(ur, *(ulong*)(ur->sp+BY2WD));

	splhi();
	if(u->scallnr != RFORK && (p->procctl || u->nnote))
		notify(ur);
	return ret;
}

void
evenaddr(ulong addr)
{
	if(addr & (BY2WD-1)){
		postnote(u->p, 1, "sys: odd address", NDebug);
		error(Ebadarg);
	}
}

long
execregs(ulong entry, ulong ssize, ulong nargs)
{
	ulong *sp;

	sp = (ulong*)(USTKTOP - QUADALIGN(ssize+3*BY2WD));
	sp[0] = USTKTOP - BY2WD;
	sp[2] = USTKTOP - ssize;
	((Ureg*)UREGADDR)->msp = USTKTOP;	/* ??? */
	((Ureg*)UREGADDR)->sp = (ulong)sp;
	((Ureg*)UREGADDR)->pc = entry;
	return nargs;
}

ulong
userpc(void)
{
	return ((Ureg*)UREGADDR)->pc;
}

void
setregisters(Ureg *xp, char *pureg, char *uva, int n)
{
	USED(xp, pureg, uva, n);
	error(Eperm);
}
