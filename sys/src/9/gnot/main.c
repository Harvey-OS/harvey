#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"init.h"

#include	<libg.h>
#include	<gnot.h>

typedef struct Boot Boot;
struct Boot
{
	long station;
	long traffic;
	char user[NAMELEN];
	char server[64];
	char line[64];
	char device;
};
#define BOOT ((Boot*)0)

char	bootuser[NAMELEN];
char	bootline[64];
char	bootserver[72];
int	bank[2];
uchar	*sp;

extern	GBitmap	gscreen;

void unloadboot(void);

void
main(void)
{
	u = 0;
	unloadboot();
	machinit();
	active.exiting = 0;
	active.machs = 1;
	mmuinit();
	confinit();
	xinit();
	kmapinit();
	duartinit();
	screeninit();
	printinit();
	print("bank 0: %dM  bank 1: %dM\n", bank[0], bank[1]);
	flushmmu();
	pageinit();
	procinit0();
	initseg();
	chandevreset();
	streaminit();
	swapinit();
	kmapinit();
	userinit();
	schedinit();
}

void
unloadboot(void)
{
	char s[64];

	memmove(bootuser, BOOT->user, sizeof(bootuser)-1);
	bootuser[sizeof(bootuser)-1] = 0;
	memmove(bootline, BOOT->line, sizeof(bootline)-1);
	bootline[sizeof(bootline)-1] = 0;
	memmove(s, BOOT->server, sizeof(s) - 1);
	s[sizeof(s)-1] = 0;
	switch(BOOT->device){
	case 'a':
		sprint(bootserver, "19200!%s", s);
		break;
	case 'A':
		sprint(bootserver, "9600!%s", s);
		break;
	case 'i':
		sprint(bootserver, "incon!%s", s);
		break;
	default:
		/* older boot ROM's don't zero out BOOT->user if it isn't set. */
		memset(bootuser, 0, sizeof(bootuser));
		sprint(bootserver, "scsi!%s", s);
		break;
	}
}

void
machinit(void)
{
	int n;

	n = m->machno;
	memset(m, 0, sizeof(Mach));
	m->machno = n;
	m->fpstate = FPinit;
	fprestore(&initfp);
}

void
mmuinit(void)
{
	ulong l, d, i;

	/*
	 * Invalidate user addresses
	 */
	for(l=0; l<4*1024*1024; l+=BY2PG)
		putmmu(l, INVALIDPTE, 0);
	/*
	 * Four meg of usable memory, with top 256K for screen
	 */
	for(i=1,l=KTZERO; i<MB4/BY2PG; l+=BY2PG,i++)
		putkmmu(l, PPN(l)|PTEVALID|PTEKERNEL);
	/*
	 * Screen after end
	 */
	l = PGROUND((ulong)end);
	gscreen.base = (ulong*)l;
	for(i=0,d=DISPLAYRAM; i<256*1024/BY2PG; d+=BY2PG,l+=BY2PG,i++)
		putkmmu(l, PPN(d)|PTEVALID|PTEKERNEL);
}

void
init0(void)
{
	u->nerrlab = 0;
	m->proc = u->p;
	u->p->state = Running;
	u->p->mach = m;
	spllo();
	
	u->slash = (*devtab[0].attach)(0);
	u->dot = clone(u->slash, 0);

	kproc("alarm", alarmkproc, 0);
	chandevinit();

	if(!waserror()){
		ksetterm("at&t %s");
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
	 */
	p->sched.pc = (ulong)init0;
	p->sched.sp = USERADDR+BY2PG-5*BY2WD;
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
	 * User Stack, copy in boot arguments
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
	char *p, *pp;
	uchar **lsp;

	sp = (uchar*)base + BY2PG - MAXSYSARG*BY2WD;

	ac = 0;
	for(p = bootline; p && *p; p = pp){
		pp = strchr(p, ' ');
		if(pp)
			*pp++ = 0;
		av[ac++] = pusharg(p);
	}
	if(bootuser[0]){
		av[ac++] = pusharg("-u");
		av[ac++] = pusharg(bootuser);
	}
	av[ac++] = pusharg(bootserver);

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
	int i;

	u = 0;
	wipekeys();
	spllo();
	while(consactive())
		for(i=0; i<1000; i++)
			;
	splhi();
	if(ispanic)
		for(;;);
	firmware();
}

banksize(int base)
{
	ulong va;

	if(end > (char *)((KZERO|1024L*1024L)-BY2PG))
		return 0;
	va = UZERO;	/* user page 1 is free to play with */
	putmmu(va, PTEVALID|(base+0)*MB/BY2PG, 0);
	*(ulong*)va = 0;	/* 0 at 0M */
	putmmu(va, PTEVALID|(base+1)*MB/BY2PG, 0);
	*(ulong*)va = 1;	/* 1 at 1M */
	putmmu(va, PTEVALID|(base+4)*MB/BY2PG, 0);
	*(ulong*)va = 4;	/* 4 at 4M */
	putmmu(va, PTEVALID|(base+0)*MB/BY2PG, 0);
	if(*(ulong*)va == 0)
		return 16;
	putmmu(va, PTEVALID|(base+1)*MB/BY2PG, 0);
	if(*(ulong*)va == 1)
		return 4;
	putmmu(va, PTEVALID|(base+0)*MB/BY2PG, 0);
	if(*(ulong*)va == 4)
		return 1;
	return 0;
}

Conf	conf;

void
confinit(void)
{
	int mul, pcnt;
	ulong ktop;

	conf.nmach = 1;
	if(conf.nmach > MAXMACH)
		panic("confinit");
	conf.monitor = 1;
	bank[0] = banksize(0);
	bank[1] = banksize(16);
	conf.npage0 = (bank[0]*MB)/BY2PG;
	conf.base0 = 0;
	conf.npage1 = (bank[1]*MB)/BY2PG;
	conf.base1 = 16*MB;
	conf.npage = conf.npage0+conf.npage1;

	pcnt = screenbits()-1;		/* Calculate % of memory for page pool */
	pcnt = 70 - (pcnt*10);
	conf.upages = (conf.npage*pcnt)/100;

	ktop = PGROUND((ulong)end) + 256*1024;	/* cf. mmuinit */
	ktop = PADDR(ktop);
	conf.npage0 -= ktop/BY2PG;
	conf.base0 += ktop;

	mul = 1 + (conf.npage1>0);
	conf.nproc = 50*mul;
	conf.nswap = 4096;
	conf.nimage = 50;
	conf.copymode = 0;		/* copy on write */
	conf.portispaged = 0;
	confinit1(mul);
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
	Balu *balu = (Balu *)u->balusave;

	USED(p);
	fpsave(&u->fpsave);
	if(u->fpsave.type){
		if(u->fpsave.size > sizeof u->fpsave.junk)
			panic("fpsize %d max %d\n", u->fpsave.size, sizeof u->fpsave.junk);
		fpregsave(u->fpsave.reg);
		u->p->fpstate = FPactive;
		m->fpstate = FPdirty;
	}
	if(BALU->cr0 != 0xFFFFFFFF)	/* balu busy */
		memmove(balu, BALU, sizeof(Balu));
	else{
		balu->cr0 = 0xFFFFFFFF;
		BALU->cr0 = 0xFFFFFFFF;
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
	Balu *balu = (Balu *)u->balusave;

	if(p->fpstate != m->fpstate){
		if(p->fpstate == FPinit){
			u->p->fpstate = FPinit;
			fprestore(&initfp);
			m->fpstate = FPinit;
		}else{
			fpregrestore(u->fpsave.reg);
			fprestore(&u->fpsave);
			m->fpstate = FPdirty;
		}
	}
	if(balu->cr0 != 0xFFFFFFFF)	/* balu busy */
		memmove(BALU, balu, sizeof BALU);
}

void
buzz(int f, int d)
{
	USED(f, d);
}

void
lights(int val)
{
	USED(val);
}
