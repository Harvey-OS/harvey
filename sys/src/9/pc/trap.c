#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"../port/error.h"

void	noted(Ureg*, ulong);

void	intr0(void), intr1(void), intr2(void), intr3(void);
void	intr4(void), intr5(void), intr6(void), intr7(void);
void	intr8(void), intr9(void), intr10(void), intr11(void);
void	intr12(void), intr13(void), intr14(void), intr15(void);
void	intr16(void);
void	intr24(void), intr25(void), intr26(void), intr27(void);
void	intr28(void), intr29(void), intr30(void), intr31(void);
void	intr32(void), intr33(void), intr34(void), intr35(void);
void	intr36(void), intr37(void), intr38(void), intr39(void);
void	intr64(void);
void	intrbad(void);

int	int0mask = 0xff;	/* interrupts enabled for first 8259 */
int	int1mask = 0xff;	/* interrupts enabled for second 8259 */

/*
 *  trap/interrupt gates
 */
Segdesc ilt[256];
int badintr[16];

enum 
{
	Maxhandler=	128,		/* max number of interrupt handlers */
};

typedef struct Handler	Handler;
struct Handler
{
	void	(*r)(void*, void*);
	void	*arg;
	Handler	*next;
};

struct
{
	Lock;
	Handler	*ivec[256];
	Handler	h[Maxhandler];
	int	free;
} halloc;

void
sethvec(int v, void (*r)(void), int type, int pri)
{
	ilt[v].d0 = ((ulong)r)&0xFFFF|(KESEL<<16);
	ilt[v].d1 = ((ulong)r)&0xFFFF0000|SEGP|SEGPL(pri)|type;
}

void
setvec(int v, void (*r)(Ureg*, void*), void *arg)
{
	Handler *h;

	lock(&halloc);
	if(halloc.free >= Maxhandler)
		panic("out of interrupt handlers");
	h = &halloc.h[halloc.free++];
	h->next = halloc.ivec[v];
	h->r = r;
	h->arg = arg;
	halloc.ivec[v] = h;
	unlock(&halloc);

	/*
	 *  enable corresponding interrupt in 8259
	 */
	if((v&~0x7) == Int0vec){
		int0mask &= ~(1<<(v&7));
		outb(Int0aux, int0mask);
	} else if((v&~0x7) == Int1vec){
		int1mask &= ~(1<<(v&7));
		outb(Int1aux, int1mask);
	}
}

void
debugbpt(Ureg *ur, void *a)
{
	char buf[ERRLEN];

	USED(a);

	if(u == 0)
		panic("kernel bpt");
	/* restore pc to instruction that caused the trap */
	ur->pc--;
	sprint(buf, "sys: breakpoint");
	postnote(u->p, 1, buf, NDebug);
}

/*
 *  set up the interrupt/trap gates
 */
void
trapinit(void)
{
	int i;

	/*
	 *  set all interrupts to panics
	 */
	for(i = 0; i < 256; i++)
		sethvec(i, intrbad, SEGTG, 0);

	/*
	 *  80386 processor (and coprocessor) traps
	 */
	sethvec(0, intr0, SEGTG, 0);
	sethvec(1, intr1, SEGTG, 0);
	sethvec(2, intr2, SEGTG, 0);
	sethvec(4, intr4, SEGTG, 0);
	sethvec(5, intr5, SEGTG, 0);
	sethvec(6, intr6, SEGTG, 0);
	sethvec(7, intr7, SEGTG, 0);
	sethvec(8, intr8, SEGTG, 0);
	sethvec(9, intr9, SEGTG, 0);
	sethvec(10, intr10, SEGTG, 0);
	sethvec(11, intr11, SEGTG, 0);
	sethvec(12, intr12, SEGTG, 0);
	sethvec(13, intr13, SEGTG, 0);
	sethvec(14, intr14, SEGIG, 0);	/* page fault, interrupts off */
	sethvec(15, intr15, SEGTG, 0);
	sethvec(16, intr16, SEGIG, 0);	/* math coprocessor, interrupts off */

	/*
	 *  device interrupts
	 */
	sethvec(24, intr24, SEGIG, 0);
	sethvec(25, intr25, SEGIG, 0);
	sethvec(26, intr26, SEGIG, 0);
	sethvec(27, intr27, SEGIG, 0);
	sethvec(28, intr28, SEGIG, 0);
	sethvec(29, intr29, SEGIG, 0);
	sethvec(30, intr30, SEGIG, 0);
	sethvec(31, intr31, SEGIG, 0);
	sethvec(32, intr32, SEGIG, 0);
	sethvec(33, intr33, SEGIG, 0);
	sethvec(34, intr34, SEGIG, 0);
	sethvec(35, intr35, SEGIG, 0);
	sethvec(36, intr36, SEGIG, 0);
	sethvec(37, intr37, SEGIG, 0);
	sethvec(38, intr38, SEGIG, 0);
	sethvec(39, intr39, SEGIG, 0);

	/*
	 *  system calls and break points
	 */
	sethvec(Syscallvec, intr64, SEGTG, 3);
	setvec(Syscallvec, (void (*)(Ureg*, void*))syscall, 0);
	sethvec(Bptvec, intr3, SEGTG, 3);
	setvec(Bptvec, debugbpt, 0);

	/*
	 *  tell the hardware where the table is (and how long)
	 */
	putidt(ilt, sizeof(ilt));

	/*
	 *  Set up the first 8259 interrupt processor.
	 *  Make 8259 interrupts start at CPU vector Int0vec.
	 *  Set the 8259 as master with edge triggered
	 *  input with fully nested interrupts.
	 */
	outb(Int0ctl, (1<<4)|(0<<3)|(1<<0));	/* ICW1 - edge triggered, master,
					   ICW4 will be sent */
	outb(Int0aux, Int0vec);		/* ICW2 - interrupt vector offset */
	outb(Int0aux, 0x04);		/* ICW3 - have slave on level 2 */
	outb(Int0aux, 0x01);		/* ICW4 - 8086 mode, not buffered */

	/*
	 *  Set up the second 8259 interrupt processor.
	 *  Make 8259 interrupts start at CPU vector Int0vec.
	 *  Set the 8259 as master with edge triggered
	 *  input with fully nested interrupts.
	 */
	outb(Int1ctl, (1<<4)|(0<<3)|(1<<0));	/* ICW1 - edge triggered, master,
					   ICW4 will be sent */
	outb(Int1aux, Int1vec);		/* ICW2 - interrupt vector offset */
	outb(Int1aux, 0x02);		/* ICW3 - I am a slave on level 2 */
	outb(Int1aux, 0x01);		/* ICW4 - 8086 mode, not buffered */

	/*
	 *  pass #2 8259 interrupts to #1
	 */
	int0mask &= ~0x04;
	outb(Int0aux, int0mask);

	/*
	 * Set Ocw3 to return the ISR when ctl read.
	 */
	outb(Int0ctl, Ocw3|0x03);
	outb(Int1ctl, Ocw3|0x03);
}

char *excname[] = {
	[0]	"divide error",
	[1]	"debug exception",
	[2]	" nonmaskable interrupt",
	[3]	"breakpoint",
	[4]	"overflow",
	[5]	"bounds check",
	[6]	"invalid opcode",
	[7]	"coprocessor not available",
	[8]	"double fault",
	[9]	"9 (reserved)",
	[10]	"invalid TSS",
	[11]	"segment not present",
	[12]	"stack exception",
	[13]	"general protection violation",
	[14]	"page fault",
	[15]	"15 (reserved)",
	[16]	"coprocessor error",
};


/*
 *  All traps
 */
void
trap(Ureg *ur)
{
	int v, user;
	int c;
	char buf[ERRLEN];
	Handler *h;
	static ulong intrtime;
	static int snoozing;
	static int iret_traps;
	ushort isr;

	v = ur->trap;

	user = ((ur->cs)&0xffff)!=KESEL && v!=Syscallvec;
	if(user)
		u->dbgreg = ur;
	else if(ur->pc >= KTZERO && ur->pc < (ulong)end && *(uchar*)ur->pc == 0xCF) {
		if(iret_traps++ > 10)
			panic("iret trap");
		return;
	}
	iret_traps = 0;

	/*
	 *  tell the 8259 that we're done with the
	 *  highest level interrupt (interrupts are still
	 *  off at this point)
	 */
	c = v&~0x7;
	isr = 0;
	if(c==Int0vec || c==Int1vec){
		isr = inb(Int0ctl);
		outb(Int0ctl, EOI);
		if(c == Int1vec){
			isr |= inb(Int1ctl)<<8;
			outb(Int1ctl, EOI);
		}
	}

	if(v>=256 || (h = halloc.ivec[v]) == 0){
		/* an old 386 generates these fairly often, no idea why */
		if(v == 13)
			return;

		/* a processor or coprocessor error */
		if(v <= 16){
			if(user){
				sprint(buf, "sys: trap: %s", excname[v]);
				postnote(u->p, 1, buf, NDebug);
				return;
			} else {
				dumpregs(ur);
				panic("%s pc=0x%lux", excname[v], ur->pc);
			}
		}

		if(v >= Int0vec && v < Int0vec+16){
			/* an unknown interrupt */
			v -= Int0vec;
			/*
			 * Check for a default IRQ7. This can happen when
			 * the IRQ input goes away before the acknowledge.
			 * In this case, a 'default IRQ7' is generated, but
			 * the corresponding bit in the ISR isn't set.
			 * In fact, just ignore all such interrupts.
			 */
			if((isr & (1<<v)) == 0)
				return;
			if(badintr[v]++ == 0 || (badintr[v]%100000) == 0){
				print("unknown interrupt %d pc=0x%lux: total %d\n", v,
					ur->pc, badintr[v]);
				print("isr = 0x%4.4ux\n", isr);
			}
		} else {
			/* unimplemented traps */
			print("illegal trap %d pc=0x%lux\n", v, ur->pc);
		}
		return;
	}

	/* there may be multiple handlers on one interrupt level */
	do {
		(*h->r)(ur, h->arg);
		h = h->next;
	} while(h);
	splhi();

	/* power management */
	if(v == Clockvec){
		/* allow power sheding on clock ticks */
		if(arch->snooze)
			snoozing = (*arch->snooze)(intrtime, 0);
	} else {
		/* turn power back on when anything else happens */
		if(snoozing && arch->snooze)
			snoozing = (*arch->snooze)(intrtime, 1);
		intrtime = m->ticks;
	}

	/* check user since syscall does its own notifying */
	if(user && (u->p->procctl || u->nnote))
		notify(ur);
}

/*
 *  dump registers
 */
void
dumpregs2(Ureg *ur)
{
	if(u)
		print("registers for %s %d\n", u->p->text, u->p->pid);
	else
		print("registers for kernel\n");
	print("FLAGS=%lux TRAP=%lux ECODE=%lux CS=%lux PC=%lux", ur->flags, ur->trap,
		ur->ecode, ur->cs&0xff, ur->pc);
	if(ur == (Ureg*)UREGADDR)
		print(" SS=%lux USP=%lux\n", ur->ss&0xff, ur->usp);
	else
		print("\n");
	print("  AX %8.8lux  BX %8.8lux  CX %8.8lux  DX %8.8lux\n",
		ur->ax, ur->bx, ur->cx, ur->dx);
	print("  SI %8.8lux  DI %8.8lux  BP %8.8lux\n",
		ur->si, ur->di, ur->bp);
	print("  DS %4.4ux  ES %4.4ux  FS %4.4ux  GS %4.4ux\n",
		ur->ds&0xffff, ur->es&0xffff, ur->fs&0xffff, ur->gs&0xffff);

}

void
dumpregs(Ureg *ur)
{
	dumpregs2(ur);
	print("  ur %lux\n", ur);
}

void
dumpstack(void)
{
	ulong l, v, i;
	extern ulong etext;

	if(u == 0)
		return;

	i = 0;
	for(l=(ulong)&l; l<USERADDR+BY2PG; l+=4){
		v = *(ulong*)l;
		if(KTZERO < v && v < (ulong)&etext){
			print("%lux ", v);
			i++;
		}
		if(i == 4){
			i = 0;
			print("\n");
		}
	}

}

long
execregs(ulong entry, ulong ssize, ulong nargs)
{
	ulong *sp;

	sp = (ulong*)(USTKTOP - ssize);
	*--sp = nargs;
	((Ureg*)UREGADDR)->usp = (ulong)sp;
	((Ureg*)UREGADDR)->pc = entry;
	return USTKTOP-BY2WD;			/* address of user-level clock */
}

ulong
userpc(void)
{
	return ((Ureg*)UREGADDR)->pc;
}

/*
 *  system calls
 */
#include "../port/systab.h"

/*
 *  syscall is called spllo()
 */
long
syscall(Ureg *ur, void *a)
{
	ulong	sp;
	long	ret;
	int	i;

	USED(a);

	u->p->insyscall = 1;
	u->p->pc = ur->pc;
	if((ur->cs)&0xffff == KESEL)
		panic("recursive system call");

	u->scallnr = ur->ax;
	if(u->scallnr == RFORK && u->p->fpstate == FPactive){
		/*
		 *  so that the child starts out with the
		 *  same registers as the parent
		 */
		splhi();
		if(u->p->fpstate == FPactive){
			fpsave(&u->fpsave);
			u->p->fpstate = FPinactive;
		}
		spllo();
	}
	sp = ur->usp;
	u->nerrlab = 0;
	ret = -1;
	if(!waserror()){
		if(u->scallnr >= sizeof systab/BY2WD){
			pprint("bad sys call number %d pc %lux\n", u->scallnr, ur->pc);
			postnote(u->p, 1, "sys: bad sys call", NDebug);
			error(Ebadarg);
		}

		if(sp<(USTKTOP-BY2PG) || sp>(USTKTOP-(1+MAXSYSARG)*BY2WD))
			validaddr(sp, (1+MAXSYSARG)*BY2WD, 0);

		u->s = *((Sargs*)(sp+1*BY2WD));
		u->p->psstate = sysctab[u->scallnr];

		ret = (*systab[u->scallnr])(u->s.args);
		poperror();
	}
	if(u->nerrlab){
		print("bad errstack [%d]: %d extra\n", u->scallnr, u->nerrlab);
		for(i = 0; i < NERR; i++)
			print("sp=%lux pc=%lux\n", u->errlab[i].sp, u->errlab[i].pc);
		panic("error stack");
	}

	u->p->insyscall = 0;
	u->p->psstate = 0;

	/*
	 *  Put return value in frame.  On the safari the syscall is
	 *  just another trap and the return value from syscall is
	 *  ignored.  On other machines the return value is put into
	 *  the results register by caller of syscall.
	 */
	ur->ax = ret;

	if(u->scallnr == NOTED)
		noted(ur, *(ulong*)(sp+BY2WD));

	splhi(); /* avoid interrupts during the iret */
	if(u->scallnr!=RFORK && (u->p->procctl || u->nnote))
		notify(ur);

	return ret;
}

/*
 *  Call user, if necessary, with note.
 *  Pass user the Ureg struct and the note on his stack.
 */
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
		sprint(n->msg+l, " pc=0x%.8lux", ur->pc);
	}

	if(n->flag!=NUser && (u->notified || u->notify==0)){
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
		
	if(!u->notify){
		qunlock(&u->p->debug);
		pexit(n->msg, n->flag!=NDebug);
	}
	sp = ur->usp;
	sp -= sizeof(Ureg);

	if(!okaddr((ulong)u->notify, 1, 0)
	|| !okaddr(sp-ERRLEN-4*BY2WD, sizeof(Ureg)+ERRLEN+4*BY2WD, 1)){
		qunlock(&u->p->debug);
		pprint("suicide: bad address in notify\n");
		pexit("Suicide", 0);
	}

	u->ureg = (void*)sp;
	memmove((Ureg*)sp, ur, sizeof(Ureg));
	*(Ureg**)(sp-BY2WD) = u->ureg;	/* word under Ureg is old u->ureg */
	u->ureg = (void*)sp;
	sp -= BY2WD+ERRLEN;
	memmove((char*)sp, u->note[0].msg, ERRLEN);
	sp -= 3*BY2WD;
	*(ulong*)(sp+2*BY2WD) = sp+3*BY2WD;	/* arg 2 is string */
	*(ulong*)(sp+1*BY2WD) = (ulong)u->ureg;	/* arg 1 is ureg* */
	*(ulong*)(sp+0*BY2WD) = 0;		/* arg 0 is pc */
	ur->usp = sp;
	ur->pc = (ulong)u->notify;
	u->notified = 1;
	u->nnote--;
	memmove(&u->lastnote, &u->note[0], sizeof(Note));
	memmove(&u->note[0], &u->note[1], u->nnote*sizeof(Note));

	qunlock(&u->p->debug);
	splx(s);
	return 1;
}

/*
 *   Return user to state before notify()
 */
void
noted(Ureg *ur, ulong arg0)
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

	nur = u->ureg;		/* pointer to user returned Ureg struct */

	/* sanity clause */
	oureg = (ulong)nur;
	if(!okaddr((ulong)oureg-BY2WD, BY2WD+sizeof(Ureg), 0)){
		qunlock(&u->p->debug);
		pprint("bad u->ureg in noted or call to noted() when not notified\n");
		pexit("Suicide", 0);
	}

	/* don't let user change text or stack segments */
	nur->cs = ur->cs;
	nur->ss = ur->ss;

	/* don't let user change system flags */
	nur->flags = (ur->flags & ~0xCD5) | (nur->flags & 0xCD5);

	memmove(ur, nur, sizeof(Ureg));

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
		ur->sp = sp;
		((ulong*)sp)[1] = oureg;	/* arg 1 0(FP) is ureg* */
		((ulong*)sp)[0] = 0;		/* arg 0 is pc */
		break;

	default:
		u->lastnote.flag = NDebug;
		qunlock(&u->p->debug);
		pprint("unknown noted arg 0x%lux\n", arg0);
		pprint("suicide: %s\n", u->lastnote.msg);
		pexit(u->lastnote.msg, 0);
		break;
		
	case NDFLT:
		if(u->lastnote.flag == NDebug){
			qunlock(&u->p->debug);
			pprint("suicide: %s\n", u->lastnote.msg);
		}else
			qunlock(&u->p->debug);
		pexit(u->lastnote.msg, u->lastnote.flag!=NDebug);
	}
}

/* This routine must save the values of registers the user is not permitted to write
 * from devproc and the restore the saved values before returning
 */
void
setregisters(Ureg *xp, char *pureg, char *uva, int n)
{
	ulong flags;
	ulong cs;
	ulong ss;

	flags = xp->flags;
	cs = xp->cs;
	ss = xp->ss;
	memmove(pureg, uva, n);
	xp->flags = (xp->flags & 0xff) | (flags & 0xff00);
	xp->cs = cs;
	xp->ss = ss;
}
