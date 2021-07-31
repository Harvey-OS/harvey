#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"init.h"

#include	<libg.h>
#include	<gnot.h>

uchar	*intrreg;
uchar	idprom[32];
ulong	rom;		/* open boot rom vector */
int	cpuserver;
ulong	romputcxsegm;
ulong	bank[2];
char	mempres[256];
char	fbstr[32];
ulong	fbslot;
Label	catch;
uchar	*sp;

typedef struct Sysparam Sysparam;
struct Sysparam
{
	int	id;		/* Model type from id prom */
	char	*name;		/* System name */
	char	ss2;		/* Is Sparcstation 2? */
	int	vacsize;	/* Cache size */
	int	vacline;	/* Cache line size */
	int	ncontext;	/* Number of MMU contexts */
	int	npmeg;		/* Number of page map entry groups */
	char	cachebug;	/* Machine needs cache bug work around */
	int	memscan;	/* Number of Meg to scan looking for memory */
}
sysparam[] =
{
	{ 0x51, "1 4/60",   0, 65536, 16,  8, 128, 0, 64 },
	{ 0x52, "IPC 4/40", 0, 65536, 16,  8, 128, 0, 64 },
	{ 0x53, "1+ 4/65",  0, 65536, 16,  8, 128, 0, 64 },
	{ 0x54, "SLC 4/20", 0, 65536, 16,  8, 128, 0, 64 },
	{ 0x55, "2 4/75",   1, 65536, 32, 16, 256, 1, 64 },
	{ 0x56, "ELC 4/25", 1, 65536, 32,  8, 128, 1, 64 },
	{ 0x57, "IPX 4/50", 1, 65536, 32,  8, 256, 1, 64 },
	{ 0 }
};
Sysparam *sparam;

void
main(void)
{
	u = 0;
	memset(edata, 0, (char*)end-(char*)edata);

	machinit();
	active.exiting = 0;
	active.machs = 1;
	trapinit();
	confinit();
	xinit();
	mmuinit();
	kmapinit();
	if(conf.monitor)
		screeninit(fbstr, fbslot);
	printinit();
	ioinit();
	if(conf.monitor == 0)
		sccspecial(2, &printq, &kbdq, 9600);
	pageinit();
	cacheinit();
	intrinit();
	procinit0();
	initseg();
	chandevreset();
	streaminit();
	swapinit();
	userinit();
	clockinit();
	schedinit();
}

void
intrinit(void)
{
	KMap *k;

	k = kmappa(INTRREG, PTEIO|PTENOCACHE);
	intrreg = (uchar*)k->va;
}

void
systemreset(void)
{
	delay(200);
	putenab(ENABRESET);
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
}

void
ioinit(void)
{
	KMap *k;

	/* tell scc driver its addresses */
	k = kmappa(KMDUART, PTEIO|PTENOCACHE);
	sccsetup((void*)(k->va), KMFREQ, 1);
	k = kmappa(EIADUART, PTEIO|PTENOCACHE);
	sccsetup((void*)(k->va), EIAFREQ, 1);

	/* scc port 0 is the keyboard */
	sccspecial(0, 0, &kbdq, 2400);
	kbdq.putc = kbdstate;

	/* scc port 1 is the mouse */
	sccspecial(1, 0, &mouseq, 2400);
}

void
init0(void)
{
	u->nerrlab = 0;
	m->proc = u->p;
	u->p->state = Running;
	u->p->mach = m;
	spllo();

	print("Sun Sparcstation %s\n", sparam->name);
	print("bank 0: %dM  1: %dM\n", bank[0], bank[1]);
	print("frame buffer id %lux slot %ld %s\n", conf.monitor, fbslot, fbstr);

	u->slash = (*devtab[0].attach)(0);
	u->dot = clone(u->slash, 0);

	kproc("alarm", alarmkproc, 0);
	chandevinit();

	if(!waserror()){
		ksetterm("sun %s");
		ksetenv("cputype", "sparc");
		poperror();
	}

	touser((long)sp);
}

FPsave	*initfpp;
uchar	initfpa[sizeof(FPsave)+7];

void
userinit(void)
{
	Proc *p;
	Segment *s;
	User *up;
	KMap *k;
	ulong l;
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
	/* must align initfpp to an ODD word boundary */
	l = (ulong)initfpa;
	l += 3;
	l &= ~7;
	l += 4;
	initfpp = (FPsave*)l;
	savefpregs(initfpp);
	p->fpstate = FPinit;

	/*
	 * Kernel Stack
	 */
	p->sched.pc = (((ulong)init0) - 8);	/* 8 because of RETURN in gotolabel */
	p->sched.sp = USERADDR+BY2PG-(1+MAXSYSARG)*BY2WD;
	p->sched.sp &= ~7;		/* SP must be 8-byte aligned */
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
	av[ac++] = pusharg("/sparc/9ss");

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
	print("cpu %d exiting\n", m->machno);
	while(consactive())
		for(i=0; i<1000; i++)
			;
	splhi();
	if(ispanic)
		for(;;);
	systemreset();
}

void
scanmem(char *mempres, int n)
{
	int i;
	ulong va, addr;

	va = 1*MB-2*BY2PG;
	for(i=0; i<n; i++){
		mempres[i] = 0;
		addr = i*MB;
		putw4(va, PPN(addr)|PTEPROBEMEM);
		*(ulong*)va = addr;
		if(*(ulong*)va == addr){
			addr = ~addr;
			*(ulong*)va = addr;
			if(*(ulong*)va == addr){
				mempres[i] = 1;
				*(ulong*)va = i + '0';
			}
		}
	}
	for(i=0; i<n; i++)
		if(mempres[i]){
			addr = i*MB;
			putw4(va, PPN(addr)|PTEPROBEMEM);
			mempres[i] = *(ulong*)va;
		}else
			mempres[i] = 0;
}

Conf	conf;

void
confinit(void)
{
	int mul;
	ulong i, j, f;
	ulong ktop, va, mbytes, npg, v;

	conf.nmach = 1;
	if(conf.nmach > MAXMACH)
		panic("confinit");

	/* map id prom */
	va = 1*MB-BY2PG;
	putw4(va, PPN(EEPROM)|PTEPROBEIO);
	memmove(idprom, (char*)(va+0x7d8), 32);
	if(idprom[0]!=1 || (idprom[1]&0xF0)!=0x50)
		*(ulong*)va = 0;	/* not a sparcstation; die! */
	putw4(va, INVALIDPTE);

	/*
	 * Look for a frame buffer.  Do it the way the
	 * ROM does it: scan the slots in a specified order and use
	 * the first one it finds.
	 *
	 * If we find a frame buffer, we always use it as a console
	 * rather than the attached terminal, if any.  This means
	 * if you have a frame buffer you'd better have a builtin
	 * keyboard, too.
	 */
	switch(idprom[1]){
	case 0x52:	/* IPC */
	case 0x54:	/* SLC */
		conf.monitor = 1;
		fbslot = 3;
		strcpy(fbstr, "bwtwo");
		break;
	case 0x57:	/* IPX */
		conf.monitor = 1;
		fbslot = 3;
		strcpy(fbstr, "cgsix");
		break;
	default:
		/* map frame buffer id in SBUS slots 0..3 */
		for(f=0; f<=3 && fbstr[0]==0; f++){
			putw4(va, ((FRAMEBUFID(f)>>PGSHIFT)&0xFFFF)|PTEPROBEIO);
			conf.monitor = 0;
			/* if frame buffer not present, we will trap, so prepare to catch it */
			if(setlabel(&catch) == 0){
				conf.monitor = *(ulong*)va;
				switch(conf.monitor & 0xF0FFFFFF){
				case 0xF0010101:
					strcpy(fbstr, "cgthree");
					break;
				case 0xF0010104:
					strcpy(fbstr, "bwtwo");
					break;
				}
				if(fbstr[0] == 0){
					j = *(ulong*)(va+4);
					if(j > BY2PG-8)
						j = BY2PG - 8;	/* -8 for safety */
					for(i=0; i<j && fbstr[0]==0; i++)
						switch(*(uchar*)(va+i)){
						case 'b':
							if(strncmp((char*)(va+i), "bwtwo", 5) == 0)
								strcpy(fbstr, "bwtwo");
							break;
						case 'c':
							if(strncmp((char*)(va+i), "cgthree", 7) == 0)
								strcpy(fbstr, "cgthree");
							if(strncmp((char*)(va+i), "cgsix", 5) == 0)
								strcpy(fbstr, "cgsix");
							break;
						}
				}
			}
			catch.pc = 0;
			putw4(va, INVALIDPTE);
			if(fbstr[0])
				fbslot = f;
		}
		break;
	}

	for(sparam = sysparam; sparam->id; sparam++)
		if(sparam->id == idprom[1])
			break;

	/* First entry in the table is the default */
	if(sparam->id == 0)
		sparam = sysparam;

	conf.ss2 = sparam->ss2;
	conf.vacsize = sparam->vacsize;
	conf.vaclinesize = sparam->vacline;
	conf.ncontext = sparam->ncontext;
	conf.npmeg = sparam->npmeg;
	conf.ss2cachebug = sparam->cachebug;

	/* Chart memory */
	scanmem(mempres, sparam->memscan);

	/* Find mirrors and allocate banks */
	for(i=0; i<sparam->memscan; i++)
		if(mempres[i]){
			v = mempres[i];
			i = v-'0';
			for(j=i+1; j<sparam->memscan && mempres[j]>v; j++)
				v = mempres[j];
			npg = (j-i)*MB/BY2PG;
			if(conf.npage0 == 0){
				conf.base0 = i*MB;
				conf.npage0 = npg;
			}else if(conf.npage1 < npg){
				conf.base1 = i*MB;
				conf.npage1 = npg;
			}
			i = v-'0';
		}

	bank[0] = conf.npage0*BY2PG/MB;
	bank[1] = conf.npage1*BY2PG/MB;

	if(bank[1] == 0){
		/*
		 * This split of memory into 2 banks fools the allocator into
		 * allocating low memory pages from bank 0 for the ethernet since
		 * it has only a 24bit address counter.
		 * NB. Suns must have at LEAST 8Mbytes.
		 */
		conf.npage1 = conf.npage0 - (8*MB)/BY2PG;
		conf.base1 = conf.base0 + 8*MB;
		conf.npage0 = (8*MB)/BY2PG;
		bank[1] = bank[0]-8;
		bank[0] = 8;
	}
	conf.npage = conf.npage0+conf.npage1;
	i = screenbits()-1;		/* Calculate % of memory for page pool */
	i = 70 - (i*10);
	conf.upages = (conf.npage*i)/100;
	if(cpuserver){
		i = conf.npage-conf.upages;
		if(i > (12*MB)/BY2PG)
			conf.upages +=  i - ((12*MB)/BY2PG);
	}

	romputcxsegm = *(ulong*)(rom+260);

	ktop = PGROUND((ulong)end);
	ktop = PADDR(ktop);
	conf.npage0 -= ktop/BY2PG;
	conf.base0 += ktop;

	mbytes = (conf.npage*BY2PG)>>20;
	mul = 1 + (mbytes+11)/12;
	if(mul > 2)
		mul = 2;

	conf.nproc = 50*mul;
	if(cpuserver)
		conf.nswap = conf.npage*2;
	else
		conf.nswap = 16*MB/BY2PG;
	conf.nimage = 50;
	conf.copymode = 0;		/* copy on write */
	if(cpuserver)
		conf.nproc = 500;
}

/*
 *  set up the lance
 */
void
lancesetup(Lance *lp)
{
	KMap *k;
	uchar *cp;
	ulong pa, va;
	int i;

	k = kmappa(ETHER, PTEIO|PTENOCACHE);
	lp->rdp = (void*)(k->va+0);
	lp->rap = (void*)(k->va+2);
	k = kmappa(EEPROM, PTEIO|PTENOCACHE);
	cp = (uchar*)(k->va+0x7da);
	for(i=0; i<6; i++)
		lp->ea[i] = *cp++;
	kunmap(k);

	lp->lognrrb = 7;
	lp->logntrb = 5;
	lp->nrrb = 1<<lp->lognrrb;
	lp->ntrb = 1<<lp->logntrb;
	lp->sep = 1;
	lp->busctl = BSWP | ACON | BCON;

	/*
	 * Allocate area for lance init block and descriptor rings
	 */
	pa = (ulong)xspanalloc(BY2PG, BY2PG, 0);

	/* map at LANCESEGM */
	k = kmappa(pa, PTEMAINMEM|PTENOCACHE);
	lp->lanceram = (ushort*)k->va;
	lp->lm = (Lancemem*)k->va;

	/*
	 * Allocate space in host memory for the io buffers.
	 * Allocate a block and kmap it page by page.  kmap's are initially
	 * in reverse order so rearrange them.
	 */
	i = (lp->nrrb+lp->ntrb)*sizeof(Etherpkt);
	i = (i+(BY2PG-1))/BY2PG;
	pa = (ulong)xspanalloc(i*BY2PG, BY2PG, 0)&~KZERO;
	va = kmapregion(pa, i*BY2PG, PTEMAINMEM|PTENOCACHE);

	lp->lrp = (Etherpkt*)va;
	lp->rp = (Etherpkt*)va;
	lp->ltp = lp->lrp+lp->nrrb;
	lp->tp = lp->rp+lp->nrrb;
}
