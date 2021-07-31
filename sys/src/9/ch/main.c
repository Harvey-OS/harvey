#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"init.h"
#include	"pool.h"

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
char screenldepth[NAMELEN];

/*
 * software tlb simulation
 */
Softtlb stlb[MAXMACH][STLBSIZE];

/*
 *  mapping of physical cpu id to virtual id, virtual id to physical id
 */
ulong	*mpidp2v = (ulong*)CPUIDMAP;
ulong	mpidv2p[MAXMACH];

Address	epcswin;	/* base of EPC's small window */
Address S1win;		/* base of S1 small window */
Conf	conf;
FPsave	initfp;

void
main(void)
{
	arginit();
	confinit();
	savefpregs(&initfp);
	machinit();
	active.exiting = 0;
	active.machs = 1;
	kmapinit();
	xinit();
	printinit();
	vecinit();
	ioinit();
	iprint("\n\nPlan 9\n");
	tlbinit();
	pageinit();
	procinit0();
	initseg();
	links();
	chandevreset();
	swapinit();
	userinit();
	hinv();
	launchinit();
	schedinit();
}

void
hinv(void)
{
	int i, s, mem;
	CPUconf *cp;		/* configuration info on CPU board */
	Cpuconfig *cpuconf;	/* PROM generated cpu card configuration */
	Ioconfig *ip;		/* PROM generated io card configuration */
	uvlong x;

	for(s = 0; s < Maxslots; s++)
		switch(MCONF->board[s].type){
		case BTip19:
			cp = &CPUREGS->conf[s*Maxcpus];
			uvmove(&cp->rev, &x);
			print("	IP19 rev %ld in slot %d\n", (ulong)x, s);
			for(i = 0; i < Maxcpus; i++, cp++){
				cpuconf = &(MCONF->board[s].cpus[i]);
				if(cpuconf->vpid > MAXMACH)
					continue;
				if(cpuconf->enable == 0)
					continue;
				uvmove(&cp->cachesz, &x);
				print("\t\t%d R4400 %dMhz cachesz %dKB\n",
					cpuconf->vpid,
					cpuconf->speed*2,
					(1<<(22-(ulong)x))/1024);
			}
			break;
		case BTip21:
			print("	IP21 in slot %d\n", s);
			break;
		case BTio4:
			print("	IO4 in slot %d window %ud epcioa %lud\n", s,
				MCONF->board[s].winnum, MCONF->epcioa);
			ip = MCONF->board[s].io;
			for(i = 0; i < Maxios; i++){
				if(ip[i].type == IOAnone)
					continue;
				print("\t\tadaptor %d, ", i);
				switch(ip[i].type){
				case IOAscsi:
					print("s1 scsi\n");
					break;
				case IOAepc:
					print("epc\n");
					break;
				case IOAfchip:
					print("fchip");
					switch(ip[i].subtype){
					case IOAvmecc:
						print(" vmecc\n");
						break;
					case IOAfcg:
						print(" fcg\n");
						break;
					default:
						print("\n");
					}
					break;
				case IOAgiocc:
					print("dang\n");
					break;
				default:
					print("unknown type=%d\n", ip[i].type);
					break;
				}
			}
			break;
		case BTio5:
			print("	IO5 in slot %d\n", s);
			break;
		case BTmc3:
			mem = 0;
			for(i = 0; i < Maxmc3s; i++)
				if(MCONF->board[s].banks[i].enable)
					mem += MCONF->board[s].banks[i].size;
			print("	%dMB MC3 in slot %d\n", mem*8, s);
			break;
		}
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
	{"ldepth=",	screenldepth},
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
			for(i=0; i < nelem(bootenv); i++){
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
	for(i = 0; i < _argc; i++){
		if(strchr(_argv[i], '='))
			break;
		av[i] = pusharg(_argv[i]);
	}
	av[i] = 0;
}

/*
 *  initialize a processor's mach structure.  each processor does this
 *  for itself.
 */
void
machinit(void)
{
	int n;
	uvlong x;

	/* Ensure CU1 is off */
	clrfpintr();

	n = m->machno;
	m->stb = &stlb[n][0];
	m->nrdy = 0;
	clockinit();

	/* enable all interrupts except for IP19 on-board duart */
	x = 0x1b;
	uvmove(&x, &CPUREGS->ile);
}

/*
 *  setup MIPS trap vectors
 */
void
vecinit(void)
{
	memmove((ulong*)UTLBMISS, (ulong*)vector0, 0x100);
	memmove((ulong*)XEXCEPTION, (ulong*)vector0, 0x80);
	memmove((ulong*)CACHETRAP, (ulong*)vector100, 0x80);
	memmove((ulong*)EXCEPTION, (ulong*)vector180, 0x80);
	icflush((ulong*)UTLBMISS, 32*1024);
}

/*
 *  handler for strange ether interrupt I don't understand.
 *  we get a separate interrupt for the ether that seems to
 *  do it all.
 */
void
netintr(void)
{
}

/*
 *  Initialize the vmecc in window 'w' of IO adapter 'a'.
 */
void
vmeccinit(int w, int a)
{
	USED(w, a);
}

void
s1intr(void)
{
	uvlong sr;
	ulong srlo;
	
	uvld(S1win+S1srcmd, &sr);
	srlo = sr;
	if(srlo & S1srINT0)
		wd95aintr(0);
	if(srlo & S1srINT1)
		wd95aintr(1);
	if(srlo & S1srINT2)
		wd95aintr(2);
}

/*
 * SCSI DMA engine and interface
 * Will not recognize anything but WD95A and WD96A controllers
 */
void
s1init(int window, int adap)
{
	int i;
	ulong dmabase;
	S1chan *schan, *sc;

	/*
 	 *  set pointer to s1
	 */
	S1win = SWIN(window, adap, 0);

	schan = malloc(S1nchan*sizeof(S1chan));
	for(i = 0; i < S1nchan; i++) {

		if(wd95acheck(S1win+i*0x800))
			continue;

		sc = &schan[i];
		sc->swin = S1win;
		dmabase = S1win+S1dmaoffset+(i*0x800)+4;

		sc->chan     = i;

		sc->dmawrite = (ulong*)(dmabase+S1dmawrite);
		sc->dmaread  = (ulong*)(dmabase+S1dmaread);
		sc->dmaxlatlo= (ulong*)(dmabase+S1dmaxlatlo);
		sc->dmaxlathi= (ulong*)(dmabase+S1dmaxlathi);
		sc->dmaflush = (ulong*)(dmabase+S1dmaflush);
		sc->dmareset = (ulong*)(dmabase+S1dmareset);

		/* Must be accessed as 64bit locations */
		sc->srcmd    = S1win+S1srcmd;
		sc->ibuserr  = S1win+S1ibuserr;

		wd95aboot(i, sc, S1win+i*0x800);
	}

	/* Connect up a single S1 chip */
	setleveldest(ILs1+adap, 0, (uvlong*)(S1win+S1dmainterrupt));
	setleveldest(ILs1+adap, 0, (uvlong*)(S1win+S1dmainterrupt+0x08));

	sethandler(ILs1+adap, s1intr);
}

/*
 *  initialize io4 boards
 */
void
ioinit(void)
{
	int s, i;
	Ioconfig *ip;

	/* Get a quick fault if the address looks wrong */
	if(MCONF->magic != 0xdeadbabe)
		*(ulong*)0 = 0;

	sethandler(ILnet, netintr);

	/*
	 * Find the EPC and initialize it so the devices can print
	 * diagnostics
	 */
	for(s = 0; s < Maxslots; s++){
		if(MCONF->board[s].type != BTio4)
			continue;

		ip = MCONF->board[s].io;
		for(i = 0; i < Maxios; i++)
			if(ip[i].type)
				epcinit(MCONF->board[s].winnum, i);
	}

	/*
	 * Initialise the IO4 device hardware
	 */
	for(s = 0; s < Maxslots; s++){
		if(MCONF->board[s].type != BTio4)
			continue;

		ip = MCONF->board[s].io;
		for(i = 0; i < Maxios; i++){
			switch(ip[i].type){
			case IOAscsi:
				s1init(MCONF->board[s].winnum, i);
				break;
			case IOAfchip:
				switch(ip[i].subtype){
				case IOAvmecc:
					vmeccinit(MCONF->board[s].winnum, i);
					break;
				case IOAfcg:
					break;
				}
				break;
			}
		}
	}
}

static
char *gioslv[] =
{
	"idle",
	"pas wait",
	"wr wait",
	"ibus wait",
	"?",
	"ibus arb wait",
	"stalled",
	"busy",
};

/*
 *  enable/reenable an EPC interrupt
 */
void
epcenable(ulong intr)
{
	uvlong x;

	x = intr;
	uvmove(&x, &EPCINTR->imset);
}

/*
 *  handlers for EPC duart interrupts
 */
void
epcd0intr(void)
{
	z8530intr(4);
	epcenable(EIduart0);
}
void
epcd12intr(void)
{
	z8530intr(0);
	z8530intr(2);
	epcenable(EIduart12);
}

/*
 *  Initialize the EPC in window 'w' of IO adapter 'a'.
 *  We recognize only one EPC per system.
 */
void
epcinit(int w, int a)
{
	Duartregs *dp;
	EPCmisc *mp;
	uvlong x;

	/*
 	 *  we only know how to handle one EPC per system.
	 */
	if(epcswin)
		return;

	/*
 	 *  set pointer to epc
	 */
	epcswin = SWIN(w, a, 0);

	/*
	 *  Configure duart drivers, the order of these calls determines
	 *  the numbering of the port's (eia0, eia1, ...)
	 */
	dp = EPCDUART2;
	z8530setup(&dp->dataa, &dp->ctla, &dp->datab, &dp->ctlb, DUARTFREQ, 0);
	dp = EPCDUART1;
	z8530setup(&dp->dataa, &dp->ctla, &dp->datab, &dp->ctlb, DUARTFREQ, 0);
	dp = EPCDUART0;
	z8530setup(&dp->dataa, &dp->ctla, &dp->datab, &dp->ctlb, DUARTFREQ, 0);
	z8530special(0, 9600, &kbdq, &printq, kbdcr2nl);
	/*  Send interrupts to CPU 0 at an interupt level (0x6e, 0x6d, ...) */
	setleveldest(ILduart0, 0, &EPCINTR->duart0dest);
	sethandler(ILduart0, epcd0intr);
	setleveldest(ILduart12, 0, &EPCINTR->duart12dest);
	sethandler(ILduart12, epcd12intr);


	/* clear error state */
	mp = EPCMISC;
	x = 0x1000000;
	uvmove(&x, &mp->ibuserr);
	uvmove(&mp->clribuserr, &x);

	/* enable interrupts */
	epcenable(EIduart0 | EIduart12);

}

/*
 *  set interrupt level and destination
 */
void
setleveldest(int level, int cpudest, uvlong *addr)
{
	uvlong x;
	static int nextcpu;

	if(cpudest >= 0 && mpidv2p[cpudest] == 0)
		cpudest = -1;

	while(cpudest < 0){
		nextcpu++;
		if(nextcpu >= conf.nmach)
			nextcpu = 1;
		if(mpidv2p[cpudest] == 0)
			continue;
		cpudest = nextcpu;
	}


	x = (level<<8) | mpidv2p[cpudest];
	uvmove(&x, addr);
}

/*
 *  info a processor needs to launch
 */
typedef struct
{	
	ulong	magic;
	ulong	checksum;	/* 2-complement 32 bit checksum of 32 words */
	uchar	phys_id;	/* Physical ID of the processor */ 
	uchar	virt_id;	/* CPU virtual ID */	
	uchar	errno;		/* bootstrap error */
	uchar	proc_type;	/* Processor type (TFP or R4000) */
	ulong 	launch;		/* routine to start bootstrap */
	ulong	rendezvous; 	/* call this, when done launching*/
	ulong	bevutlb;	/* address of bev utlb exception handler */
	ulong	bevnormal;	/* address of bev normal exception handler */
	ulong	bevecc;		/* address of bev cache error handler  */
	ulong	scache_size;	/* 2nd level cache size for this CPU */
	ulong	nonbss;		/* God only knows what this is for */
	ulong	stack;		/* Stack pointer */
	ulong	lnch_parm;	/* User can pass single parm to launch */
	ulong	rndv_parm;	/* User can pass single parm to rendezvous */
	ulong	pr_id;		/* Processor implementation and revision */
	ulong 	filler[2];	/* Extra space */
} Launchpad;

void
launch(int i)
{
	Launchpad *l;

	if(mpidv2p[i] == 0)
		return;			/* not enabled */

	l = &((Launchpad*)LAUNCHADDR)[i];
	l->lnch_parm = 0;
	l->rendezvous = 0;
	l->rndv_parm = 0;
	l->stack = MACHADDR + BY2PG*(i+1);
	l->launch = (ulong)newstart;
}

void
launchinit(void)
{
	int i;

	m->fastclock = 0;
	wrcount(0);

	for(i=1; i<conf.nmach; i++){
		launch(i);
	}
	for(i=0; i<1000000; i++){
		if(active.machs == (1<<conf.nmach) - 1)
			break;
	}
	print("launch: active = %x\n", active.machs);
}

void
nop(void)
{
}

void
online(void)
{
	machinit();
	tlbinit();
	machwire();
	m->fastclock = MACHP(0)->fastclock;	/* synchronize clocks (more or less) */
	lock(&active);
	active.machs |= 1<<m->machno;
	unlock(&active);
	schedinit();
}

void
init0(void)
{
	char buf[2*NAMELEN];

	spllo();

	/*
	 * These are o.k. because rootinit is null.
	 * Then early kproc's will have a root and dot.
	 */
	up->slash = namec("#/", Atodir, 0, 0);
	cnameclose(up->slash->name);
	up->slash->name = newcname("/");
	up->dot = cclone(up->slash, 0);

	chandevinit();

	if(!waserror()){
		ksetenv("cputype", "mips");
		sprint(buf, "sgi %s R4400", conffile);
		ksetenv("terminal", buf);
		ksetenv("sysname", sysname);
		poperror();
	}

	kproc("alarm", alarmkproc, 0);
	touser((uchar*)(USTKTOP-sizeof(argbuf)));
}

FPsave	initfp;

void
userinit(void)
{
	Proc *p;
	KMap *k;
	Page *pg;
	char **av;
	Segment *s;

	p = newproc();
	p->pgrp = newpgrp();
	p->egrp = smalloc(sizeof(Egrp));
	p->egrp->ref = 1;
	p->fgrp = dupfgrp(nil);
	p->rgrp = newrgrp();
	p->procmode = 0640;

	strcpy(p->text, "*init*");
	strcpy(p->user, eve);

	p->fpstate = FPinit;
	p->fpsave.fpstatus = initfp.fpstatus;

	/*
	 * Kernel Stack
	 */
	p->sched.pc = (ulong)init0;
	p->sched.sp = (ulong)p->kstack+KSTACK-(1+MAXSYSARG)*BY2WD;
	/*
	 * User Stack, pass input arguments to boot process
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

	/* Text */
	s = newseg(SG_TEXT, UTZERO, 1);
	s->flushme++;
	p->seg[TSEG] = s;
	pg = newpage(1, 0, UTZERO);
	memset(pg->cachectl, PG_TXTFLUSH, sizeof(pg->cachectl));
	segpage(s, pg);
	k = kmap(s->map[0]->pages[0]);
	memmove((ulong*)VA(k), initcode, sizeof initcode);
	kunmap(k);

	ready(p);
}

void
exit(int type)
{
	uvlong x;
	int timer;

	lock(&active);
	active.machs &= ~(1<<m->machno);
	active.exiting = 1;
	unlock(&active);
	epcenable(EIduart0);
	epcenable(EIduart12);
	spllo();
	print("cpu %d exiting\n", m->machno);
	timer = 0;
	while(active.machs || consactive()) {
		if(timer++ > 400)
			break;
		delay(10);
	}
	splhi();
	x = ((uvlong)type << 32) | type;
	uvmove(&x, &CPUREGS->reset);
}

void
confinit(void)
{
	Mach *mm;
	int i, s, pcnt;
	ulong ktop, top;
	Cpuconfig *cpuconf;

	/*
	 *  divide memory twixt user pages and kernel. Since
	 *  the kernel can't use anything above .5G unmapped,
	 *  make sure all that memory goes to the user.
	 */
	ktop = PGROUND((ulong)end);
	ktop = PADDR(ktop);
	top = MCONF->memsize/16;

	conf.base0 = 0;
	conf.npage0 = top;
	conf.npage = conf.npage0;
	conf.npage0 -= ktop/BY2PG;
	conf.base0 += ktop;
	conf.npage1 = 0;
	conf.base1 = 0;

	pcnt = 70;
	if(top > (256*1024*1024)/BY2PG)
		pcnt = 90;
	mainmem->maxsize = BY2PG*conf.npage*(100-pcnt)/100;

	conf.upages = (conf.npage*pcnt)/100;
	if(top - conf.upages > (256*1024*1024)/BY2PG)
		conf.upages = top - (256*1024*1024)/BY2PG;
	conf.ialloc = ((conf.npage-conf.upages)/2)*BY2PG;

	/*
	 *  count CPU's, set up their mach structures
	 */
	conf.nmach = 0;
	for(s = 0; s < Maxslots; s++){
		switch(MCONF->board[s].type){
		case BTip19:
			for(i = 0; i < Maxcpus; i++){
				cpuconf = &(MCONF->board[s].cpus[i]);
				if(cpuconf->vpid > MAXMACH)
					continue;

 				if(conf.nmach >= MAXMACH)
					continue;

				mm = MACHP(cpuconf->vpid);
				memset(mm, 0, sizeof(Mach));

				if(cpuconf->enable == 0)
					continue;

				mpidp2v[s*Maxcpus + i] = cpuconf->vpid;
				mpidv2p[cpuconf->vpid] = s*Maxcpus + i;
				mm->machno = cpuconf->vpid;
				mm->speed = cpuconf->speed;
				conf.nmach++;
			}
			break;
		}
	}

	/* set up other configuration parameters */
	conf.nproc = 2000;
	conf.nswap = 262144;
	conf.nswppo = 4096;
	conf.nimage = 200;
	conf.ipif = 8;
	conf.ip = 64;
	conf.arp = 32;
	conf.frag = 32;

	conf.copymode = 1;		/* copy on reference */
}


/*
 *  for the sake of devcons
 */
void
lights(int)
{
}

void
buzz(int, int)
{
}
