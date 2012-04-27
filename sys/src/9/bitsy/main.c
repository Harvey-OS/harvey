#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"init.h"
#include	"pool.h"

Mach	*m;
Proc	*up;
Conf	conf;
int 	noprint;

void
main(void)
{
	mmuinvalidate();

	/* zero out bss */
	memset(edata, 0, end-edata);

	/* point to Mach structure */
	m = (Mach*)MACHADDR;
	memset(m, 0, sizeof(Mach));
	m->ticks = 1;

	active.machs = 1;

	rs232power(1);
	quotefmtinstall();
	iprint("\nPlan 9 bitsy kernel\n");
	confinit();
	xinit();
	mmuinit();
	machinit();
	trapinit();
	sa1110_uartsetup(1);
	dmainit();
	screeninit();
	printinit();	/* from here on, print works, before this we need iprint */
	clockinit();
	procinit0();
	initseg();
	links();
	chandevreset();
	pageinit();
	swapinit();
	userinit();
	powerinit();
	schedinit();
}

/* need to do better */
void
reboot(void*, void*, ulong)
{
	exit(0);
}


/*
 *  exit kernel either on a panic or user request
 */
void
exit(int ispanic)
{
	void (*f)(void);

	USED(ispanic);
	delay(1000);

	iprint("it's a wonderful day to die\n");
	cacheflush();
	mmuinvalidate();
	mmudisable();
	f = nil;
	(*f)();
}

static uchar *sp;

/*
 *  starting place for first process
 */
void
init0(void)
{
	up->nerrlab = 0;

	spllo();

	/*
	 * These are o.k. because rootinit is null.
	 * Then early kproc's will have a root and dot.
	 */
	up->slash = namec("#/", Atodir, 0, 0);
	pathclose(up->slash->path);
	up->slash->path = newpath("/");
	up->dot = cclone(up->slash);

	chandevinit();

	if(!waserror()){
		ksetenv("terminal", "bitsy", 0);
		ksetenv("cputype", "arm", 0);
		if(cpuserver)
			ksetenv("service", "cpu", 0);
		else
			ksetenv("service", "terminal", 0);
		poperror();
	}
	kproc("alarm", alarmkproc, 0);
	kproc("power", powerkproc, 0);

	touser(sp);
}

/*
 *  pass boot arguments to /boot
 */
static uchar *
pusharg(char *p)
{
	int n;

	n = strlen(p)+1;
	sp -= n;
	memmove(sp, p, n);
	return sp;
}
static void
bootargs(ulong base)
{
 	int i, ac;
	uchar *av[32];
	uchar *bootpath;
	uchar **lsp;

	/*
 	 *  the sizeof(Sargs) is to make the validaddr check in
	 *  trap.c's syscall() work even when we have less than the
	 *  max number of args.
	 */
	sp = (uchar*)base + BY2PG - sizeof(Sargs);

	bootpath = pusharg("/boot/boot");
	ac = 0;
	av[ac++] = pusharg("boot");

	/* 4 byte word align stack */
	sp = (uchar*)((ulong)sp & ~3);

	/* build argc, argv on stack */
	sp -= (ac+1)*sizeof(sp);
	lsp = (uchar**)sp;
	for(i = 0; i < ac; i++)
		*lsp++ = av[i] + ((USTKTOP - BY2PG) - base);
	*lsp = 0;

	/* push argv onto stack */
	sp -= BY2WD;
	lsp = (uchar**)sp;
	*lsp = sp + BY2WD + ((USTKTOP - BY2PG) - base);

	/* push pointer to "/boot" */
	sp -= BY2WD;
	lsp = (uchar**)sp;
	*lsp = bootpath + ((USTKTOP - BY2PG) - base);

	/* leave space for where the initcode's caller's return PC would normally reside */
	sp -= BY2WD;

	/* relocate stack to user's virtual addresses */
	sp += (USTKTOP - BY2PG) - base;
}

/*
 *  create the first process
 */
void
userinit(void)
{
	Proc *p;
	Segment *s;
	KMap *k;
	Page *pg;

	/* no processes yet */
	up = nil;

	p = newproc();
	p->pgrp = newpgrp();
	p->egrp = smalloc(sizeof(Egrp));
	p->egrp->ref = 1;
	p->fgrp = dupfgrp(nil);
	p->rgrp = newrgrp();
	p->procmode = 0640;

	kstrdup(&eve, "");
	kstrdup(&p->text, "*init*");
	kstrdup(&p->user, eve);

	/*
	 * Kernel Stack
	 */
	p->sched.pc = (ulong)init0;
	p->sched.sp = (ulong)p->kstack+KSTACK-(sizeof(Sargs)+BY2WD);

	/*
	 * User Stack
	 */
	s = newseg(SG_STACK, USTKTOP-USTKSIZE, USTKSIZE/BY2PG);
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
	pg = newpage(1, 0, UTZERO);
	memset(pg->cachectl, PG_TXTFLUSH, sizeof(pg->cachectl));
	segpage(s, pg);
	k = kmap(s->map[0]->pages[0]);
	memmove((ulong*)VA(k), initcode, sizeof initcode);
	kunmap(k);

	ready(p);
}

/*
 *  set mach dependent process state for a new process
 */
void
procsetup(Proc *p)
{
	p->fpstate = FPinit;
}

/*
 *  Save the mach dependent part of the process state.
 */
void
procsave(Proc *p)
{
	USED(p);
}

/* place holder */
/*
 *  dummy since rdb is not included 
 */
void
rdb(void)
{
}

/*
 *  probe the last location in a meg of memory, make sure it's not
 *  reflected into something else we've already found.
 */
int
probemem(ulong addr)
{
	int i;
	ulong *p;
	ulong a;

	addr += OneMeg - sizeof(ulong);
	p = (ulong*)addr;
	*p = addr;
	for(i=0; i<nelem(conf.mem); i++){
		for(a = conf.mem[i].base+OneMeg-sizeof(ulong); a < conf.mem[i].limit; a += OneMeg){
			p = (ulong*)a;
			*p = 0;
		}
	}
	p = (ulong*)addr;
	if(*p != addr)
		return -1;
	return 0;
}

/*
 *  we assume that the kernel is at the beginning of one of the
 *  contiguous chunks of memory.
 */
void
confinit(void)
{
	int i, j;
	ulong addr;
	ulong ktop;

	/* find first two contiguous sections of available memory */
	addr = PHYSDRAM0;
	for(i=0; i<nelem(conf.mem); i++){
		conf.mem[i].base = addr;
		conf.mem[i].limit = addr;
	}
	for(j=0; j<nelem(conf.mem); j++){
		conf.mem[j].base = addr;
		conf.mem[j].limit = addr;
		
		for(i = 0; i < 512; i++){
			if(probemem(addr) == 0)
				break;
			addr += OneMeg;
		}
		for(; i < 512; i++){
			if(probemem(addr) < 0)
				break;
			addr += OneMeg;
			conf.mem[j].limit = addr;
		}
	}
	
	conf.npage = 0;
	for(i=0; i<nelem(conf.mem); i++){
		/* take kernel out of allocatable space */
		ktop = PGROUND((ulong)end);
		if(ktop >= conf.mem[i].base && ktop <= conf.mem[i].limit)
			conf.mem[i].base = ktop;
		
		/* zero memory */
		memset((void*)conf.mem[i].base, 0, conf.mem[i].limit - conf.mem[i].base);

		conf.mem[i].npage = (conf.mem[i].limit - conf.mem[i].base)/BY2PG;
		conf.npage += conf.mem[i].npage;
	}

	if(conf.npage > 16*MB/BY2PG){
		conf.upages = (conf.npage*60)/100;
		imagmem->minarena = 4*1024*1024;
	}else
		conf.upages = (conf.npage*40)/100;
	conf.ialloc = ((conf.npage-conf.upages)/2)*BY2PG;

	/* only one processor */
	conf.nmach = 1;

	/* set up other configuration parameters */
	conf.nproc = 100;
	conf.nswap = conf.npage*3;
	conf.nswppo = 4096;
	conf.nimage = 200;

	conf.monitor = 1;

	conf.copymode = 0;		/* copy on write */
}

GPIOregs *gpioregs;
ulong *egpioreg = (ulong*)EGPIOREGS;
PPCregs *ppcregs;
MemConfRegs *memconfregs;
PowerRegs *powerregs;
ResetRegs *resetregs;

/*
 *  configure the machine
 */
void
machinit(void)
{
	/* set direction of SA1110 io pins and select alternate functions for some */
	gpioregs = mapspecial(GPIOREGS, sizeof(GPIOregs));
	gpioregs->direction = 
		GPIO_LDD8_o|GPIO_LDD9_o|GPIO_LDD10_o|GPIO_LDD11_o
		|GPIO_LDD12_o|GPIO_LDD13_o|GPIO_LDD14_o|GPIO_LDD15_o
		|GPIO_CLK_SET0_o|GPIO_CLK_SET1_o
		|GPIO_L3_SDA_io|GPIO_L3_MODE_o|GPIO_L3_SCLK_o
		|GPIO_COM_RTS_o;
	gpioregs->rising = 0;
	gpioregs->falling = 0;
	gpioregs->altfunc |= 
		GPIO_LDD8_o|GPIO_LDD9_o|GPIO_LDD10_o|GPIO_LDD11_o
		|GPIO_LDD12_o|GPIO_LDD13_o|GPIO_LDD14_o|GPIO_LDD15_o
		|GPIO_SSP_CLK_i;

	/* map in special H3650 io pins */
	egpioreg = mapspecial(EGPIOREGS, sizeof(ulong));

	/* map in peripheral pin controller (ssp will need it) */
	ppcregs = mapspecial(PPCREGS, sizeof(PPCregs));

	/* SA1110 power management */
	powerregs = mapspecial(POWERREGS, sizeof(PowerRegs));

	/* memory configuraton */
	memconfregs = mapspecial(MEMCONFREGS, sizeof(MemConfRegs));

	/* reset controller */
	resetregs = mapspecial(RESETREGS, sizeof(ResetRegs));
}


/*
 *  manage egpio bits
 */
static ulong egpiosticky;

void
egpiobits(ulong bits, int on)
{
	if(on)
		egpiosticky |= bits;
	else
		egpiosticky &= ~bits;
	*egpioreg = egpiosticky;
}

void
rs232power(int on)
{
	egpiobits(EGPIO_rs232_power, on);
	delay(50);
}

void
audioamppower(int on)
{
	egpiobits(EGPIO_audio_power, on);
	delay(50);
}

void
audioicpower(int on)
{
	egpiobits(EGPIO_audio_ic_power, on);
	delay(50);
}

void
irpower(int on)
{
	egpiobits(EGPIO_ir_power, on);
	delay(50);
}

void
lcdpower(int on)
{
	egpiobits(EGPIO_lcd_3v|EGPIO_lcd_ic_power|EGPIO_lcd_5v|EGPIO_lcd_9v, on);
	delay(50);
}

void
flashprogpower(int on)
{
	egpiobits(EGPIO_prog_flash, on);
}

/* here on hardware reset */
void
resettrap(void)
{
}

/*
 *  for drivers that used to run on x86's
 */
void
outb(ulong a, uchar v)
{
	*(uchar*)a = v;
}
void
outs(ulong a, ushort v)
{
	*(ushort*)a = v;
}
void
outss(ulong a, void *p, int n)
{
	ushort *sp = p;

	while(n-- > 0)
		*(ushort*)a = *sp++;
}
void
outl(ulong a, ulong v)
{
	*(ulong*)a = v;
}
uchar
inb(ulong a)
{
	return *(uchar*)a;
}
ushort
ins(ulong a)
{
	return *(ushort*)a;
}
void
inss(ulong a, void *p, int n)
{
	ushort *sp = p;

	while(n-- > 0)
		*sp++ = *(ushort*)a;
}
ulong
inl(ulong a)
{
	return *(ulong*)a;
}

char*
getconf(char*)
{
	return nil;
}

long
_xdec(long *p)
{
	int s;
	long v;

	s = splhi();
	v = --*p;
	splx(s);
	return v;
}

void
_xinc(long *p)
{
	int s;

	s = splhi();
	++*p;
	splx(s);
}

int
cmpswap(long *addr, long old, long new)
{
	int r, s;
	
	s = splhi();
	if(r = (*addr==old))
		*addr = new;
	splx(s);
	return r;
}

