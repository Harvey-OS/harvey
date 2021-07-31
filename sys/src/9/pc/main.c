#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"init.h"
#include	<ctype.h>


uchar *sp;	/* stack pointer for /boot */

extern PCArch nsx20, generic, ncr3170;

PCArch *arch;
PCArch *knownarch[] =
{
	&nsx20,
	&ncr3170,
	&generic,
};

void
main(void)
{
	ident();
	i8042a20();		/* enable address lines 20 and up */
	machinit();
	active.exiting = 0;
	active.machs = 1;
	confinit();
	xinit();
	screeninit();
	printinit();
	mmuinit();
	pageinit();
	trapinit();
	mathinit();
	clockinit();
	faultinit();
	kbdinit();
	procinit0();
	initseg();
	chandevreset();
	streaminit();
	swapinit();
	userinit();
	schedinit();
}

/*
 *  This tries to capture architecture dependencies since things
 *  like power management/reseting/mouse are outside the hardware
 *  model.
 */
void
ident(void)
{
	char *id = (char*)(ROMBIOS + 0xFF40);
	PCArch **p;

	for(p = knownarch; *p != &generic; p++)
		if(strncmp((*p)->id, id, strlen((*p)->id)) == 0)
			break;
	arch = *p;
}

void
machinit(void)
{
	int n;

	n = m->machno;
	memset(m, 0, sizeof(Mach));
	m->machno = n;
	m->mmask = 1<<m->machno;
}

ulong garbage;

void
init0(void)
{
	char tstr[32];

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
		strcpy(tstr, arch->id);
		strcat(tstr, " %s");
		ksetterm(tstr);
		ksetenv("cputype", "386");
		poperror();
	}
	touser(sp);
}

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
	fpoff();

	/*
	 * Kernel Stack
	 *
	 * N.B. The -12 for the stack pointer is important.
	 *	4 bytes for gotolabel's return PC
	 */
	p->sched.pc = (ulong)init0;
	p->sched.sp = USERADDR + BY2PG - 4;
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
	char *cp = BOOTLINE;
	char buf[64];

	sp = (uchar*)base + BY2PG - MAXSYSARG*BY2WD;

	ac = 0;
	av[ac++] = pusharg("/386/9safari");
	av[ac++] = pusharg("-p");
	cp[64] = 0;
	if(strncmp(cp, "fd!", 3) == 0){
		sprint(buf, "local!#f/fd%ddisk", atoi(cp+3));
		av[ac++] = pusharg(buf);
	} else if(strncmp(cp, "hd!", 3) == 0){
		sprint(buf, "local!#w/hd%ddisk", atoi(cp+3));
		av[ac++] = pusharg(buf);
	}

	/* 4 byte word align stack */
	sp = (uchar*)((ulong)sp & ~3);

	/* build argc, argv on stack */
	sp -= (ac+1)*sizeof(sp);
	lsp = (uchar**)sp;
	for(i = 0; i < ac; i++)
		*lsp++ = av[i] + ((USTKTOP - BY2PG) - base);
	*lsp = 0;
	sp += (USTKTOP - BY2PG) - base - sizeof(ulong);
}

Conf	conf;

void
confinit(void)
{
	long x, i, j, *l;
	int pcnt;
	ulong ktop;

	/*
	 *  the first 640k is the standard useful memory
	 *  the next 128K is the display, I/O mem, and BIOS
	 *  the last 256k belongs to the roms and other devices
	 */
	conf.npage0 = 640/4;
	conf.base0 = 0;

	/*
	 *  size the non-standard memory
	 */
	x = 0x12345678;
	for(i=2; i<17; i++){
		/*
		 *  write the word
		 */
		l = (long*)(KZERO|(i*MB));
		l--;
		*l = x;
		/*
		 *  take care of wraps
		 */
		for(j = 1; j < i; j++){
			l = (long*)(KZERO|(j*MB));
			l--;
			*l = ~x;
		}
		/*
		 *  check
		 */
		l = (long*)(KZERO|(i*MB));
		l--;
		if(*l != x)
			break;
		x += 0x3141526;
	}
	i--;
	conf.base1 = 1*MB;
	conf.npage1 = (i*MB - conf.base1)/BY2PG;
	conf.npage = conf.npage0 + conf.npage1;

	conf.ldepth = 0;
	pcnt = (1<<conf.ldepth)-1;		/* Calculate % of memory for page pool */
	pcnt = 70 - (pcnt*10);
	conf.upages = (conf.npage*pcnt)/100;
	if(conf.npage - conf.upages < (2*MB)/BY2PG)
		conf.upages = conf.npage - (2*MB)/BY2PG;

	ktop = PGROUND((ulong)end);
	ktop = PADDR(ktop);
	conf.npage0 -= ktop/BY2PG;
	conf.base0 += ktop;

	conf.topofmem = i*MB;

	conf.monitor = 1;
	conf.nproc = 30 + i*5;
	conf.nswap = conf.nproc*80;
	conf.nimage = 50;
	conf.copymode = 0;			/* copy on write */
	conf.arp = 32;
	conf.nfloppy = 2;
	conf.nhard = 2;
}

char *mathmsg[] =
{
	"invalid",
	"denormalized",
	"div-by-zero",
	"overflow",
	"underflow",
	"precision",
	"stack",
	"error",
};

/*
 *  math coprocessor error
 */
void
matherror(Ureg *ur)
{
	ulong status;
	int i;
	char *msg;
	char note[ERRLEN];

	/*
	 *  a write cycle to port 0xF0 clears the interrupt latch attached
	 *  to the error# line from the 387
	 */
	outb(0xF0, 0xFF);

	/*
	 *  save floating point state to check out error
	 */
	fpenv(&u->fpsave);
	status = u->fpsave.status;

	msg = 0;
	for(i = 0; i < 8; i++)
		if((1<<i) & status){
			msg = mathmsg[i];
			sprint(note, "sys: fp: %s fppc=0x%lux", msg, u->fpsave.pc);
			postnote(u->p, 1, note, NDebug);
			break;
		}
	if(msg == 0){
		sprint(note, "sys: fp: unknown fppc=0x%lux", u->fpsave.pc);
		postnote(u->p, 1, note, NDebug);
	}
	if(ur->pc & KZERO)
		panic("fp: status %lux fppc=0x%lux pc=0x%lux", status,
			u->fpsave.pc, ur->pc);
}

/*
 *  math coprocessor emulation fault
 */
void
mathemu(Ureg *ur)
{
	USED(ur);
	switch(u->p->fpstate){
	case FPinit:
		fpinit();
		u->p->fpstate = FPactive;
		break;
	case FPinactive:
		fprestore(&u->fpsave);
		u->p->fpstate = FPactive;
		break;
	case FPactive:
		panic("math emu", 0);
		break;
	}
}

/*
 *  math coprocessor segment overrun
 */
void
mathover(Ureg *ur)
{
	USED(ur);
print("sys: fp: math overrun pc 0x%lux pid %d\n", ur->pc, u->p->pid);
	pexit("math overrun", 0);
}

void
mathinit(void)
{
	setvec(Matherr1vec, matherror);
	setvec(Matherr2vec, matherror);
	setvec(Mathemuvec, mathemu);
	setvec(Mathovervec, mathover);
}

/*
 *  set up floating point for a new process
 */
void
procsetup(Proc *p)
{
	p->fpstate = FPinit;
	fpoff();
}

/*
 *  Save the mach dependent part of the process state.
 */
void
procsave(Proc *p)
{
	if(p->fpstate == FPactive){
		if(p->state == Moribund)
			fpoff();
		else
			fpsave(&u->fpsave);
		p->fpstate = FPinactive;
	}
}

/*
 *  Restore what procsave() saves
 */
void
procrestore(Proc *p)
{
	USED(p);
}


/*
 *  the following functions all are slightly different from
 *  PC to PC.
 */

/*
 *  reset the i387 chip
 */
void
exit(int ispanic)
{
	u = 0;
	print("exiting\n");
	if(ispanic)
		for(;;);

	(*arch->reset)();
}

/*
 *  set cpu speed
 *	0 == low speed
 *	1 == high speed
 */
int
cpuspeed(int speed)
{
	if(arch->cpuspeed)
		return (*arch->cpuspeed)(speed);
	else
		return 0;
}

/*
 *  f == frequency (Hz)
 *  d == duration (ms)
 */
void
buzz(int f, int d)
{
	if(arch->buzz)
		(*arch->buzz)(f, d);
}

/*
 *  each bit in val stands for a light
 */
void
lights(int val)
{
	if(arch->lights)
		(*arch->lights)(val);
}

/*
 *  power to serial port
 *	onoff == 1 means on
 *	onoff == 0 means off
 */
int
serial(int onoff)
{
	if(arch->serialpower)
		return (*arch->serialpower)(onoff);
	else
		return 0;
}

/*
 *  power to modem
 *	onoff == 1 means on
 *	onoff == 0 means off
 */
int
modem(int onoff)
{
	if(arch->modempower)
		return (*arch->modempower)(onoff);
	else
		return 0;
}
