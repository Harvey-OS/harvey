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

/* where b.com leaves configuration info */
#define BOOTARGS	((char*)(KZERO|1024))
#define	BOOTARGSLEN	1024
#define	MAXCONF		32

char bootdisk[NAMELEN];
char *confname[MAXCONF];
char *confval[MAXCONF];
int nconf;

/* memory map */
#define MAXMEG 64
char mmap[MAXMEG+2];

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
	dmainit();
	screeninit();
	printinit();
	mmuinit();
	pageinit();
	trapinit();
	mathinit();
	clockinit();
	printcpufreq();
	faultinit();
	kbdinit();
	procinit0();
	initseg();
	streaminit();
	chandevreset();
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
	int i;
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
		for(i = 0; i < nconf; i++)
			if(confname[i])
				ksetenv(confname[i], confval[i]);
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
	av[ac++] = pusharg("/386/9pc");
	cp[64] = 0;
	buf[0] = 0;

	/*
	 *  decode the b.com bootline and convert to
	 *  a disk device name to pass to the boot
	 */
	if(strncmp(cp, "fd!", 3) == 0){
		sprint(buf, "local!#f/fd%ddisk", atoi(cp+3));
		av[ac++] = pusharg(buf);
	} else if(strncmp(cp, "h!", 2) == 0){
		sprint(buf, "local!#H/hd%dfs", atoi(cp+2));
		av[ac++] = pusharg(buf);
	} else if(strncmp(cp, "hd!", 3) == 0){
		sprint(buf, "local!#H/hd%ddisk", atoi(cp+3));
		av[ac++] = pusharg(buf);
	} else if(strncmp(cp, "s!", 2) == 0){
		sprint(buf, "local!#w%d/sd%dfs", atoi(cp+2), atoi(cp+2));
		av[ac++] = pusharg(buf);
	} else if(strncmp(cp, "sd!", 3) == 0){
		sprint(buf, "local!#w%d/sd%ddisk", atoi(cp+3), atoi(cp+3));
		av[ac++] = pusharg(buf);
	} else if(getconf("bootdisk") == 0){
		if(conf.nhard){
			sprint(buf, "local!#H/hd0disk");
			av[ac++] = pusharg(buf);
		} else{
			sprint(buf, "local!#w/sd0disk");
			av[ac++] = pusharg(buf);
		}
	}
	if(buf[0]){
		cp = strchr(buf, '!');
		if(cp){
			strcpy(bootdisk, cp+1);
			addconf("bootdisk", bootdisk);
		}
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
addconf(char *name, char *val)
{
	if(nconf >= MAXCONF)
		return;
	confname[nconf] = name;
	confval[nconf] = val;
	nconf++;
}

char*
getconf(char *name)
{
	int i;

	for(i = 0; i < nconf; i++)
		if(strcmp(confname[i], name) == 0)
			return confval[i];
	return 0;
}

/*
 *  look for unused address space in 0xC8000 to 1 meg
 */
void
romscan(void)
{
	uchar *p;

	p = (uchar*)(KZERO+0xC8000);
	while(p < (uchar*)(KZERO+0xE0000)){
		p[0] = 0x55;
		p[1] = 0xAA;
		p[2] = 4;
		if(p[0] != 0x55 || p[1] != 0xAA){
			putisa(PADDR(p), 2048);
			p += 2048;
			continue;
		}
		p += p[2]*512;
	}

	p = (uchar*)(KZERO+0xE0000);
	if(p[0] != 0x55 || p[1] != 0xAA)
		putisa(PADDR(p), 64*1024);
}


void
confinit(void)
{
	long x, i, j, n;
	int pcnt;
	ulong ktop;
	char *cp;
	char *line[MAXCONF];

	pcnt = 0;

	/*
	 *  parse configuration args from dos file p9rc
	 */
	cp = BOOTARGS;	/* where b.com leaves plan9.ini */
	cp[BOOTARGSLEN-1] = 0;
	n = getfields(cp, line, MAXCONF, "\n");
	for(j = 0; j < n; j++){
		cp = strchr(line[j], '\r');
		if(cp)
			*cp = 0;
		cp = strchr(line[j], '=');
		if(cp == 0)
			continue;
		*cp++ = 0;
		if(cp - line[j] >= NAMELEN+1)
			*(line[j]+NAMELEN-1) = 0;
		confname[nconf] = line[j];
		confval[nconf] = cp;
		if(strcmp(confname[nconf], "kernelpercent") == 0)
			pcnt = 100 - atoi(confval[nconf]);
		nconf++;
	}
	/*
	 *  size memory above 1 meg. Kernel sits at 1 meg.  We
	 *  only recognize MB size chunks.
	 */
	memset(mmap, ' ', sizeof(mmap));
	x = 0x12345678;
	for(i = 1; i <= MAXMEG; i++){
		/*
		 *  write the first & last word in a megabyte of memory
		 */
		*mapaddr(KZERO|(i*MB)) = x;
		*mapaddr(KZERO|((i+1)*MB-BY2WD)) = x;

		/*
		 *  write the first and last word in all previous megs to
		 *  handle address wrap around
		 */
		for(j = 1; j < i; j++){
			*mapaddr(KZERO|(j*MB)) = ~x;
			*mapaddr(KZERO|((j+1)*MB-BY2WD)) = ~x;
		}

		/*
		 *  check for correct value
		 */
		if(*mapaddr(KZERO|(i*MB)) == x && *mapaddr(KZERO|((i+1)*MB-BY2WD)) == x){
			mmap[i] = 'x';
			/*
			 *  zero memory to set ECC but skip over the kernel
			 */
			if(i != 1)
				for(j = 0; j < MB/BY2PG; j += BY2PG)
					memset(mapaddr(KZERO|(i*MB+j)), 0, BY2PG);
		}
		x += 0x3141526;
	}
	/*
	 *  bank0 usually goes from the end of kernel bss to the end of memory
	 */
	ktop = PGROUND((ulong)end);
	ktop = PADDR(ktop);
	conf.base0 = ktop;
	for(i = 1; mmap[i] == 'x'; i++)
		;
	conf.npage0 = (i*MB - ktop)/BY2PG;
	conf.topofmem = i*MB;

	/*
	 *  bank1 usually goes from the end of BOOTARGS to 640k
	 */
	conf.base1 = (ulong)(BOOTARGS+BOOTARGSLEN);
	conf.base1 = PGROUND(conf.base1);
	conf.base1 = PADDR(conf.base1);
	conf.npage1 = (640*1024-conf.base1)/BY2PG;

	/*
	 *  if there is a hole in memory (due to a shadow BIOS) make the
	 *  memory after the hole be bank 1. The memory from 0 to 640k
	 *  is lost.
	 */
	for(; i <= MAXMEG; i++)
		if(mmap[i] == 'x'){
			conf.base1 = i*MB;
			for(j = i+1; mmap[j] == 'x'; j++)
				;
			conf.npage1 = (j - i)*MB/BY2PG;
			conf.topofmem = j*MB;
			break;
		}

	/*
 	 *  add address space holes holes under 16 meg to available
	 *  isa space.
	 */
	romscan();
	if(conf.topofmem < 16*MB)
		putisa(conf.topofmem, 16*MB - conf.topofmem);

	conf.npage = conf.npage0 + conf.npage1;
	conf.ldepth = 0;
	if(pcnt < 10)
		pcnt = 70;
	conf.upages = (conf.npage*pcnt)/100;

	conf.nproc = 30 + ((conf.npage*BY2PG)/MB)*8;
	conf.monitor = 1;
	conf.nswap = conf.nproc*80;
	conf.nimage = 50;
	switch(x86()){
	case 3:
		conf.copymode = 1;	/* copy on reference */
		break;
	default:
		conf.copymode = 0;	/* copy on write */
		break;
	}
	conf.nfloppy = 2;
	conf.nhard = 2;
	conf.nmach = 1;
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
matherror(Ureg *ur, void *a)
{
	ulong status;
	int i;
	char *msg;
	char note[ERRLEN];

	USED(a);

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
mathemu(Ureg *ur, void *a)
{
	USED(ur, a);

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
mathover(Ureg *ur, void *a)
{
	USED(ur, a);

print("sys: fp: math overrun pc 0x%lux pid %d\n", ur->pc, u->p->pid);
	pexit("math overrun", 0);
}

void
mathinit(void)
{
	setvec(Matherr1vec, matherror, 0);
	setvec(Matherr2vec, matherror, 0);
	setvec(Mathemuvec, mathemu, 0);
	setvec(Mathovervec, mathover, 0);
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
	wipekeys();
	print("exiting\n");
	if(ispanic){
		if(cpuserver)
			delay(10000);
		else
			for(;;);
	}

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

int
parseether(uchar *to, char *from)
{
	char nip[4];
	char *p;
	int i;

	p = from;
	while(*p == ' ')
		++p;
	for(i = 0; i < 6; i++){
		if(*p == 0)
			return -1;
		nip[0] = *p++;
		if(*p == 0)
			return -1;
		nip[1] = *p++;
		nip[2] = 0;
		to[i] = strtoul(nip, 0, 16);
		if(*p == ':')
			p++;
	}
	return 0;
}

int
isaconfig(char *class, int ctlrno, ISAConf *isa)
{
	char cc[NAMELEN], *p, *q;
	int n;

	sprint(cc, "%s%d", class, ctlrno);
	for(n = 0; n < nconf; n++){
		if(strncmp(confname[n], cc, NAMELEN))
			continue;
		p = confval[n];
		while(*p){
			while(*p == ' ' || *p == '\t')
				p++;
			if(*p == '\0')
				break;
			if(strncmp(p, "type=", 5) == 0){
				p += 5;
				for(q = isa->type; q < &isa->type[NAMELEN-1]; q++){
					if(*p == '\0' || *p == ' ' || *p == '\t')
						break;
					*q = *p++;
				}
				*q = '\0';
			}
			else if(strncmp(p, "port=", 5) == 0)
				isa->port = strtoul(p+5, &p, 0);
			else if(strncmp(p, "irq=", 4) == 0)
				isa->irq = strtoul(p+4, &p, 0);
			else if(strncmp(p, "mem=", 4) == 0)
				isa->mem = strtoul(p+4, &p, 0);
			else if(strncmp(p, "size=", 5) == 0)
				isa->size = strtoul(p+5, &p, 0);
			else if(strncmp(p, "dma=", 4) == 0)
				isa->dma = strtoul(p+4, &p, 0);
			else if(strncmp(p, "ea=", 3) == 0){
				if(parseether(isa->ea, p+3) == -1)
					memset(isa->ea, 0, 6);
			}
			while(*p && *p != ' ' && *p != '\t')
				p++;
		}
		return 1;
	}
	return 0;
}

static void
pcfloppyintr(Ureg *ur, void *a)
{
	USED(a);

	floppyintr(ur);
}

void
floppysetup0(FController *fl)
{
	USED(fl);
}

void
floppysetup1(FController *fl)
{
	uchar equip;

	/*
	 *  read nvram for types of floppies 0 & 1
	 */
	equip = nvramread(0x10);
	if(conf.nfloppy > 0){
		fl->d[0].dt = (equip >> 4) & 0xf;
		floppysetdef(&fl->d[0]);
	}
	if(conf.nfloppy > 1){
		fl->d[1].dt = equip & 0xf;
		floppysetdef(&fl->d[1]);
	}

	setvec(Floppyvec, pcfloppyintr, 0);
}

/*
 *  eject disk ( unknown on safari )
 */
void
floppyeject(FDrive *dp)
{
	floppyon(dp);
	dp->vers++;
	floppyoff(dp);
}

int 
floppyexec(char *a, long b, int c)
{
	USED(a, b, c);
	return b;
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
