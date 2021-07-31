#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"init.h"

Softtlb stlb[MAXMACH][STLBSIZE];		/* software tlb simulation */
int	_argc;					/* args passed by boot process */
char 	**_argv;
char	**_env;
char 	argbuf[128];				/* arguments passed to initcode */
int	argsize;
char	consname[NAMELEN];			/* environment vars from NVRAM */
char	diskless[NAMELEN];
char	confbuf[4*1024];			/* config file read by boot program */
int	ioid;					/* IO board type */

void
main(void)
{
	machinit();
	active.exiting = 0;
	active.machs = 1;
	arginit();
	confinit();
	lockinit();
	xinit();
	printinit();
	duartspecial(0, &printq, &kbdq, 9600);
	pageinit();
	tlbinit();
	vecinit();
	procinit0();
	initseg();
	clockinit();
	ioboardinit();
	chandevreset();
	streaminit();
	swapinit();
	userinit();
	launchinit();
	schedinit();
}

void
machinit(void)
{
	int n;

	icflush(0, 64*1024);
	n = m->machno;
	memset(m, 0, sizeof(Mach));
	m->machno = n;
	m->stb = &stlb[n][0];
	duartinit();

	m->ledval = 0xff;
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

/*
 *  reset the vme bus
 */
void
vmereset(void)
{
	int noforce;

	if(ioid >= IO3R1)
		noforce = 1;
	else
		noforce = 0;
	MODEREG->resetforce = (1<<1) | noforce;
	delay(140);
	MODEREG->resetforce = noforce;
}

/*
 *  We have to program both the IO2 board to generate interrupts
 *  and the SBCC on CPU 0 to accept them.
 */
void
ioboardinit(void)
{
	long i;
	int maxlevel;

	ioid = *IOID;
	if(ioid >= IO3R1)
		maxlevel = 8;
	else
		maxlevel = 8;

	vmereset();
	MODEREG->masterslave = (SLAVE<<4) | MASTER;

	/*
	 *  all VME interrupts to the error routine
	 */
	for(i=0; i<256; i++)
		setvmevec(i, novme);

	/*
	 *  tell IO2 to sent all interrupts to CPU 0's SBCC
	 */
	for(i=0; i<maxlevel; i++)
		INTVECREG->i[i].vec = 0<<8;

	/*
	 *  Tell CPU 0's SBCC to map all interrupts from the IO2 to MIPS level 5
	 *
	 *	0x01		level 0
	 *	0x02		level 1
	 *	0x04		level 2
	 *	0x08		level 4
	 *	0x10		level 5
	 */
	SBCCREG->flevel = 0x10;

	/*
	 *  Tell CPU 0's SBCC to enable all interrupts from the IO2.
	 *
	 *  The SBCC 16 bit registers are read/written as ulong, but only
	 *  bits 23-16 and 7-0 are meaningful.
	 */
	SBCCREG->fintenable |= 0xff;	  /* allow all interrupts on the IO2 */
	SBCCREG->idintenable |= 0x800000; /* allow interrupts from the IO2 */

	/*
	 *  Enable all interrupts on the IO2.  If IO3, run in compatibility mode.
	 */
	*IO2SETMASK = 0xff000000;

}

void
launchinit(void)
{
	int i;

	for(i=1; i<conf.nmach; i++)
		launch(i);
	for(i=0; i<1000000; i++)
		if(active.machs == (1<<conf.nmach) - 1){
			print("all launched\n");
			return;
		}
	print("launch: active = %x\n", active.machs);
}


void
init0(void)
{
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
	chandevinit();

	if(!waserror()){
		ksetenv("cputype", "mips");
		ksetterm("sgi %s 4D");
		ksetenv("sysname", sysname);
		poperror();
	}

	if(conf.debugger && conf.nmach > 2)
		kproc("kdebug", debugger, 0);
	kproc("alarm", alarmkproc, 0);
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
	up->fpsave.fpstatus = initfp.fpstatus;
	kunmap(k);

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

void
lights(int v)
{

	*LED = ~v;
}

typedef struct Beef	Beef;
struct	Beef
{
	long	deadbeef;
	long	sum;
	long	cpuid;
	long	virid;
	long	erno;
	void	(*launch)(void);
	void	(*rend)(void);
	long	junk1[4];
	long	isize;
	long	dsize;
	long	nonbss;
	long	junk2[18];
};

void
launch(int n)
{
	Beef *p;
	long i, s;
	ulong *ptr;

	p = (Beef*) 0xb0000500 + n;
	p->launch = newstart;
	p->sum = 0;
	s = 0;
	ptr = (ulong*)p;
	for (i = 0; i < sizeof(Beef)/sizeof(ulong); i++)
		s += *ptr++;
	p->sum = -(s+1);

	for(i=0; i<3000000; i++)
		if(p->launch == 0)
			break;
}

void
online(void)
{

	machinit();
	lock(&active);
	active.machs |= 1<<m->machno;
	unlock(&active);
	tlbinit();
	clockinit();
	schedinit();
}

void
exit(int ispanic)
{
	int i;

	USED(ispanic);
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
	duartenable0();
	firmware(cpuserver ? PROM_AUTOBOOT : PROM_REINIT);
}

typedef struct Conftab {
	char *sym;
	ulong *x;
} Conftab;

#include "conf.h"

Conf	conf;

ulong
confeval(char *exp)
{
	char *op;
	Conftab *ct;

	/* crunch leading white */
	while(*exp==' ' || *exp=='\t')
		exp++;

	op = strchr(exp, '+');
	if(op != 0){
		*op++ = 0;
		return confeval(exp) + confeval(op);
	}

	op = strchr(exp, '*');
	if(op != 0){
		*op++ = 0;
		return confeval(exp) * confeval(op);
	}

	if(*exp >= '0' && *exp <= '9')
		return strtoul(exp, 0, 0);

	/* crunch trailing white */
	op = strchr(exp, ' ');
	if(op)
		*op = 0;
	op = strchr(exp, '\t');
	if(op)
		*op = 0;

	/* lookup in symbol table */
	for(ct = conftab; ct->sym; ct++)
		if(strcmp(exp, ct->sym) == 0)
			return *(ct->x);

	return 0;
}

/*
 *  each line of the configuration is of the form `param = expression'.
 */
void
confset(char *sym)
{
	char *val, *p;
	Conftab *ct;

	/*
 	 *  parse line
	 */

	/* comment */
	if(p = strchr(sym, '#'))
		*p = 0;

	/* skip white */
	for(p = sym; *p==' ' || *p=='\t'; p++)
		;
	sym = p;

	/* skip sym */
	for(; *p && *p!=' ' && *p!='\t' && *p!='='; p++)
		;
	if(*p)
		*p++ = 0;

	/* skip white */
	for(; *p==' ' || *p=='\t' || *p=='='; p++)
		;
	val = p;

	/*
	 *  lookup value
	 */
	for(ct = conftab; ct->sym; ct++)
		if(strcmp(sym, ct->sym) == 0){
			*(ct->x) = confeval(val);
			return;
		}

	if(strcmp(sym, "sysname")==0){
		p = strchr(val, ' ');
		if(p)
			*p = 0;
		strcpy(sysname, val);
	} else if(strcmp(sym, "eve")==0){
		p = strchr(val, ' ');
		if(p)
			*p = 0;
		strcpy(eve, val);
	}
}

/*
 *  read the ascii configuration left by the boot kernel
 */
void
confread(void)
{
	char *line;
	char *end;

	/*
	 *  process configuration file
	 */
	line = confbuf;
	while(end = strchr(line, '\n')){
		*end = 0;
		confset(line);
		line = end+1;
	}
}

void
confprint(void)
{
	Conftab *ct;

	/*
	 *  lookup value
	 */
	for(ct = conftab; ct->sym; ct++)
		print("%s == %d\n", ct->sym, *ct->x);
}

void
confinit(void)
{
	long x, i, *l;
	ulong ktop;

	/*
	 *  copy configuration down from high memory
	 */
	strcpy(confbuf, (char *)(0x80000000 + (4*MB) - BY2PG));

	/*
	 *  size memory
	 */
	x = 0x12345678;
	for(i=4; i<128; i+=4){
		l = (long*)(KSEG1|(i*MB));
		*l = x;
		wbflush();
		*(ulong*)KSEG1 = *(ulong*)KSEG1;	/* clear latches */
		if(*l != x)
			break;
		x += 0x3141526;
	}
	conf.npage0 = i*1024/4;
	conf.base0 = 0;
	conf.npage = conf.npage0;
	conf.npage1 = 0;
	conf.base1 = 0;

	ktop = PGROUND((ulong)end);
	ktop = PADDR(ktop);
	conf.npage0 -= ktop/BY2PG;
	conf.base0 += ktop;

	conf.upages = (conf.npage*70)/100;
	i = conf.npage-conf.upages;
	if(i > (12*MB)/BY2PG)
		conf.upages +=  i - ((12*MB)/BY2PG);
	/*
 	 *  clear MP bus error caused by sizing memory
	 */
	i = *SBEADDR;
	USED(i);

	/*
	 *  set minimal default values
	 */
	conf.nmach = 1;
	conf.nproc = 100;
	conf.nswap = 262144;
	conf.nimage = 200;
	conf.ipif = 8;
	conf.ip = 64;
	conf.arp = 32;
	conf.frag = 32;

	confread();

	if(conf.nmach > MAXMACH)
		panic("confinit");

	conf.copymode = 1;		/* copy on reference */
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
	{"diskless=",	diskless},
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
		if(i && *(_argv[i]) != '-')
			diskless[0] = 0;
		av[i] = pusharg(_argv[i]);
	}

	/*
	 *  if no boot method is specified, look for
	 *  a default in the diskless environment variable
	 */
	if(diskless[0] > '1')
		av[i++] = pusharg(diskless);
	av[i] = 0;
}

/*
 *  setup the IO2 lance, io buffers are in lance memory
 */
void
lanceIO2setup(Lance *lp)
{
	ushort *sp;

	/*
	 *  reset lance and set parity on its memory
	 */
	MODEREG->promenet &= ~1;
	MODEREG->promenet |= 1;
	for(sp = LANCERAM; sp < LANCEEND; sp += 1)
		*sp = 0;

	lp->sep = 1;
	lp->lanceram = LANCERAM;
	lp->lm = (Lancemem*)0;

	/*
	 *  Allocate space in lance memory for the io buffers.
	 *  Start at 4k to avoid the initialization block and
	 *  descriptor rings.
	 */
	lp->lrp = (Etherpkt*)(4*1024);
	lp->ltp = lp->lrp + lp->nrrb;
	lp->rp = (Etherpkt*)(((ulong)LANCERAM) + (ulong)lp->lrp);
	lp->tp = lp->rp + lp->nrrb;
}

/*
 *  setup the IO3 lance, io buffers are in host memory mapped to
 *  lance address space
 */
void
lanceIO3setup(Lance *lp)
{
	ulong x, y;
	int index;
	ushort *sp;
	int len;

	/*
	 *  reset lance and set parity on its memory
	 */
	MODEREG->promenet |= 1;
	MODEREG->promenet &= ~1;
	for(sp = LANCE3RAM; sp < LANCE3END; sp += 2)
		*sp = 0;

	lp->sep = 4;
	lp->lanceram = LANCE3RAM;
	lp->lm = (Lancemem*)0x800000;

	/*
	 *  allocate some host memory for buffers and map it into lance
	 *  space
	 */
	len = (lp->nrrb + lp->ntrb)*sizeof(Etherpkt);
	lp->rp = (Etherpkt*)xspanalloc(len , BY2PG, 0);
	lp->tp = lp->rp + lp->nrrb;
	x = (ulong)lp->rp;
	lp->lrp = (Etherpkt*)(x & 0xFFF);
	lp->ltp = lp->lrp + lp->nrrb;
	index = LANCEINDEX;
	for(y = x+len; x < y; x += 0x1000){
		*WRITEMAP = (index<<16) | (x>>12)&0xFFFF;
		index++;
	}
}

/*
 *  set up the lance
 */
void
lancesetup(Lance *lp)
{
	lp->rap = LANCERAP;
	lp->rdp = LANCERDP;
	lp->ea[0] = LANCEID[20]>>8;
	lp->ea[1] = LANCEID[16]>>8;
	lp->ea[2] = LANCEID[12]>>8;
	lp->ea[3] = LANCEID[8]>>8;
	lp->ea[4] = LANCEID[4]>>8;
	lp->ea[5] = LANCEID[0]>>8;
	lp->lognrrb = 7;
	lp->logntrb = 7;
	lp->nrrb = 1<<lp->lognrrb;
	lp->ntrb = 1<<lp->logntrb;
	lp->busctl = BSWP;
	if(ioid >= IO3R1)
		lanceIO3setup(lp);
	else
		lanceIO2setup(lp);
}

void
lanceparity(void)
{
	print("lance DRAM parity error\n");
	MODEREG->promenet &= ~4;
	MODEREG->promenet |= 4;
}

/*
 *  for the sake of a single devcons.c
 */
void
buzz(int f, int d)
{
	USED(f);
	USED(d);
}

int
mouseputc(IOQ *q, int c)
{
	USED(q);
	USED(c);
	return 0;
}
