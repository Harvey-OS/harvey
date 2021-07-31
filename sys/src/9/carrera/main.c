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

Conf	conf;
FPsave	initfp;
int	configreg;

int	int0mask = 0xff;	/* interrupts enabled for first 8259 */
int	int1mask = 0xff;	/* interrupts enabled for second 8259 */

extern	uchar rdbgcode[];
extern	ulong	rdbglen;
extern	int	noprint;

void
main(void)
{
	noprint = 1;
	tlbinit();
	ioinit(1);		/* Very early to establish IO mappings */
	rdbginit();
	arginit();
	confinit();
	savefpregs(&initfp);
	machinit();
	kmapinit();
	xinit();
	iomapinit();
	printinit();
	serialinit();
	iprint("Plan 9\n");		/**/
	vecinit();
	screeninit();
	pageinit();
	procinit0();
	initseg();
	links();
	chandevreset();
	swapinit();
	userinit();
	noprint = 0;
	schedinit();
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
}bootenv[] =
{
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

	/* Ensure CU1 is off */
	clrfpintr();

	/* scrub cache */
	configreg = ((int(*)(void))((ulong)cleancache|0xA0000000))();

	memset(m, 0, sizeof(Mach));

	n = m->machno;
	m->stb = &stlb[n][0];

	m->speed = 50;
	clockinit();

	active.exiting = 0;
	active.machs = 1;
}

/*
 * Set up a console on serial port 2
 */
void
serialinit(void)
{
	ns16552install();
	kbdinit();
}

/*
 * Map IO address space in wired down TLB entry 1
 */
void
ioinit(int mapeisa)
{
	ulong devphys, isaphys, intphys, isamphys, promphys, ptec, ptes, v;

	/*
	 * If you want to segattach the eisa space these
	 * mappings must be turned off to prevent duplication
	 * of the tlb entries
	 */
	if(mapeisa) {
		isaphys = IOPTE|PPN(Eisaphys)|PTEGLOBL;
		isamphys = 0x04000000|IOPTE|PTEGLOBL;
	}
	else {
		isaphys = PTEGLOBL;
		isamphys = PTEGLOBL;
	}

	/*
	 * Map devices and the Eisa control space
	 */
	devphys = IOPTE|PPN(Devicephys);
	puttlbx(1, Devicevirt, devphys, isaphys, PGSZ64K);

	/*
	 * Map Interrupt control & Eisa memory
	 */
	intphys  = IOPTE|PPN(Intctlphys);
	puttlbx(2, Intctlvirt, intphys, isamphys, PGSZ1M);

	/* Enable all device interrupts */
	IO(ushort, Intenareg) = 0xffff;

	/* Map the rom back into Promvirt to allow NMI handling */
	promphys = IOPTE|PPN(Promphys);
	puttlbx(3, Promvirt, promphys, PTEGLOBL, PGSZ1M);

	/* 8 MB video ram config */
	v = IO(ulong, 0xE0000004);
	v &= ~(3<<8);
	v |= (2<<8);
	IO(ulong, 0xE0000004) = v;

	/* Map the display hardware */
	ptec = PPN(VideoCTL)|PTEGLOBL|PTEVALID|PTEWRITE|PTEUNCACHED;
	ptes = PPN(VideoMEM)|PTEGLOBL|PTEVALID|PTEWRITE|PTEUNCACHED;
	puttlbx(4, Screenvirt, ptes, ptec, PGSZ4M);

	/* for PC weenies */
	/*
	 *  Set up the first 8259 interrupt processor.
	 *  Make 8259 interrupts start at CPU vector Int0vec.
	 *  Set the 8259 as master with edge triggered
	 *  input with fully nested interrupts.
	 */
	EISAOUTB(Int0ctl, 0x11);	/* ICW1 - edge triggered, master,
					   ICW4 will be sent */
	EISAOUTB(Int0aux, Int0vec);	/* ICW2 - interrupt vector offset */
	EISAOUTB(Int0aux, 0x04);	/* ICW3 - have slave on level 2 */
	EISAOUTB(Int0aux, 0x01);	/* ICW4 - 8086 mode, not buffered */

	/*
	 *  Set up the second 8259 interrupt processor.
	 *  Make 8259 interrupts start at CPU vector Int0vec.
	 *  Set the 8259 as master with edge triggered
	 *  input with fully nested interrupts.
	 */
	EISAOUTB(Int1ctl, 0x11);	/* ICW1 - edge triggered, master,
					   ICW4 will be sent */
	EISAOUTB(Int1aux, Int1vec);	/* ICW2 - interrupt vector offset */
	EISAOUTB(Int1aux, 0x02);	/* ICW3 - I am a slave on level 2 */
	EISAOUTB(Int1aux, 0x01);	/* ICW4 - 8086 mode, not buffered */

	/*
	 *  pass #2 8259 interrupts to #1
	 */

	/* enable all PC interrupts except the clock */
	int0mask = 0;
	int0mask |= 1<<(Clockvec&7);
	EISAOUTB(Int0aux, int0mask);

	int1mask = 0;
	EISAOUTB(Int1aux, int1mask);

	IO(ulong, R4030ier) = 0xf;	/* enable eisa interrupts */
}

/*
 * Pull the ethernet address out of NVRAM
 */
void
enetaddr(uchar *ea)
{
	int i;
	uchar tbuf[8];

	for(i = 0; i < 8; i++)
		tbuf[i] = ((uchar*)(NvramRO+Enetoffset))[i];

	print("ether:");
	for(i = 0; i < 6; i++) {
		ea[i] = tbuf[7-i];
		print("%2.2ux", ea[i]);
	}
	print("\n");
}

/*
 * All DMA and ether IO buffers must reside in the first 16M bytes of
 * memory to be covered by the translation registers
 */
void
iomapinit(void)
{
	int i;
	Tte *t;

	t = xspanalloc(Ntranslation*sizeof(Tte), BY2PG, 0);

	for(i = 0; i < Ntranslation; i++)
		t[i].lo = i<<PGSHIFT;

	/* Set the translation table */
	IO(ulong, Ttbr) = PADDR(t);
	IO(ulong, Tlrb) = Ntranslation*sizeof(Tte);

	/* Invalidate the old entries */
	IO(ulong, Tir) = 0;

	/* Invalidate R4030 I/O cache */
	for(i=0; i<8; i++){
		*(ulong*)0xE0000034 = i<<2;
		*(ulong*)0xE000004C = 0;
	}
}

/*
 *  setup MIPS trap vectors
 */
void
vecinit(void)
{
	memmove((ulong*)UTLBMISS, (ulong*)vector0, 0x80);
	memmove((ulong*)XEXCEPTION, (ulong*)vector0, 0x80);
	memmove((ulong*)CACHETRAP, (ulong*)vector100, 0x80);
	memmove((ulong*)EXCEPTION, (ulong*)vector180, 0x80);

	icflush((ulong*)UTLBMISS, 8*1024);
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
		sprint(buf, "carrera %s R4400PC", conffile);
		ksetenv("terminal", buf);
		ksetenv("sysname", sysname);
		poperror();
	}

	kproc("alarm", alarmkproc, 0);
	touser((uchar*)(USTKTOP-sizeof(argbuf)));
}

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
	uchar *vec;

	USED(type);

	lock(&active);
	active.machs &= ~(1<<m->machno);
	active.exiting = 1;
	unlock(&active);

	spllo();
	print("cpu%d exiting\n", m->machno);
	while(consactive())
		delay(10);

	splhi();
	if(type == 1)
		rdb();

	/* Turn off the NMI hander for the debugger */
	vec = (uchar*)0xA0000420;
	vec[0] = 0;

	firmware(type);
}

ulong
memsize(void)
{
	ulong gcr;

	gcr = IO(ulong, MCTgcr);
	/* Check for bank one enable -
	 * assumes both have 4Meg SIMMS
	 */
	if(gcr & 0x20){
		memset(UNCACHED(void, 16*MB), 0, 16*MB);
		return 32*MB;
	}
	return 16*MB;
}

void
confinit(void)
{
	ulong ktop, top;

	ktop = PGROUND((ulong)end);
	ktop = PADDR(ktop);
	top = memsize()/BY2PG;

	conf.base0 = 0;
	conf.npage0 = top;
	conf.npage = conf.npage0;
	conf.npage0 -= ktop/BY2PG;
	conf.base0 += ktop;
	conf.npage1 = 0;
	conf.base1 = 0;

	if(top > 16*MB/BY2PG){
		conf.upages = (conf.npage*60)/100;
		imagmem->minarena = 4*1024*1024;
	}else
		conf.upages = (conf.npage*40)/100;
	conf.ialloc = ((conf.npage-conf.upages)/2)*BY2PG;

	conf.nmach = 1;

	/* set up other configuration parameters */
	conf.nproc = 100;
	conf.nswap = conf.npage*3;
	conf.nswppo = 4096;
	conf.nimage = 200;

	conf.monitor = 1;

	conf.copymode = 0;		/* copy on write */
}

void
buzz(int f, int d)
{
	USED(f);
	USED(d);
}

int
cistrcmp(char *a, char *b)
{
	int ac, bc;

	for(;;){
		ac = *a++;
		bc = *b++;
	
		if(ac >= 'A' && ac <= 'Z')
			ac = 'a' + (ac - 'A');
		if(bc >= 'A' && bc <= 'Z')
			bc = 'a' + (bc - 'A');
		ac -= bc;
		if(ac)
			return ac;
		if(bc == 0)
			break;
	}
	return 0;
}

/*
 * dummy routine for interoperability with pc's
 */
int
isaconfig(char *class, int ctlrno, ISAConf *isa)
{
	if(cistrcmp(class, "audio") == 0 && ctlrno == 0){
		strcpy(isa->type, "sb16");
		return 1;
	}
	if(cistrcmp(class, "ether") == 0 && ctlrno == 0){
		strcpy(isa->type, "sonic");
		return 1;
	}
	return 0;
}

/*
	register offsets of ARCS prom jmpbuf
		JB_PC		0
		JB_SP		1
		JB_FP		2
		JB_S0		3
		JB_S1		4
		JB_S2		5
		JB_S3		6
		JB_S4		7
		JB_S5		8
		JB_S6		9
		JB_S7		10
*/

struct 
{
	ulong	pc;
	ulong	sp;
	ulong	fp;
	ulong	s[7];
} Mipsjmpbuf;

void
rdbginit(void)
{
	uchar *vec;
	ulong jba;
return;/**/
	/* Only interested in the PC */
	Mipsjmpbuf.pc = 0xA001C020;

	/* Link an NMI handler to the debugger
	 * - addresses from the ARCS rom source
	 */
	vec = (uchar*)0xA0000420;
	jba = (ulong)UNCACHED(void, &Mipsjmpbuf);

	vec[0] = 'N';
	vec[1] = 'm';
	vec[2] = 'i';
	vec[3] = 's';
	vec[4] = jba>>24;
	vec[5] = jba>>16;
	vec[6] = jba>>8;
	vec[7] = jba;

	/* Install the debugger code in a known place */
	memmove((void*)0xA001C000, rdbgcode, rdbglen);
}
