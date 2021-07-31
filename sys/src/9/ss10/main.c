#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"init.h"

#include	<libg.h>
#include	<gnot.h>

typedef enum {EMPTY, DSIMM4MB, DSIMM16MB, DSIMM64MB} Simm;

#define DPRINT if(floppydebug)kprint
extern int floppydebug;

typedef 	struct Floppy		Floppy;	
struct Floppy {
	uchar Psra;
	uchar Psrb;
	uchar Pdor;
	uchar tdr;
	uchar Pmsr; 
	uchar Pfdata;
	uchar res;
	uchar Pdirdsr;
} *fdd;
uchar *auxio;

struct Intrreg		*intrreg;
struct Sysintrreg 	*sysintrreg;
struct Busctl		*busctl;
ulong			*sysctlreg;
struct Ecc			*eccreg;


/* uchar	idprom[32]; */
struct {
	uchar	format;
	uchar	type;
	uchar	ea[6];
	uchar	pad[32-8];
} idprom ;
ulong	rom;		/* open boot rom vector */
int	cpuserver;
ulong	bank[2];
char	mempres[256];
char	fbstr[32];
ulong	fbslot;
Label	catch;
uchar	*sp;

typedef struct Sysparam Sysparam;
struct Sysparam
{
	int	id;			/* Model type from id prom */
	char	*name;		/* System name */
	int	ncontext;		/* Number of MMU contexts to use*/
	int	ctxalign;		/* Alignment of context table */
	int	nbank;		/* Number of banks of memory */
	int	banksize;		/* Maximum Mbytes per bank */
	int	nfloppy;		
}
sysparam[] =
{
	{ 0x72, "SPARCStation 10", 	128, 0x10000, 8, 64, 1 },  
	{ 0x71, "Galaxy", 			128, 0x10000, 2, 256, 0 },  /* Not supported */
	{ 0 }
};
Sysparam *sparam;

extern struct
{
	Lock;
	Proc	*arena;
	Proc	*free;
}procalloc;

typedef struct
{
	Lock;
	Proc	*head;
	Proc	*tail;
}Schedq;

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
	putaction(0, getaction(0)|0x1000); /* Enable superscalar; after compile() */
	xinit();
	mmuinit();
	sysctlreg = (ulong *)kmappas(IOSPACE, SYSCTLREG, PTEIO|PTENOCACHE);
	kmapinit();
	printinit();	
	ioinit();	

	if(conf.monitor == 0) {
		sccspecial(2, &printq, &kbdq, 9600);
	} 
	if(conf.monitor)
		screeninit(fbstr, fbslot);

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

	k = kmappas(IOSPACE, INTRREG, PTEIO|PTENOCACHE);
	intrreg = (struct Intrreg*)VA(k);

	k = kmappas(IOSPACE, SYSINTRREG, PTEIO|PTENOCACHE);
	sysintrreg = (struct Sysintrreg *)VA(k);

	sysintrreg->sys_mset = ~0;
	sysintrreg->sys_mclear = ~0;
	sysintrreg->itr = 0;

	/* Enable Sbus arbitration */
	k = kmappas(IOSPACE, BUSCTL, PTEIO|PTENOCACHE);
	busctl = (struct Busctl *)VA(k);
	busctl->arben |= 0x001F0000; 
}

void
systemreset(void)
{
	delay(200);
	*sysctlreg = 1; /* software equivalent power-on reset */
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
	k = kmappas(IOSPACE, KMDUART, PTEIO|PTENOCACHE);
	sccsetup((void*)k, KMFREQ, 1);
	k = kmappas(IOSPACE, EIADUART, PTEIO|PTENOCACHE);
	sccsetup((void*)k, EIAFREQ, 1);

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

Conf	conf;

void
confinit(void)
{
	int mul;
	ulong slot, slotaddr, i, f, ktop, mbytes, bankmask; 
	ulong copy[8];
	char *name = (char *)copy, *s, *d;
	Simm simms[MAXSIMM];

	conf.nmach = 1;
	if(conf.nmach > MAXMACH)
		panic("confinit");

	/* 
	 * Fetch ID prom. Note that the NVRAM address uses the boot 
	 * ROM supplied virtual address of NVRAM, not the physical address. 
	 */
	for (i = 0, d = (char *)&idprom, s = (char *)BOOT_NVRAM + IDOFF; 
		i < sizeof(idprom); i++)
		d[i] = s[i];
	if(idprom.format!=1 || idprom.type!=0x72)
		*(ulong*)~0 = 0;	/* not a new generation sparc; die! */

	/* 
	 * Look up parameter information in sysparam table
	 */
	for(sparam = sysparam; sparam->id; sparam++)
		if(sparam->id == idprom.type)
			break;

	/* First entry in the table is the default */
	if(sparam->id == 0)
		sparam = sysparam;

	conf.ncontext = sparam->ncontext;
	conf.ctxalign = sparam->ctxalign;
	conf.nfloppy = sparam->nfloppy; 

	/* 
	 * Compile ASI access routines
	 * Needed for frame buffer search and memory bank scans
	 * Note that if caches are on, then snooping must also be on. 
	 */
	compile();

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
	conf.monitor = 0;
	/* map frame buffer id in SBUS slots 0..3 */
	for(f=0; f<=3 && fbstr[0]==0; f++){
		/* if frame buffer not present, we will trap, so prepare to catch it */
		if(setlabel(&catch) == 0){
			for (i = 0; i < 8; ++i) 
				copy[i] = getsbus(f*SBUSSLOT+i*sizeof(ulong));
			for(i=0; i<24 && fbstr[0]==0; i++) {
				switch(*(uchar*)(name+i)){
				case 'b':
					if(strncmp((char*)(name+i), "bwtwo", 5) == 0)
						strcpy(fbstr, "bwtwo");
					break;
				case 'c':
					if(strncmp((char*)(name+i), "cgthree", 7) == 0)
						strcpy(fbstr, "cgthree");
					if(strncmp((char*)(name+i), "cgsix", 5) == 0)
						strcpy(fbstr, "cgsix");
					break;
				}
			}
		}
		catch.pc = 0;
		if(fbstr[0]) {
			conf.monitor = 1;
			fbslot = f;
		}
	}

	/* 
	 * Chart memory 
	 * Warning: this code has only been tested with 2 16MB SIMMs installed 
	 */
	setpcr(getpcr()&~ALTCACHE); /* Disable pass-through ASI caching */

	/* From Sun-4M System Architecture, Spec Number 950-1373-01, p.71 */
	putio(ECC, 0x3FC); /* Disable ECC, enable refresh */
	putio(ECC+0xC, 0x000); /* No VSIMM support, for now */
	bankmask = 0;
	for (slot = 0; slot < sparam->nbank; ++slot) {
		slotaddr = slot * sparam->banksize * MB; 
		putphys(slotaddr | 0x0000000, 0x5555);
		putphys(slotaddr | 0x0800000, 0x6666);
		putphys(slotaddr | 0x2000000, 0x7777);
		bankmask |= (1 << (slot + 2));
		if (getphys(slotaddr | 0x0000000) == 0x5555) {
			simms[slot] = DSIMM64MB;
		} else if (getphys(slotaddr | 0x0800000) == 0x6666) {
			simms[slot] = DSIMM16MB;
		} else if (getphys(slotaddr | 0x2000000) == 0x7777) {
			simms[slot] = DSIMM4MB;
		} else {
			simms[slot] = EMPTY;
			bankmask &= ~(1 << (slot + 2));
		}
	}
	putio(ECC, bankmask); /* Enable refresh only for populated banks */

	/* Allocate contiguous banks */
	conf.base0 = conf.base1 = conf.npage0 = conf.npage1 = 0;
	for (slot = 0; slot < MAXSIMM && simms[slot] == DSIMM64MB; ++slot)
		conf.npage0 += 64 * MB / BY2PG;
	if (slot < MAXSIMM) {
		switch (simms[slot]) {
		case DSIMM16MB:	conf.npage0 += 16 * MB / BY2PG;	++slot;	break;
		case DSIMM4MB:	conf.npage0 += 4 * MB / BY2PG;	++slot;	break;
		case EMPTY:											break;
		case DSIMM64MB:
		default:			panic("confinit");						break;
		}

		while (slot < MAXSIMM && simms[slot] == EMPTY)
			++slot;
		
		if (slot < MAXSIMM)
			conf.base1 = slot * 64 * MB;
		for (; slot < MAXSIMM && simms[slot] == DSIMM64MB; ++slot)
			conf.npage1 += 64 * MB / BY2PG;
		if (slot < MAXSIMM) {
			switch (simms[slot]) {
			case DSIMM16MB:	conf.npage1 += 16 * MB / BY2PG;	++slot;	break;
			case DSIMM4MB:	conf.npage1 += 4 * MB / BY2PG;	++slot;	break;
			case EMPTY:											break;
			case DSIMM64MB:
			default:			panic("confinit");						break;
			}
		
			while (slot < MAXSIMM && simms[slot] == EMPTY)
				++slot;
		
			if (slot < MAXSIMM)
				conf.extras = 1;
		}
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

	ktop = PGROUND((ulong)end);
	ktop = PADDR(ktop);
	conf.npage0 -= ktop/BY2PG;
	conf.base0 += ktop;

	mbytes = (conf.npage*BY2PG)>>20;
	mul = 1 + (mbytes+11)/12;
	if(mul > 2)
		mul = 2;

	conf.nproc = 50*mul;
	conf.nswap = conf.npage*2;
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
	DMAdev *dma;
	ulong pa, va;
	int i;

	k = kmappas(SBUSSPACE, ETHER, PTEIO|PTENOCACHE);
	lp->rdp = (void*)(VA(k)+0);
	lp->rap = (void*)(VA(k)+2);
	for(i=0; i<6; i++)
		lp->ea[i] = idprom.ea[i];

	lp->lognrrb = 7;
	lp->logntrb = 7;
	lp->nrrb = 1<<lp->lognrrb;
	lp->ntrb = 1<<lp->logntrb;
	lp->sep = 1;
	lp->busctl = BSWP | ACON | BCON;

	/*
	 * Allocate area for lance init block and descriptor rings
	 */
	pa = PADDR(xspanalloc(BY2PG, BY2PG, 0));

	/* map at LANCESEGM */
	va = kmapdma(pa, BY2PG);
	lp->lanceram = (ushort*)va;
	lp->lm = (Lancemem*)pa; 

	/*
	 * Allocate space in host memory for the io buffers. From Forsyth
	 */
	i = (lp->nrrb+lp->ntrb)*sizeof(Etherpkt);
	i = (i+(BY2PG-1))/BY2PG;
	pa = PADDR(xspanalloc(i*BY2PG, BY2PG, 0));
	va = kmapdma(pa, i*BY2PG);

	lp->lrp = (Etherpkt*)pa; 
	lp->rp = (Etherpkt*)va; 
	lp->ltp = lp->lrp+lp->nrrb;
	lp->tp = lp->rp+lp->nrrb;

	/*
	 * Print out ethernet type. From Forsyth
	 */
	k = kmappas(SBUSSPACE, DMA, PTEIO|PTENOCACHE);
	dma = (DMAdev*)VA(k);
	dma->base = pa >> 24; 
	/* for now, let's assume the ROM has left the results of its auto-sensing */
	if(dma->ecsr & E_TP_select)
		print("Twisted pair ethernet\n");
	else
		print("AUI ethernet\n");

	/* Reset the Lance */
	delay(1);
	dma->ecsr |= E_Int_en|E_Invalidate|E_Dsbl_wr_inval|E_Dsbl_rd_drn; 
	delay(1);
}

void
floppysetup0(FController *fl)
{
	int x; 

	/*
	 * Map the floppy disk controller
	 */
	fdd = kmappas(IOSPACE, FLOPPY, PTEIO|PTENOCACHE); 
	auxio = kmappas(IOSPACE, AUXIO, PTEIO|PTENOCACHE); 
	*auxio &= ~0x01; /* Turn drive light off */

	/* 
	 * set for non-DMA mode
	 */
	fl->ncmd = 0;
	fl->cmd[fl->ncmd++] = Fdumpreg;
	x = splhi();
	if (floppycmd() < 0)
		panic("floppy reset");
	if (floppyresult() < 0)
		panic("floppy reset");
	splx(x);
	fl->ncmd = 0;
	fl->cmd[fl->ncmd++] = Fspec;
	fl->cmd[fl->ncmd++] = fl->stat[4];
	fl->cmd[fl->ncmd++] = fl->stat[5]|0x01;
	if (floppycmd() < 0)
		panic("floppy reset");

}

void
floppysetup1(FController *fl)
{
	USED(fl);
}

/*
 *  Execute a command on the floppy
 *
 *  when we've transferred n bytes, we're done
 * 
 */
int
floppyexec(char *va, long n, int isread)
{
	int i, j, s, x;
	int tries;

	DPRINT("floppyexec: %s %d bytes\n", isread ? "read" : "write", n);
	x = splhi(); 

	*auxio |= 0x03; /* Clear terminal count, turn drive light on */

	for(i = 0; i < n; i++){
		/* wait for ready bits */
		for(tries = 0; ; tries++){
			s = inb(Pmsr) & (Fready | Fnondma);
			if (s==(Fready|Fnondma))
				break;
			if (s==Fready) {
				DPRINT("floppyexec: stopped after %d bytes %d tries\n", i, tries);
				*auxio &= ~0x01; /* turn light back off */
				splx(x); 
				return i;
			}
		}

		if (isread) {
			va[i] = inb(Pfdata);
		} else
			outb(Pfdata, va[i]);
	}
	*auxio &= ~0x03; /* Signal terminal count, turn drive light off */

	/* Flush leftover FIFO bytes */
	for(i = 0, j = 0;; ++i) {
		s = inb(Pmsr) & (Fready | Fnondma);
		if (s==Fready) {
 	splx(x); 
	DPRINT("floppyexec: complete; flushed %d\n", j);
			return n;
		}
		if (s==(Fready|Fnondma)) {
			inb(Pfdata);
			++j;
		}
	}

	DPRINT("floppyexec: not possible\n");
	return n; /* Never reached */
}

/*
 *  eject disk; working routine from decompiling SS10 boot ROM
 */
void
floppyeject(FDrive *dp)
{
	USED(dp);

	outb(Pdor, (inb(Pdor)&~0x01)|0x10);
	delay(4);
	outb(Pdor, inb(Pdor)|0x80);
	delay(4);
	outb(Pdor, inb(Pdor)&~0x80);
	delay(4);
}

uchar 
inb(int addr)
{
	switch (addr) {
	case Psra: 	return fdd->Psra;		break;
	case Psrb: 	return fdd->Psrb;		break;
	case Pdor: 	return fdd->Pdor;		break;
	case Pmsr:	return fdd->Pmsr;		break;
	case Pfdata:	return fdd->Pfdata;		break;
	case Pdir: 		return fdd->Pdirdsr;		break;
	default: 		panic("inb");			break;
	}
	
	return 0; /* Never reached */
}

void
outb(int addr, uchar b)
{
	switch (addr) {
	case Pdor: 	fdd->Pdor = b;		break;
	case Pdsr:		fdd->Pdirdsr = b;	break;
	case Pfdata: 	fdd->Pfdata = b;	break;
	default: 		panic("outb");		break;
	}
}

void	
dmaend(int a)
{
	USED(a);
}

long	
dmasetup(int a, void* b, long len, int c)
{
	USED(a, b, c);
	return len > BY2PG ? BY2PG : len;
}
