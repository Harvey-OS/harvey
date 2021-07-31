#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"init.h"
/*
 * software tlb simulation
 */
Softtlb stlb[1][STLBSIZE];

/*
 *  args passed by boot process
 */
int _argc; char **_argv; char **_env;

/*
 *  arguments passed to initcode and /boot
 */
char argbuf[128];

/*
 *  environment passed to boot -- sysname, consname, diskid
 */
char consname[NAMELEN];
char bootdisk[NAMELEN];

Conf	conf;

void
main(void)
{
	int havekbd;

	active.exiting = 0;
	active.machs = 1;
	machinit();
	arginit();
	confinit();
	xinit();
	printinit();
	tlbinit();

	/* pick a console */
	havekbd = kbdinit();
	screeninit(consname[0]);
	sccsetup(SCCADDR, SCCFREQ, 1);
	switch(consname[0]){
	case '0':
		sccspecial(0, &printq, &kbdq, 9600);
		break;
	case '1':
		sccspecial(1, &printq, &kbdq, 9600);
		break;
	default:
		if(havekbd==0)
			sccspecial(1, &printq, &kbdq, 9600);
		break;
	}

	pageinit();
	vecinit();
	procinit0();
	initseg();
	clockinit();
	chandevreset();
	streaminit();
	swapinit();
	userinit();
	schedinit();
}

void
machinit(void)
{
	int n;

	icflush(0, 32*1024);
	n = m->machno;
	memset(m, 0, sizeof(Mach));
	m->machno = n;
	m->stb = &stlb[n][0];
}

void
tlbinit(void)
{
	int i;

	for(i=0; i<NTLB; i++)
		puttlbx(i, KZERO | PTEPID(i), 0);
}

void
vecinit(void)
{
	ulong *p, *q;
	int size;

	p = (ulong*)EXCEPTION;
	q = (ulong*)vector80;
	for(size=0; size<4; size++)
		*p++ = *q++;
	p = (ulong*)UTLBMISS;
	q = (ulong*)vector0;
	for(size=0; size<0x80/sizeof(*q); size++)
		*p++ = *q++;
}

void
ioinit(void)
{
}

void
init0(void)
{
	m->proc = u->p;
	u->p->state = Running;
	u->p->mach = m;
	spllo();

	u->slash = (*devtab[0].attach)(0);
	u->dot = clone(u->slash, 0);

	kproc("alarm", alarmkproc, 0);
	chandevinit();

	if(!waserror()){
		ksetterm("mips %s 3000");
		ksetenv("cputype", "mips");
		ksetenv("consname", consname);
		ksetenv("sysname", sysname);
		if(bootdisk[0])
			ksetenv("bootdisk", bootdisk);
		poperror();
	}
	touser((uchar*)(USTKTOP - sizeof(argbuf)));
}

FPsave	initfp;

void
userinit(void)
{
	Proc *p;
	Segment *s;
	User *up;
	KMap *k;
	char **av;
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
	savefpregs(&initfp);
	p->fpstate = FPinit;

	/*
	 * Kernel Stack
	 */
	p->sched.pc = (ulong)init0;
	p->sched.sp = USERADDR+BY2PG-(1+MAXSYSARG)*BY2WD;
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
	s = newseg(SG_STACK, USTKTOP-USTKSIZE, USTKSIZE/BY2PG);
	p->seg[SSEG] = s;
	pg = newpage(1, 0, USTKTOP-BY2PG);
	segpage(s, pg);
	k = kmap(pg);
	for(av = (char**)argbuf; *av; av++)
		*av += (USTKTOP - sizeof(argbuf)) - (ulong)argbuf;
	memmove((uchar*)VA(k) + BY2PG - sizeof(argbuf), argbuf, sizeof argbuf);
	kunmap(k);

	/*
	 * Text
	 */
	s = newseg(SG_TEXT, UTZERO, 1);
	p->seg[TSEG] = s;
	segpage(s, newpage(1, 0, UTZERO));
	k = kmap(s->map[0]->pages[0]);
	memmove((uchar*)VA(k), initcode, sizeof initcode);
	kunmap(k);

	ready(p);
}

void
exit(int ispanic)
{
	int i;

	u = 0;
	lock(&active);
	active.machs &= ~(1<<m->machno);
	active.exiting = 1;
	unlock(&active);
	spllo();
	print("cpu %d exiting\n", m->machno);
	while(active.machs || consactive())
		for(i=0; i<1000; i++)
			;
	splhi();
	for(i=0; i<2000000; i++)
		;
	if(ispanic && !cpuserver)
		for(;;);
	firmware(cpuserver ? PROM_REBOOT : PROM_REINIT);
}

enum
{
	ZeroVal		= 0x12345678,
	ZeroInc		= 0x03141526,
};

ulong
meminit(void)
{
	long x, i;
	ulong *mirror, *zero, save0;
	ulong creg;

	/*
	 * put the memory system into a
	 * known state.
	 */
	creg = *CREG;
	*CREG = creg & ~Penable;
	wbflush();

	/*
	 * set up for 4MB SIMMS, then look for a
	 * mirror at 8MB.
	 * save anything we value at physical 0,
	 * it's probably part of the utlbmiss handler.
	 */
	setsimmtype(4);
	zero = (ulong*)KSEG1;
	save0 = *zero;
	mirror = (ulong*)(KSEG1+8*1024*1024);
	x = ZeroVal+ZeroInc;
	*zero = ZeroVal;
	*mirror = x;
	wbflush();
	if((*mirror != x) || (*mirror == *zero))
		setsimmtype(1);

	/*
	 * size memory, looking at every 8MB.
	 * this should catch the one possible 1MB bank
	 * after one or more 4MB banks.
	 */
	for(i = 8; i < 128; i += 8){
		mirror = (ulong*)(KSEG1+(i*1024L*1024L));
		*mirror = x;
		*zero = ZeroVal;
		wbflush();
		if(*mirror != x || *mirror == *zero)
			break;
		x += ZeroInc;
		zero = mirror;
	}

	/*
	 * restore things and enable parity checking.
	 * we have to scrub memory to reset any parity errors
	 * first, the easiest way is to copy memory to
	 * itself.
	 * there may be a better way.
	 */
	*(ulong*)KSEG1 = save0;
	memmove((ulong*)KSEG1, (ulong*)KSEG0, i*1024*1024);
	wbflush();
	*CREG = creg|Penable;
	wbflush();
	return i;
}

void
confinit(void)
{
	long mbytes;
	int mul;
	ulong ktop;

	mbytes = meminit();

	/*
	 * This split of memory into 2 banks fools the allocator into
	 * allocating low memory pages from bank 0 for the ethernet since
	 * it has only a 24bit address counter.
	 * NB. Magnums must have at LEAST 8Mbytes.
	 */
	conf.npage0 = (4*1024*1024)/BY2PG;
	conf.base0 = 0;

	conf.npage1 = (mbytes-4)*1024/4;
	conf.base1 = 4*1024*1024;

	conf.npage = conf.npage0+conf.npage1;
	conf.upages = (conf.npage*70)/100;

	ktop = PGROUND((ulong)end);
	ktop = PADDR(ktop);
	conf.npage0 -= ktop/BY2PG;
	conf.base0 += ktop;
	
	mul = (mbytes+11)/12;
	if(mul > 2)
		mul = 2;
	conf.nmach = 1;
	conf.nproc = 20 + 50*mul;
	conf.nswap = conf.nproc*80;
	conf.nimage = 50;
	conf.copymode = 0;			/* copy on write */
	conf.ipif = 4;
	conf.ip = mul*64;
	conf.arp = 32;
	conf.frag = 128;

	if(cpuserver)
		conf.nproc = 500;
	else
		conf.monitor = 1;	/* BUG */
}

/*
 *  copy arguments passed by the boot kernel (or ROM) into a temporary buffer.
 *  we do this because the arguments are in memory that may be allocated
 *  to processes or kernel buffers.
 *
 *  also grab any environment variables that might be useful
 */
struct
{
	char	*name;
	char	*val;
}bootenv[] = {
	{"netaddr=",	sysname},
	{"console=",	consname},
	{"bootdisk=",	bootdisk},
};
char *sp;

char *
pusharg(char *p)
{
	int n;

	n = strlen(p)+1;
	sp -= n;
	memmove(sp, p, n);
	return sp;
}

void
arginit(void)
{
	int i, n;
	char **av;

	/*
	 *  get boot env variables
	 */
	if(*sysname == 0)
		for(av = _env; *av; av++)
			for(i=0; i < sizeof bootenv/sizeof bootenv[0]; i++){
				n = strlen(bootenv[i].name);
				if(strncmp(*av, bootenv[i].name, n) == 0){
					strncpy(bootenv[i].val, (*av)+n, NAMELEN);
					bootenv[i].val[NAMELEN-1] = '\0';
					break;
				}
			}

	/*
	 *  pack args into buffer
	 */
	av = (char**)argbuf;
	sp = argbuf + sizeof(argbuf);
	for(i = 0; i < _argc; i++)
		av[i] = pusharg(_argv[i]);
	av[i] = 0;
}

/*
 *  set up the lance
 */
void
lancesetup(Lance *lp)
{
	int i;
	ulong x;
	Nvram *nv = NVRAM;

	lp->rap = LANCERAP;
	lp->rdp = LANCERDP;

	/*
	 *  get lance address
	 */
	for(i = 0; i < 6; i++)
		lp->ea[i] = nv[i].val;

	/*
	 *  number of buffers
	 */
	lp->lognrrb = 7;
	lp->logntrb = 7;
	lp->nrrb = 1<<lp->lognrrb;
	lp->ntrb = 1<<lp->logntrb;
	lp->sep = 1;
	lp->busctl = BSWP;

	/*
	 *  allocate area for lance init block and descriptor rings
	 */
	x = (ulong)xspanalloc(BY2PG, BY2PG, 0);
	if((x&KZERO) < 0xffffff)
		panic("lance above A24");

	lp->lanceram = (ushort *)(x|UNCACHED);
	lp->lm = (Lancemem*)x;
	/*
	 *  Allocate space in host memory for the io buffers.
	 */
	x = (lp->nrrb + lp->ntrb)*sizeof(Etherpkt);
	x = (ulong)xspanalloc(x, 64, 512*1024);
	if((x&KZERO) < 0xffffff)
		panic("lance above A24");

	lp->lrp = (Etherpkt*)x;
	lp->ltp = lp->lrp + lp->nrrb;
	lp->rp = (Etherpkt*)(x|UNCACHED);
	lp->tp = lp->lrp + lp->nrrb;
}
