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

/*
 *  8259 interrupt controllers
 */
enum
{
	Int0ctl=	0x20,		/* control port (ICW1, OCW2, OCW3) */
	Int0aux=	0x21,		/* everything else (ICW2, ICW3, ICW4, OCW1) */
	Int1ctl=	0xA0,		/* control port */
	Int1aux=	0xA1,		/* everything else (ICW2, ICW3, ICW4, OCW1) */

	EOI=		0x20,		/* non-specific end of interrupt */
};

int	int0mask = 0xff;	/* interrupts enabled for first 8259 */
int	int1mask = 0xff;	/* interrupts enabled for second 8259 */

/*
 *  trap/interrupt gates
 */
Segdesc ilt[256];
void	(*ivec[256])(void*);

void
sethvec(int v, void (*r)(void), int type, int pri)
{
	ilt[v].d0 = ((ulong)r)&0xFFFF|(KESEL<<16);
	ilt[v].d1 = ((ulong)r)&0xFFFF0000|SEGP|SEGPL(pri)|type;
}

void
setvec(int v, void (*r)(Ureg*))
{
	ivec[v] = r;

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
debugbpt(Ureg *ur)
{
	char buf[ERRLEN];

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
	setvec(Syscallvec, (void (*)(Ureg*))syscall);
	sethvec(Bptvec, intr3, SEGTG, 3);
	setvec(Bptvec, debugbpt);

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
	outb(Int0ctl, 0x11);		/* ICW1 - edge triggered, master,
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
	outb(Int1ctl, 0x11);		/* ICW1 - edge triggered, master,
					   ICW4 will be sent */
	outb(Int1aux, Int1vec);		/* ICW2 - interrupt vector offset */
	outb(Int1aux, 0x02);		/* ICW3 - I am a slave on level 2 */
	outb(Int1aux, 0x01);		/* ICW4 - 8086 mode, not buffered */

	/*
	 *  pass #2 8259 interrupts to #1
	 */
	int0mask &= ~0x04;
	outb(Int0aux, int0mask);
}

char *excname[] =
{
[0]	"divide error",
[1]	"debug exception",
[4]	"overflow",
[5]	"bounds check",
[6]	"invalid opcode",
[8]	"double fault",
[10]	"invalid TSS",
[11]	"segment not present",
[12]	"stack exception",
[13]	"general protection violation",
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

	v = ur->trap;

	user = ((ur->cs)&0xffff)!=KESEL && v!=Syscallvec;
	if(user)
		u->dbgreg = ur;

	/*
	 *  tell the 8259 that we're done with the
	 *  highest level interrupt (interrupts are still
	 *  off at this point)
	 */
	c = v&~0x7;
	if(c==Int0vec || c==Int1vec){
		if(c == Int1vec)
			outb(Int1ctl, EOI);
		outb(Int0ctl, EOI);
		if(v != Uart0vec)
			uartintr0(ur);
	}

	if(v>=256 || ivec[v] == 0){
		if(v == 13)
			return;
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
		print("bad trap type %d pc=0x%lux\n", v, ur->pc);
		return;
	}

	(*ivec[v])(ur);

	/*
	 *  check user since syscall does its own notifying
	 */
	if(user && (u->p->procctl || u->nnote))
		notify(ur);
}

/*
 *  dump registers
 */
void
dumpregs(Ureg *ur)
{
	if(u)
		print("registers for %s %d\n", u->p->text, u->p->pid);
	else
		print("registers for kernel\n");
	print("FLAGS=%lux ECODE=%lux CS=%lux PC=%lux", ur->flags,
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
syscall(Ureg *ur)
{
	ulong	sp;
	long	ret;
	int	i;

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
		sprint(n->msg+l, " pc=0x%.8lux", ur->pc);
	}
	if(n->flag!=NUser && (u->notified || u->notify==0)){
		if(n->flag == NDebug)
			pprint("suicide: %s\n", n->msg);
		qunlock(&u->p->debug);
		pexit(n->msg, n->flag!=NDebug);
	}
	sent = 0;
	if(!u->notified){
		if(!u->notify){
			qunlock(&u->p->debug);
			pexit(n->msg, n->flag!=NDebug);
		}
		sent = 1;
		u->svcs = ur->cs;
		u->svss = ur->ss;
		u->svflags = ur->flags;
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
		*(ulong*)(sp+1*BY2WD) = (ulong)u->ureg;	/* arg 1 is ureg* */
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
 *   Return user to state before notify()
 */
void
noted(Ureg *ur, ulong arg0)
{
	Ureg *nur;

	nur = u->ureg;		/* pointer to user returned Ureg struct */
	if(nur->cs!=u->svcs || nur->ss!=u->svss
	|| (nur->flags&0xff00)!=(u->svflags&0xff00)){
		pprint("bad noted ureg cs %ux ss %ux flags %ux\n", nur->cs, nur->ss,
			nur->flags);
    Die:
		pexit("Suicide", 0);
	}
	qlock(&u->p->debug);
	if(!u->notified){
		pprint("call to noted() when not notified\n");
		qunlock(&u->p->debug);
		return;
	}
	u->notified = 0;
	nur->flags = (u->svflags&0xffffff00) | (nur->flags&0xff);
	memmove(ur, nur, sizeof(Ureg));
	switch(arg0){
	case NCONT:
		if(!okaddr(nur->pc, 1, 0) || !okaddr(nur->usp, BY2WD, 0)){
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
