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

void
main(void)
{
	*IO(uchar, LIO_0_MASK) = 0;
	*IO(uchar, LIO_1_MASK) = 0;
	active.exiting = 0;
	active.machs = 1;
	machinit();
	arginit();
	confinit();
	xinit();
	printinit();
	tlbinit();

	sccsetup(IO(void, DUART0), SCCFREQ, 0);
	sccsetup(IO(void, DUART1), SCCFREQ, 0);
	kbdinit();
	*IO(uchar, LIO_0_MASK) |= LIO_DUART;
	screeninit(0);

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
		ksetterm("sgi %s 3000");
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

/* 
 *	get memory configuration word for a bank
 */
ulong
bank_conf(int bank)
{
	switch(bank){
	case 0:
		return *(ulong *)(KSEG1|MEM_CONF_ADDR0) >> 16;
	case 1:
		return *(ulong *)(KSEG1|MEM_CONF_ADDR0) & 0xffff;
	case 2:
		return *(ulong *)(KSEG1|MEM_CONF_ADDR1) >> 16;
	case 3:
		return *(ulong *)(KSEG1|MEM_CONF_ADDR1) & 0xffff;
	}
	return 0x003f;
}

ulong
meminit(void)
{
	int i, n;
	ulong mconf, addr, size, mbytes = 0;

	for(i=0,n=0; i<4; i++){
		mconf = bank_conf(i);
		if(mconf == 0x003f || n >= 2)
			continue;
		addr = (mconf & 0x3f) << 22;
		size = ((mconf & 0x0f00) + 0x0100) << 14;
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
	for(i = 0; i < _argc; i++)
		av[i] = pusharg(_argv[i]);
	av[i] = 0;
}
