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
char screenldepth[NAMELEN];

Conf	conf;
extern int invrtsdtr;

void
main(void)
{
	active.exiting = 0;
	active.machs = 1;
	machinit();
	arginit();
	kmapinit();
	confinit();
	xinit();
	printinit();
	tlbinit();
	ioinit();
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
duartintr(Ureg*, void*)
{
	sccintr();
}

void
ioinit(void)
{
	invrtsdtr = 1;
	sccsetup(IO(void, DUART0), SCCFREQ, 0);
	sccspecial(1, &printq, &kbdq, 9600);

	setvector(IVDUART, duartintr, 0);
	setvector(IVLCL2, l2intr, 0);
}

void
machinit(void)
{
	int n;

	cleancache();

	n = m->machno;
	memset(m, 0, sizeof(Mach));
	m->machno = n;
	m->stb = &stlb[n][0];
	m->ktb = &ktlb[0];
	m->speed = 50;

	/* Disable all interrupt sources */
	*IO(uchar, LIO_0_MASK) = 0x00;
	*IO(uchar, LIO_1_MASK) = 0x00;
	*IO(uchar, LIO_2_MASK) = 0x00;
}

void
tlbinit(void)
{
	int i;

	for(i=0; i<NTLB; i++)
		puttlbx(i, KZERO | PTEPID(i), 0, 0);
}

void
vecinit(void)
{
	memmove((ulong*)UTLBMISS, (ulong*)vector0, 0x100);
	memmove((ulong*)CACHETRAP, (ulong*)vector100, 0x80);
	memmove((ulong*)EXCEPTION, (ulong*)vector180, 0x80);
	icflush((ulong*)UTLBMISS, 32*1024);
}

void
init0(void)
{
	char buf[32];

	m->proc = u->p;
	u->p->state = Running;
	u->p->mach = m;
	spllo();

	u->slash = (*devtab[0].attach)(0);
	u->dot = clone(u->slash, 0);

	kproc("alarm", alarmkproc, 0);
	chandevinit();

	if(!waserror()){
		ksetterm("sgi %s 4000");
		ksetenv("cputype", "mips");
		sprint(buf, "0x%.4ux", prid());
		ksetenv("cpuprid", buf);
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
	icflush((uchar*)VA(k), sizeof initcode);
	kunmap(k);

	ready(p);
}

void
exit(int ispanic)
{
	int i;

	u = 0;
	wipekeys();
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

/* 
 *	get memory configuration word for a bank
 */
ulong
bank_conf(int bank)
{
	switch(bank){
	case 0:
		return *(ulong *)(KSEG1|MEMCFG0) >> 16;
	case 1:
		return *(ulong *)(KSEG1|MEMCFG0) & 0xffff;
	case 2:
		return *(ulong *)(KSEG1|MEMCFG1) >> 16;
	case 3:
		return *(ulong *)(KSEG1|MEMCFG1) & 0xffff;
	}
	return 0;
}

ulong
meminit(void)
{
	int i, n;
	ulong mconf, addr, size, mbytes = 0;

	for(i=0,n=0; i<4; i++){
		mconf = bank_conf(i);
		if(!(mconf & 0x2000) || n >= 2)
			continue;
		addr = (mconf & 0xff) << 22;
		size = ((mconf & 0x1f00) + 0x0100) << 14;
		mbytes += size/(1024*1024);
		switch(n++){
		case 0:
			conf.npage0 = size/BY2PG;
			conf.base0 = addr;
			break;
		case 1:
			conf.npage1 = size/BY2PG;
			conf.base1 = addr;
			break;
		}
	}
	return mbytes;
}

void
confinit(void)
{
	long mbytes;
	int mul;
	ulong ktop;

	mbytes = meminit();

	conf.npage = conf.npage0+conf.npage1;
	conf.upages = (conf.npage*70)/100;

	ktop = PGROUND((ulong)end);
	ktop = PADDR(ktop) - conf.base0;
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
	for(i = 0; i < _argc; i++){
		if(strchr(_argv[i], '='))
			break;
		av[i] = pusharg(_argv[i]);
	}
	av[i] = 0;
}

void
buzz(int, int)
{
}

void
lights(int)
{
}

int
mouseputc(IOQ*, int)
{
	return 0;
}
