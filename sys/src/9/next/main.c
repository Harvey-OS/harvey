#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"init.h"

#include	<libg.h>

int 	bank[4];
uchar	*sp;

#ifdef TIMINGS
Systimer *st;
Timings timings;
#endif

int banksize(ulong);

void
main(void)
{
	machinit();
	active.exiting = 0;
	active.machs = 1;
	confinit();
	xinit();
	screeninit();
	printinit();
	print("bank 0: %dM  bank 1: %dM\n", bank[0], bank[1]);
	mmuinit();
	flushmmu();
	pageinit();
	ioinit();
	procinit0();
	initseg();
	chandevreset();
	streaminit();
	trapinit();
	swapinit();
	userinit();
	schedinit();
}

void
machinit(void)
{
	int n;

	n = m->machno;
	memset(m, 0, sizeof(Mach));
	m->machno = n;
	m->mmask = 1<<m->machno;
	m->fpstate = FPinit;
	fprestore(&initfp);
	active.machs = 1;
}

void
ioinit(void)
{
	sccsetup(SCCADDR, SCCFREQ, 1);

	timerinit();
}

void
init0(void)
{
	u->nerrlab = 0;
	m->proc = u->p;
	u->p->state = Running;
	u->p->mach = m;

	spllo();

	/*
	 * These are o.k. because rootinit is null.
	 * Then early kproc's will have a root and dot.
	 */
	u->slash = (*devtab[0].attach)(0);
	u->dot = clone(u->slash, 0);

	kproc("alarm", alarmkproc, 0);
	chandevinit();

	if(!waserror()){
		ksetterm("next %s");
		ksetenv("cputype", "68020");
		poperror();
	}
	touser(sp);
}

FPsave	initfp;

void
userinit(void)
{
	Proc *p;
	Segment *s;
	User *up;
	KMap *k;
	Page *pg;

	p = newproc();
	p->pgrp = newpgrp();
	p->egrp = smalloc(sizeof(Egrp));
	p->egrp->ref = 1;
	p->fgrp = smalloc(sizeof(Fgrp));
	p->fgrp->ref = 1;
	p->procmode = 0640;

	strcpy(p->text, "*init*");
	strcpy(p->user, eve);
	p->fpstate = FPinit;

	/*
	 * Kernel Stack
	 *
	 * N.B. The -4 for the stack pointer is important.  Gotolabel
	 *	uses the bottom 4 bytes of stack to store it's return pc.
	 */
	p->sched.pc = (ulong)init0;
	p->sched.sp = USERADDR+BY2PG-4; 
	p->sched.sr = SUPER|SPL(0);
	p->upage = newpage(1, 0, USERADDR|(p->pid&0xFFFF));

	/*
	 * User
	 */
	k = kmap(p->upage);
	up = (User*)VA(k);
	up->p = p;
	kunmap(k);

	/*
	 * User Stack
	 */
	s = newseg(SG_STACK, USTKTOP-BY2PG, 1);
	p->seg[SSEG] = s;
	pg = newpage(1, 0, USTKTOP-BY2PG);
	segpage(s, pg);
	k = kmap(pg);
	bootargs(VA(k));
	kunmap(k);

	/*
	 * Text
	 */
	s = newseg(SG_TEXT, UTZERO, 1);
	p->seg[TSEG] = s;
	segpage(s, newpage(1, 0, UTZERO));
	k = kmap(s->map[0]->pages[0]);
	memmove((ulong*)VA(k), initcode, sizeof initcode);
	kunmap(k);

	ready(p);
}

uchar *
pusharg(char *p)
{
	int n;

	n = strlen(p)+1;
	sp -= n;
	memmove(sp, p, n);
	return sp;
}

void
bootargs(ulong base)
{
 	int i, ac;
	uchar *av[32];
	uchar **lsp;

	sp = (uchar*)base + BY2PG - MAXSYSARG*BY2WD;

	ac = 0;
	av[ac++] = pusharg("/68020/9nextstation");

	/* 4 byte word align stack */
	sp = (uchar*)((ulong)sp & ~3);

	/* build argc, argv on stack */
	sp -= (ac+1)*sizeof(sp);
	lsp = (uchar**)sp;
	for(i = 0; i < ac; i++)
		*lsp++ = av[i] + ((USTKTOP - BY2PG) - base);
	*lsp = 0;
	sp += (USTKTOP - BY2PG) - base - sizeof(sp);
}

void
exit(int ispanic)
{
	u = 0;
	wipekeys();
	splhi();
	print("exiting\n");
	if(ispanic)
		for(;;);
	delay(1000);
	poweroff();
}

int
banksize(ulong addr)
{
	int i;
	ulong min, max, t;
	
	for(i=0; i<16; i++)
		*(ulong*)(addr + i*MB) = addr + i*MB;
	min = ~0;
	max = 0;
	for(i=0; i<16; i++){
		t = *(ulong*)(addr + i*MB);
		if(min > t)
			min = t;
		if(max < t)
			max = t;
	}
	return (max-min)/MB+1;
}

Conf	conf;

void
confinit(void)
{
	ulong ktop;
	int mul, pcnt;

	conf.nmach = 1;
	if(conf.nmach > MAXMACH)
		panic("confinit");
	conf.monitor = 1;
	bank[0] = banksize(0x04000000);
	bank[1] = banksize(0x05000000);

	conf.npage0 = (bank[0]*MB)/BY2PG;
	conf.base0 = 0x04000000;
	conf.npage1 = (bank[1]*MB)/BY2PG;
	conf.base1 = 0x05000000;
	conf.npage = conf.npage0+conf.npage1;

	pcnt = screenbits()-1;		/* Calculate % of memory for page pool */
	pcnt = 70 - (pcnt*10);
	conf.upages = (conf.npage*pcnt)/100;

	ktop = PGROUND((ulong)end);
	ktop = PADDR(ktop);
	conf.npage0 -= ktop/BY2PG;
	conf.base0 += ktop;

	mul = (bank[0] + bank[1] + 7)/8;
	conf.nproc = 20 + 50*mul;
	conf.nswap = conf.nproc*80;
	conf.nimage = 50;
	conf.copymode = 0;		/* copy on write */
	conf.nfloppy = 1;
}

/*
 *  set up floating point for a new process
 */
void
procsetup(Proc *p)
{
	long fpnull;

	fpnull = 0;
	splhi();
	m->fpstate = FPinit;
	p->fpstate = FPinit;
	fprestore((FPsave*)&fpnull);
	spllo();
}

/*
 * Save the part of the process state.
 */
void
procsave(Proc *p)
{
	USED(p);
	fpsave(&u->fpsave);
	if(u->fpsave.type){
		fpregsave(u->fpsave.reg);
		p->fpstate = FPactive;
		m->fpstate = FPdirty;
	}
}

/*
 *  Restore what procsave() saves
 *
 *  Procsave() makes sure that what state points to is long enough
 */
void
procrestore(Proc *p)
{
	if(p->fpstate != m->fpstate){
		if(p->fpstate == FPinit){
			fprestore(&initfp);
			m->fpstate = FPinit;
		}
		else{
			fpregrestore(u->fpsave.reg);
			fprestore(&u->fpsave);
			m->fpstate = FPdirty;
		}
	}
}
