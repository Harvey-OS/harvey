#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"../port/error.h"
#include	"tos.h"

Intrregs *intrregs;

typedef struct Vctl {
	Vctl*	next;			/* handlers on this vector */
	char	*name;		/* of driver, xallocated */
	void	(*f)(Ureg*, void*);	/* handler to call */
	void*	a;			/* argument to call it with */
} Vctl;

static Lock	vctllock;
static Vctl	*vctl[32];
static Vctl	*gpiovctl[27];
static int	gpioirqref[12];

/*
 *   Layout at virtual address 0.
 */
typedef struct Vpage0 {
	void	(*vectors[8])(void);
	ulong	vtable[8];
} Vpage0;
Vpage0 *vpage0;

static int	irq(Ureg*);
static void	gpiointr(Ureg*, void*);

/* recover state after power suspend
 * NB: to help debugging bad suspend code,
 *     I changed some prints below to iprints,
 *     to avoid deadlocks when a panic is being
 *     issued during the suspend/resume handler.
 */
void
trapresume(void)
{
	vpage0 = (Vpage0*)EVECTORS;
	memmove(vpage0->vectors, vectors, sizeof(vpage0->vectors));
	memmove(vpage0->vtable, vtable, sizeof(vpage0->vtable));
	wbflush();
	mappedIvecEnable();
}

/*
 *  set up for exceptions
 */
void
trapinit(void)
{
	/* set up the exception vectors */
	vpage0 = (Vpage0*)EVECTORS;
	memmove(vpage0->vectors, vectors, sizeof(vpage0->vectors));
	memmove(vpage0->vtable, vtable, sizeof(vpage0->vtable));

	wbflush();

	/* use exception vectors at 0xFFFF0000 */
	mappedIvecEnable();

	/* set up the stacks for the interrupt modes */
	setr13(PsrMfiq, m->sfiq);
	setr13(PsrMirq, m->sirq);
	setr13(PsrMabt, m->sabt);
	setr13(PsrMund, m->sund);

	/* map in interrupt registers */
	intrregs = mapspecial(INTRREGS, sizeof(*intrregs));

	/* make all interrupts IRQ (i.e. not FIQ) and disable all interrupts */
	intrregs->iclr = 0;
	intrregs->icmr = 0;

	/* turn off all gpio interrupts */
	gpioregs->rising = 0;
	gpioregs->falling = 0;
	gpioregs->edgestatus = gpioregs->edgestatus;

	/* allow all enabled interrupts to take processor out of sleep mode */
	intrregs->iccr = 0;
}

void
trapdump(char *tag)
{
	iprint("%s: icip %lux icmr %lux iclr %lux iccr %lux icfp %lux\n",
		tag, intrregs->icip, intrregs->icmr, intrregs->iclr,
		intrregs->iccr, intrregs->icfp);
}

void
warnregs(Ureg *ur, char *tag)
{
	char buf[1024];
	char *e = buf+sizeof(buf);
	char *p;

	p = seprint(buf, e, "%s:\n", tag);
	p = seprint(p, e, "type 0x%.8lux psr 0x%.8lux pc 0x%.8lux\n",
		ur->type, ur->psr, ur->pc);
	p = seprint(p, e, "r0  0x%.8lux r1  0x%.8lux r2  0x%.8lux r3  0x%.8lux\n",
		ur->r0, ur->r1, ur->r2, ur->r3);
	p = seprint(p, e, "r4  0x%.8lux r5  0x%.8lux r6  0x%.8lux r7  0x%.8lux\n",
		ur->r4, ur->r5, ur->r6, ur->r7);
	p = seprint(p, e, "r8  0x%.8lux r9  0x%.8lux r10 0x%.8lux r11 0x%.8lux\n",
		ur->r8, ur->r9, ur->r10, ur->r11);
	seprint(p, e, "r12 0x%.8lux r13 0x%.8lux r14 0x%.8lux\n",
		ur->r12, ur->r13, ur->r14);
	iprint("%s", buf);
}

/*
 *  enable an irq interrupt
 */
static void
irqenable(int irq, IntrHandler *f, void* a, char *name)
{
	Vctl *v;

	if(irq >= nelem(vctl) || irq < 0)
		panic("intrenable");

	v = malloc(sizeof(Vctl));
	v->f = f;
	v->a = a;
	v->name = xalloc(strlen(name)+1);
	strcpy(v->name, name);

	lock(&vctllock);
	v->next = vctl[irq];
	vctl[irq] = v;
	intrregs->icmr |= 1<<irq;
	unlock(&vctllock);
}

/*
 *  disable an irq interrupt
 */
static void
irqdisable(int irq, IntrHandler *f, void* a, char *name)
{
	Vctl **vp, *v;

	if(irq >= nelem(vctl) || irq < 0)
		panic("intrdisable");

	lock(&vctllock);
	for(vp = &vctl[irq]; v = *vp; vp = &v->next)
		if (v->f == f && v->a == a && strcmp(v->name, name) == 0){
			print("irqdisable: remove %s\n", name);
			*vp = v->next;
			free(v);
			break;
		}
	if (v == nil)
		print("irqdisable: irq %d, name %s not enabled\n", irq, name);
	if (vctl[irq] == nil){
		print("irqdisable: clear icmr bit %d\n", irq);
		intrregs->icmr &= ~(1<<irq);
	}
	unlock(&vctllock);
}

/*
 *  enable an interrupt
 */
void
intrenable(int type, int which, IntrHandler *f, void* a, char *name)
{
	int irq;
	Vctl *v;

	if(type == IRQ){
		irqenable(which, f, a, name);
		return;
	}

	/* from here down, it must be a GPIO edge interrupt */
	irq = which;
	if(which >= nelem(gpiovctl) || which < 0)
		panic("intrenable");
	if(which > 11)
		irq = 11;

	/* the pin had better be configured as input */
	if((1<<which) & gpioregs->direction)
		panic("intrenable of output pin %d", which);

	/* create a second level vctl for the gpio edge interrupt */
	v = malloc(sizeof(Vctl));
	v->f = f;
	v->a = a;
	v->name = xalloc(strlen(name)+1);
	strcpy(v->name, name);

	lock(&vctllock);
	v->next = gpiovctl[which];
	gpiovctl[which] = v;

	/* set edge register to enable interrupt */
	switch(type){
	case GPIOboth:
		gpioregs->rising |= 1<<which;
		gpioregs->falling |= 1<<which;
		break;
	case GPIOfalling:
		gpioregs->falling |= 1<<which;
		break;
	case GPIOrising:
		gpioregs->rising |= 1<<which;
		break;
	}
	unlock(&vctllock);
	/* point the irq to the gpio interrupt handler */
	if(gpioirqref[irq]++ == 0)
		irqenable(irq, gpiointr, nil, "gpio edge");
}

/*
 *  disable an interrupt
 */
void
intrdisable(int type, int which, IntrHandler *f, void* a, char *name)
{
	int irq;
	Vctl **vp, *v;


	if(type == IRQ){
		irqdisable(which, f, a, name);
		return;
	}

	/* from here down, it must be a GPIO edge interrupt */
	irq = which;
	if(which >= nelem(gpiovctl) || which < 0)
		panic("intrdisable");
	if(which > 11)
		irq = 11;

	lock(&vctllock);
	for(vp = &gpiovctl[which]; v = *vp; vp = &v->next)
		if (v->f == f && v->a == a && strcmp(v->name, name) == 0){
			break;
		}
	if (gpiovctl[which] == nil){
		/* set edge register to enable interrupt */
		switch(type){
		case GPIOboth:
			print("intrdisable: gpio-rising+falling clear bit %d\n", which);
			gpioregs->rising &= ~(1<<which);
			gpioregs->falling &= ~(1<<which);
			break;
		case GPIOfalling:
			print("intrdisable: gpio-falling clear bit %d\n", which);
			gpioregs->falling &= ~(1<<which);
			break;
		case GPIOrising:
			print("intrdisable: gpio-rising clear bit %d\n", which);
			gpioregs->rising &= ~(1<<which);
			break;
		}
	
	}
	if (v) {
		print("intrdisable: removing %s\n", name);
		*vp = v->next;
	}else
		print("intrdisable: which %d, name %s not enabled\n", which, name);
	unlock(&vctllock);
	/* disable the gpio interrupt handler if necessary */
	if(--gpioirqref[irq] == 0){
		print("intrdisable: inrqdisable gpiointr\n");
		irqdisable(irq, gpiointr, nil, "gpio edge");
	}
	free(v);
}

/*
 *  called by trap to handle access faults
 */
static void
faultarm(Ureg *ureg, ulong va, int user, int read)
{
	int n, insyscall;
	char buf[ERRMAX];

	if (up == nil) {
		warnregs(ureg, "kernel fault");
		panic("fault: nil up in faultarm, accessing 0x%lux", va);
	}
	insyscall = up->insyscall;
	up->insyscall = 1;
	n = fault(va, read);
	if(n < 0){
		if(!user){
			warnregs(ureg, "kernel fault");
			panic("fault: kernel accessing 0x%lux", va);
		}
//		warnregs(ureg, "user fault");
		sprint(buf, "sys: trap: fault %s va=0x%lux", read ? "read" : "write", va);
		postnote(up, 1, buf, NDebug);
	}
	up->insyscall = insyscall;
}

/*
 *  returns 1 if the instruction writes memory, 0 otherwise
 */
int
writetomem(ulong inst)
{
	/* swap always write memory */
	if((inst & 0x0FC00000) == 0x01000000)
		return 1;

	/* loads and stores are distinguished by bit 20 */
	if(inst & (1<<20))
		return 0;

	return 1;
}


/*
 *  here on all exceptions other than syscall (SWI)
 */
void
trap(Ureg *ureg)
{
	ulong inst;
	int clockintr, user, x, rv;
	ulong va, fsr;
	char buf[ERRMAX];
	int rem;

	if(up != nil)
		rem = ((char*)ureg)-up->kstack;
	else
		rem = ((char*)ureg)-((char*)(MACHADDR+sizeof(Mach)));
	if(rem < 256) {
		dumpstack();
		panic("trap %d bytes remaining, up = %#p, ureg = %#p, at pc 0x%lux",
			rem, up, ureg, ureg->pc);
	}

	user = (ureg->psr & PsrMask) == PsrMusr;

	/*
	 * All interrupts/exceptions should be resumed at ureg->pc-4,
	 * except for Data Abort which resumes at ureg->pc-8.
	 */
	if(ureg->type == (PsrMabt+1))
		ureg->pc -= 8;
	else
		ureg->pc -= 4;

	clockintr = 0;
	switch(ureg->type){
	default:
		panic("unknown trap");
		break;
	case PsrMirq:
		clockintr = irq(ureg);
		break;
	case PsrMabt:	/* prefetch fault */
		faultarm(ureg, ureg->pc, user, 1);
		break;
	case PsrMabt+1:	/* data fault */
		va = getfar();
		inst = *(ulong*)(ureg->pc);
		fsr = getfsr() & 0xf;
		switch(fsr){
		case 0x0:
			panic("vector exception at %lux", ureg->pc);
			break;
		case 0x1:
		case 0x3:
			if(user){
				snprint(buf, sizeof(buf), "sys: alignment: pc 0x%lux va 0x%lux\n",
					ureg->pc, va);
				postnote(up, 1, buf, NDebug);
			} else
				panic("kernel alignment: pc 0x%lux va 0x%lux", ureg->pc, va);
			break;
		case 0x2:
			panic("terminal exception at %lux", ureg->pc);
			break;
		case 0x4:
		case 0x6:
		case 0x8:
		case 0xa:
		case 0xc:
		case 0xe:
			panic("external abort 0x%lux pc 0x%lux addr 0x%lux", fsr, ureg->pc, va);
			break;
		case 0x5:
		case 0x7:
			/* translation fault, i.e., no pte entry */
			faultarm(ureg, va, user, !writetomem(inst));
			break;
		case 0x9:
		case 0xb:
			/* domain fault, accessing something we shouldn't */
			if(user){
				sprint(buf, "sys: access violation: pc 0x%lux va 0x%lux\n",
					ureg->pc, va);
				postnote(up, 1, buf, NDebug);
			} else
				panic("kernel access violation: pc 0x%lux va 0x%lux",
					ureg->pc, va);
			break;
		case 0xd:
		case 0xf:
			/* permission error, copy on write or real permission error */
			faultarm(ureg, va, user, !writetomem(inst));
			break;
		}
		break;
	case PsrMund:	/* undefined instruction */
		if (user) {
			/* look for floating point instructions to interpret */
			x = spllo();
			rv = fpiarm(ureg);
			splx(x);
			if (rv == 0) {
				sprint(buf, "undefined instruction: pc 0x%lux\n", ureg->pc);
				postnote(up, 1, buf, NDebug);
			}
		}else{
			iprint("undefined instruction: pc=0x%lux, inst=0x%lux, 0x%lux, 0x%lux, 0x%lux, 0x%lux\n", ureg->pc, ((ulong*)ureg->pc)[-2], ((ulong*)ureg->pc)[-1], ((ulong*)ureg->pc)[0], ((ulong*)ureg->pc)[1], ((ulong*)ureg->pc)[2]);
			panic("undefined instruction");
		}
		break;
	}
	splhi();

	/* delaysched set because we held a lock or because our quantum ended */
	if(up && up->delaysched && clockintr){
		sched();
		splhi();
	}

	if(user){
		if(up->procctl || up->nnote)
			notify(ureg);
		kexit(ureg);
	}
}

/*
 *  here on irq's
 */
static int
irq(Ureg *ur)
{
	ulong va;
	int clockintr, i;
	Vctl *v;

	va = intrregs->icip;

	if(va & (1<<IRQtimer0))
		clockintr = 1;
	else
		clockintr = 0;
	for(i = 0; i < 32; i++){
		if(((1<<i) & va) == 0)
			continue;
		for(v = vctl[i]; v != nil; v = v->next){
			v->f(ur, v->a);
			va &= ~(1<<i);
		}
	}
	if(va)
		print("unknown interrupt: %lux\n", va);

	return clockintr;
}

/*
 *  here on gpio interrupts
 */
static void
gpiointr(Ureg *ur, void*)
{
	ulong va;
	int i;
	Vctl *v;

	va = gpioregs->edgestatus;
	gpioregs->edgestatus = va;

	for(i = 0; i < 27; i++){
		if(((1<<i) & va) == 0)
			continue;
		for(v = gpiovctl[i]; v != nil; v = v->next){
			v->f(ur, v->a);
			va &= ~(1<<i);
		}
	}
	if(va)
		print("unknown gpio interrupt: %lux\n", va);
}

/*
 *  system calls
 */
#include "../port/systab.h"

/*
 *  Syscall is called directly from assembler without going through trap().
 */
void
syscall(Ureg* ureg)
{
	char *e;
	ulong	sp;
	long	ret;
	int	i, scallnr;

	if((ureg->psr & PsrMask) != PsrMusr) {
		panic("syscall: pc 0x%lux r14 0x%lux cs 0x%lux", ureg->pc, ureg->r14, ureg->psr);
	}

	m->syscall++;
	up->insyscall = 1;
	up->pc = ureg->pc;
	up->dbgreg = ureg;

	scallnr = ureg->r0;
	up->scallnr = scallnr;
	spllo();

	sp = ureg->sp;
	up->nerrlab = 0;
	ret = -1;
	if(!waserror()){
		if(scallnr >= nsyscall){
			pprint("bad sys call number %d pc %lux\n",
				scallnr, ureg->pc);
			postnote(up, 1, "sys: bad sys call", NDebug);
			error(Ebadarg);
		}

		if(sp<(USTKTOP-BY2PG) || sp>(USTKTOP-sizeof(Sargs)-BY2WD))
			validaddr(sp, sizeof(Sargs)+BY2WD, 0);

		up->s = *((Sargs*)(sp+BY2WD));
		up->psstate = sysctab[scallnr];

		ret = systab[scallnr](up->s.args);
		poperror();
	}else{
		/* failure: save the error buffer for errstr */
		e = up->syserrstr;
		up->syserrstr = up->errstr;
		up->errstr = e;
	}
	if(up->nerrlab){
		print("bad errstack [%d]: %d extra\n", scallnr, up->nerrlab);
		for(i = 0; i < NERR; i++)
			print("sp=%lux pc=%lux\n",
				up->errlab[i].sp, up->errlab[i].pc);
		panic("error stack");
	}

	up->insyscall = 0;
	up->psstate = 0;

	/*
	 *  Put return value in frame.  On the x86 the syscall is
	 *  just another trap and the return value from syscall is
	 *  ignored.  On other machines the return value is put into
	 *  the results register by caller of syscall.
	 */
	ureg->r0 = ret;

	if(scallnr == NOTED)
		noted(ureg, *(ulong*)(sp+BY2WD));

	if(up->delaysched)
		sched();

	splhi();
	if(scallnr != RFORK && (up->procctl || up->nnote))
		notify(ureg);
}

/*
 *   Return user to state before notify()
 */
void
noted(Ureg* ureg, ulong arg0)
{
	Ureg *nureg;
	ulong oureg, sp;

	qlock(&up->debug);
	if(arg0!=NRSTR && !up->notified) {
		qunlock(&up->debug);
		pprint("call to noted() when not notified\n");
		pexit("Suicide", 0);
	}
	up->notified = 0;

	nureg = up->ureg;	/* pointer to user returned Ureg struct */

	/* sanity clause */
	oureg = (ulong)nureg;
	if(!okaddr((ulong)oureg-BY2WD, BY2WD+sizeof(Ureg), 0)){
		pprint("bad ureg in noted or call to noted when not notified\n");
		qunlock(&up->debug);
		pexit("Suicide", 0);
	}

	/* don't let user change system flags */
	nureg->psr = (ureg->psr & ~(PsrMask|PsrDfiq|PsrDirq)) |
			(nureg->psr & (PsrMask|PsrDfiq|PsrDirq));

	memmove(ureg, nureg, sizeof(Ureg));

	switch(arg0){
	case NCONT:
	case NRSTR:
		if(!okaddr(nureg->pc, 1, 0) || !okaddr(nureg->sp, BY2WD, 0)){
			qunlock(&up->debug);
			pprint("suicide: trap in noted\n");
			pexit("Suicide", 0);
		}
		up->ureg = (Ureg*)(*(ulong*)(oureg-BY2WD));
		qunlock(&up->debug);
		break;

	case NSAVE:
		if(!okaddr(nureg->pc, BY2WD, 0)
		|| !okaddr(nureg->sp, BY2WD, 0)){
			qunlock(&up->debug);
			pprint("suicide: trap in noted\n");
			pexit("Suicide", 0);
		}
		qunlock(&up->debug);
		sp = oureg-4*BY2WD-ERRMAX;
		splhi();
		ureg->sp = sp;
		((ulong*)sp)[1] = oureg;	/* arg 1 0(FP) is ureg* */
		((ulong*)sp)[0] = 0;		/* arg 0 is pc */
		break;

	default:
		pprint("unknown noted arg 0x%lux\n", arg0);
		up->lastnote.flag = NDebug;
		/* fall through */
		
	case NDFLT:
		if(up->lastnote.flag == NDebug){ 
			qunlock(&up->debug);
			pprint("suicide: %s\n", up->lastnote.msg);
		} else
			qunlock(&up->debug);
		pexit(up->lastnote.msg, up->lastnote.flag!=NDebug);
	}
}

/*
 *  Call user, if necessary, with note.
 *  Pass user the Ureg struct and the note on his stack.
 */
int
notify(Ureg* ureg)
{
	int l;
	ulong s, sp;
	Note *n;

	if(up->procctl)
		procctl(up);
	if(up->nnote == 0)
		return 0;

	s = spllo();
	qlock(&up->debug);
	up->notepending = 0;
	n = &up->note[0];
	if(strncmp(n->msg, "sys:", 4) == 0){
		l = strlen(n->msg);
		if(l > ERRMAX-15)	/* " pc=0x12345678\0" */
			l = ERRMAX-15;
		sprint(n->msg+l, " pc=0x%.8lux", ureg->pc);
	}

	if(n->flag!=NUser && (up->notified || up->notify==0)){
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
		
	if(!up->notify){
		qunlock(&up->debug);
		pexit(n->msg, n->flag!=NDebug);
	}
	sp = ureg->sp;
	sp -= sizeof(Ureg);

	if(!okaddr((ulong)up->notify, 1, 0)
	|| !okaddr(sp-ERRMAX-4*BY2WD, sizeof(Ureg)+ERRMAX+4*BY2WD, 1)){
		pprint("suicide: bad address in notify\n");
		qunlock(&up->debug);
		pexit("Suicide", 0);
	}

	up->ureg = (void*)sp;
	memmove((Ureg*)sp, ureg, sizeof(Ureg));
	*(Ureg**)(sp-BY2WD) = up->ureg;	/* word under Ureg is old up->ureg */
	up->ureg = (void*)sp;
	sp -= BY2WD+ERRMAX;
	memmove((char*)sp, up->note[0].msg, ERRMAX);
	sp -= 3*BY2WD;
	*(ulong*)(sp+2*BY2WD) = sp+3*BY2WD;	/* arg 2 is string */
	*(ulong*)(sp+1*BY2WD) = (ulong)up->ureg;	/* arg 1 is ureg* */
	*(ulong*)(sp+0*BY2WD) = 0;		/* arg 0 is pc */
	ureg->sp = sp;
	ureg->pc = (ulong)up->notify;
	up->notified = 1;
	up->nnote--;
	memmove(&up->lastnote, &up->note[0], sizeof(Note));
	memmove(&up->note[0], &up->note[1], up->nnote*sizeof(Note));

	qunlock(&up->debug);
	splx(s);
	return 1;
}

/* Give enough context in the ureg to produce a kernel stack for
 * a sleeping process
 */
void
setkernur(Ureg *ureg, Proc *p)
{
	ureg->pc = p->sched.pc;
	ureg->sp = p->sched.sp+4;
	ureg->r14 = (ulong)sched;
}

/*
 *  return the userpc the last exception happened at
 */
ulong
userpc(void)
{
	Ureg *ureg;

	ureg = (Ureg*)up->dbgreg;
	return ureg->pc;
}

/* This routine must save the values of registers the user is not permitted
 * to write from devproc and then restore the saved values before returning.
 */
void
setregisters(Ureg* ureg, char* pureg, char* uva, int n)
{
	USED(ureg, pureg, uva, n);
}

/*
 *  this is the body for all kproc's
 */
static void
linkproc(void)
{
	spllo();
	up->kpfun(up->kparg);
	pexit("kproc exiting", 0);
}

/*
 *  setup stack and initial PC for a new kernel proc.  This is architecture
 *  dependent because of the starting stack location
 */
void
kprocchild(Proc *p, void (*func)(void*), void *arg)
{
	p->sched.pc = (ulong)linkproc;
	p->sched.sp = (ulong)p->kstack+KSTACK;

	p->kpfun = func;
	p->kparg = arg;
}


/* 
 *  Craft a return frame which will cause the child to pop out of
 *  the scheduler in user mode with the return register zero.  Set
 *  pc to point to a l.s return function.
 */
void
forkchild(Proc *p, Ureg *ureg)
{
	Ureg *cureg;

//print("%lud setting up for forking child %lud\n", up->pid, p->pid);
	p->sched.sp = (ulong)p->kstack+KSTACK-sizeof(Ureg);
	p->sched.pc = (ulong)forkret;

	cureg = (Ureg*)(p->sched.sp);
	memmove(cureg, ureg, sizeof(Ureg));

	/* syscall returns 0 for child */
	cureg->r0 = 0;

	/* Things from bottom of syscall which were never executed */
	p->psstate = 0;
	p->insyscall = 0;
}

/*
 *  setup stack, initial PC, and any arch dependent regs for an execing user proc.
 */
long
execregs(ulong entry, ulong ssize, ulong nargs)
{
	ulong *sp;
	Ureg *ureg;

	sp = (ulong*)(USTKTOP - ssize);
	*--sp = nargs;

	ureg = up->dbgreg;
	memset(ureg, 0, 15*sizeof(ulong));
	ureg->r13 = (ulong)sp;
	ureg->pc = entry;
//print("%lud: EXECREGS pc 0x%lux sp 0x%lux\n", up->pid, ureg->pc, ureg->r13);
	return USTKTOP-sizeof(Tos);		/* address of kernel/user shared data */
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
	ureg.sp = (ulong)&fn;
	fn(&ureg);
}

static void
_dumpstack(Ureg *ureg)
{
	ulong l, v, i;
	ulong *p;
	extern ulong etext;

	if(up == 0){
		iprint("no current proc\n");
		return;
	}

	iprint("ktrace /kernel/path %.8lux %.8lux %.8lux\n", ureg->pc, ureg->sp, ureg->r14);
	i = 0;
	for(l=(ulong)&l; l<(ulong)(up->kstack+KSTACK); l+=4){
		v = *(ulong*)l;
		if(KTZERO < v && v < (ulong)&etext && (v&3)==0){
			v -= 4;
			p = (ulong*)v;
			if((*p & 0x0f000000) == 0x0b000000){
				iprint("%.8lux=%.8lux ", l, v);
				i++;
			}
		}
		if(i == 4){
			i = 0;
			iprint("\n");
		}
	}
	if(i)
		iprint("\n");
}

void
dumpstack(void)
{
	callwithureg(_dumpstack);
}

/*
 *  pc output by ps
 */
ulong
dbgpc(Proc *p)
{
	Ureg *ureg;

	ureg = p->dbgreg;
	if(ureg == 0)
		return 0;

	return ureg->pc;
}

/*
 * called in sysfile.c
 */
void
evenaddr(ulong addr)
{
	if(addr & 3){
		postnote(up, 1, "sys: odd address", NDebug);
		error(Ebadarg);
	}
}
