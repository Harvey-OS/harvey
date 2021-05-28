#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../ip/ip.h"
#include <pool.h>
#include <tos.h>
#include <bootexec.h>
#include "init.h"
#include "reboot.h"

enum {
	/* space for syscall args, return PC, top-of-stack struct */
	Stkheadroom	= sizeof(Sargs) + sizeof(uintptr) + sizeof(Tos),
};

typedef struct mipsexec Mipsexec;

#define	MAXCONF		64
#define CONFBUFLEN	4096
#define MAXBOOTENV	256

/* only for reboot */
#define BOOTARGS	((char*)(CONFADDR))
#define BOOTARGSLEN	CONFBUFLEN
#define CONFARGVLEN	(MAXCONF*BY2WD)

/* args passed by boot loader for main() */
int		_argc;
char	**_argv;
char	**_env;
static char *bootenvname[MAXBOOTENV];
static char *bootenvval[MAXBOOTENV];
static int 	nenv;

/* plan9.ini */
static char confbuf[CONFBUFLEN];
static char *confname[MAXCONF];
static char *confval[MAXCONF];
static int	nconf;

/*
 * Option arguments from the command line.
 * oargv[0] is the boot file.
 */
static int oargc;
static char* oargv[20];
static char oargb[128];
static int oargblen;

static uintptr sp;		/* XXX - must go - user stack of init proc */

/*
 * software tlb simulation
 */
static Softtlb stlb[MAXMACH][STLBSIZE];

Mach	*machaddr[MAXMACH];
Conf	conf;
ulong	memsize;
FPsave	initfp;

int normalprint;
ulong cpufreq;

/*
 *  initialize a processor's mach structure.  each processor does this
 *  for itself.
 */
void
machinit(void)
{
	/* Ensure CU1 is off */
	clrfpintr();

	m->stb = &stlb[m->machno][0];

	clockinit();
}

static void
optionsinit(char* s)
{
	strecpy(oargb, oargb+sizeof(oargb), s);

	oargblen = strlen(oargb);
	oargc = tokenize(oargb, oargv, nelem(oargv)-1);
	oargv[oargc] = nil;
}

void
plan9iniinit(void)
{
	char *cp;
	int i;

	nconf = 0;
	if(_argv != nil){
		memmove(confbuf, _argv[0], sizeof(confbuf));
		for(i=1; i<_argc && nconf<MAXCONF; i++){
			_argv[i] += confbuf - _argv[0];
			if(*_argv[i] == '#')
				continue;
			cp = strchr(_argv[i], '=');
			if(cp == nil)
				continue;
			*cp++ = '\0';
			confname[nconf] = _argv[i];
			confval[nconf] = cp;
			nconf++;
		}
	}

	nenv = 0;
	if(_env != nil)
		for(i=0; _env[i] != nil && nenv<MAXBOOTENV; i++){
			if(*_env[i] == '#')
				continue;
			cp = strchr(_env[i], '=');
			if(cp == nil)
				continue;
			*cp++ = '\0';
			bootenvname[nenv] = _env[i];
			bootenvval[nenv] = cp;
			nenv++;
		}
}

static int
findconf(char *name)
{
	int i;

	for(i = 0; i < nconf; i++)
		if(cistrcmp(confname[i], name) == 0)
			return i;
	return -1;
}

char*
getconf(char *name)
{
	int i;

	i = findconf(name);
	if(i >= 0)
		return confval[i];
	return nil;
}

static int
findbootenv(char *name)
{
	int i;

	for(i = 0; i < nenv; i++)
		if(cistrcmp(bootenvname[i], name) == 0)
			return i;
	return -1;
}

static char*
getbootenv(char *name)
{
	int i;

	i = findbootenv(name);
	if(i >= 0)
		return bootenvval[i];
	return nil;
}

void
confinit(void)
{
	char *p;
	int i;
	ulong kpages, ktop;
	ulong maxmem, highmemsize;

	/* memory size */
	memsize = MEMSIZE;
	if((p = getbootenv("memsize")) != nil)
		memsize = strtoul(p, 0, 0) * MB;
	highmemsize = 0;
	if((p = getbootenv("highmemsize")) != nil)
		highmemsize = strtoul(p, 0, 0) * MB;
	if((p = getconf("*maxmem")) != nil){
		maxmem = strtoul(p, 0, 0);
		memsize = MIN(memsize, maxmem);
		maxmem -= memsize;
		highmemsize = MIN(highmemsize, maxmem);
		if(memsize < 16*MB)		/* sanity */
			memsize = 16*MB;
	}

	/* set up other configuration parameters */
	conf.nproc = 2000;
	conf.nswap = 262144;
	conf.nswppo = 4096;
	conf.nimage = 200;

	/*
	 *  divide memory to user pages and kernel.
	 */
	conf.mem[0].base = ktop = PADDR(PGROUND((ulong)end));
	conf.mem[0].npage = memsize/BY2PG - ktop/BY2PG;

	conf.mem[1].base = HIGHMEM;
	conf.mem[1].npage = highmemsize/BY2PG;

	conf.npage = 0;
	for(i=0; i<nelem(conf.mem); i++)
		conf.npage += conf.mem[i].npage;

	kpages = conf.npage - (conf.npage*80)/100;
	if(kpages > (64*MB + conf.npage*sizeof(Page))/BY2PG){
		kpages = (64*MB + conf.npage*sizeof(Page))/BY2PG;
		kpages += (conf.nproc*KSTACK)/BY2PG;
	}
	conf.upages = conf.npage - kpages;
	conf.ialloc = (kpages/2)*BY2PG;

	kpages *= BY2PG;
	kpages -= conf.upages*sizeof(Page)
		+ conf.nproc*sizeof(Proc)
		+ conf.nimage*sizeof(Image)
		+ conf.nswap
		+ conf.nswppo*sizeof(Page);
	mainmem->maxsize = kpages;

	/*
	 *  set up CPU's mach structure
	 *  cpu0's was zeroed in main() and our stack is in Mach, so don't zero it.
	 */
	m->machno = 0;
	m->hz = cpufreq;		/* initial guess at MHz */
	m->speed = m->hz / Mhz;
	conf.nmach = 1;

	conf.nuart = 1;
	conf.copymode = 0;		/* copy on write */
}

void
procrestore(Proc *p)
{
	uvlong t;

	if(p->kp)
		return;
	cycles(&t);
	p->pcycles -= t;
}

/*
 *  Save the mach dependent part of the process state.
 */
void
procsave(Proc *p)
{
	uvlong t;

	cycles(&t);
	p->pcycles += t;
	//fpuprocsave(); // XXX
}

static void
fmtinit(void)
{
	printinit();
	quotefmtinstall();
	/* ipreset installs these when chandevreset runs */
	fmtinstall('i', eipfmt);
	fmtinstall('I', eipfmt);
	fmtinstall('E', eipfmt);
	fmtinstall('V', eipfmt);
	fmtinstall('M', eipfmt);
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
	/* 0x200 not used in looongson 2e */
	icflush((ulong*)UTLBMISS, 4*1024);

	setstatus(getstatus() & ~BEV);
}

static void
prcpuid(void)
{
	ulong cpuid, cfg;
	char *cpu;

	cpuid = prid();
	cfg = getconfig();
	if (((cpuid>>16) & MASK(8)) == 0)		/* vendor */
		cpu = "old mips";
	else if (((cpuid>>16) & MASK(8)) == 1)
		switch ((cpuid>>8) & MASK(8)) {		/* processor */
		case 0x93:
			cpu = "mips 24k";
			break;
		case 0x96:
			cpu = "mips 24ke";
			break;
		default:
			cpu = "mips";
			break;
		}
	else
		cpu = "other mips";
	delay(200);
	print("cpu%d: %ldMHz %s %se 0x%ulx v%ld.%ld rev %ld has fpu\n",
		m->machno, m->hz / Mhz, cpu, cfg & (1<<15)? "b": "l",
		(cpuid>>8) & MASK(8), (cpuid>>5) & MASK(3), (cpuid>>2) & MASK(3),
		cpuid & MASK(2));
	delay(200);

	print("cpu%d: %d tlb entries, using %dK pages\n", m->machno,
		64, BY2PG/1024);
	delay(50);
	print("cpu%d: l1 i cache: %d sets 4 ways 32 bytes/line\n", m->machno,
		32 << ((cfg>>9) & MASK(3)));
	delay(50);
	print("cpu%d: l1 d cache: %d sets 4 ways 32 bytes/line\n", m->machno,
		32 << ((cfg>>6) & MASK(3)));
	delay(500);
}

static int
ckpagemask(ulong mask, ulong size)
{
	int s;
	ulong pm;

	s = splhi();
	setpagemask(mask);
	pm = getpagemask();
	splx(s);
	if(pm != mask){
		iprint("page size %ldK not supported on this cpu; "
			"mask %#lux read back as %#lux\n", size/1024, mask, pm);
		return -1;
	}
	return 0;
}

void
init0(void)
{
	char buf[128];
	int i;

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
		ksetenv("cputype", "spim", 0);
		snprint(buf, sizeof buf, "spim %s loongson 2e", conffile);
		ksetenv("terminal", buf, 0);
		if(cpuserver)
			ksetenv("service", "cpu", 0);
		else
			ksetenv("service", "terminal", 0);
//		ksetenv("nobootprompt", "local!/boot/bzroot", 0); // for bzroot
		ksetenv("nvram", "/boot/nvram", 0);
		/* convert plan9.ini variables to #e and #ec */
		for(i = 0; i < nconf; i++) {
			if(*confname[i] == '*')
				continue;
			ksetenv(confname[i], confval[i], 0);
			ksetenv(confname[i], confval[i], 1);
		}
		poperror();
	}
	kproc("alarm", alarmkproc, 0);
	i8250console();
	touser(sp);
}

static void
bootargs(uintptr base)
{
	int i;
	ulong ssize;
	char **av, *p;

	/*
	 * Push the boot args onto the stack.
	 * The initial value of the user stack must be such
	 * that the total used is larger than the maximum size
	 * of the argument list checked in syscall.
	 */
	i = oargblen+1;
	p = UINT2PTR(STACKALIGN(base + BY2PG - Stkheadroom - i));
	memmove(p, oargb, i);

	/*
	 * Now push the argv pointers.
	 * The code jumped to by touser in l.s expects arguments
	 *	main(char* argv0, ...)
	 * and calls
	 * 	startboot("/boot/boot", &argv0)
	 * not the usual (int argc, char* argv[])
	 */
	av = (char**)(p - (oargc+1)*sizeof(char*));
	ssize = base + BY2PG - PTR2UINT(av);
	for(i = 0; i < oargc; i++)
		*av++ = (oargv[i] - oargb) + (p - base) + (USTKTOP - BY2PG);
	*av = nil;
	sp = USTKTOP - ssize;
}

void
userinit(void)
{
	Proc *p;
	KMap *k;
	Page *pg;
	Segment *s;

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

	p->fpstate = FPinit;
	p->fpsave.fpstatus = initfp.fpstatus;

	/*
	 * Kernel Stack
	 */
	p->sched.pc = (ulong)init0;
	p->sched.sp = (ulong)p->kstack+KSTACK-Stkheadroom;
	p->sched.sp = STACKALIGN(p->sched.sp);

	/*
	 * User Stack
	 *
	 * Technically, newpage can't be called here because it
	 * should only be called when in a user context as it may
	 * try to sleep if there are no pages available, but that
	 * shouldn't be the case here.
	 */
	s = newseg(SG_STACK, USTKTOP-USTKSIZE, USTKSIZE/BY2PG);
	p->seg[SSEG] = s;
	pg = newpage(1, 0, USTKTOP-BY2PG);
	memset(pg->cachectl, PG_DATFLUSH, sizeof(pg->cachectl));
	segpage(s, pg);
	k = kmap(pg);
	bootargs(VA(k));
	kunmap(k);

	/*
	 * Text
	 */
	s = newseg(SG_TEXT, UTZERO, 1);
	s->flushme++;
	p->seg[TSEG] = s;
	pg = newpage(1, 0, UTZERO);
	memset(pg->cachectl, PG_TXTFLUSH, sizeof(pg->cachectl));
	segpage(s, pg);
	k = kmap(s->map[0]->pages[0]);
	memset((void*)VA(k), 0, BY2PG);
	memmove((ulong*)VA(k), initcode, sizeof initcode);
	kunmap(k);

	ready(p);
}

/* called from rebootcmd() */
int
parsemipsboothdr(Chan *c, ulong magic, Execvals *evp)
{
	long extra;
	Mipsexec me;

	/*
	 * BOOT_MAGIC is sometimes defined like this:
	 * #define BOOT_MAGIC	(0x160<<16) || magic == ((0x160<<16)|3)
	 * so we can only use it in a fairly stylized manner.
	 */
	if(magic == BOOT_MAGIC) {
		c->offset = 0;			/* back up */
		readn(c, &me, sizeof me);
		/* if binary is -H1, read an extra long */
		if (l2be(me.amagic) == 0407 && me.nscns == 0)
			readn(c, &extra, sizeof extra);
		evp->entry = l2be(me.mentry);
		evp->textsize = l2be(me.tsize);
		evp->datasize = l2be(me.dsize);
		return 0;
	} else
		return -1;
}

void
main(void)
{
	extern char edata[], end[];

	m = (Mach*)MACHADDR;
	memset(m, 0, sizeof(Mach));		/* clear Mach */
	m->machno = 0;
	machaddr[m->machno] = m;
	up = nil;
	memset(edata, 0, end-edata);	/* clear bss */

	optionsinit("/boot/boot boot");
	plan9iniinit();
	confinit();
	savefpregs(&initfp);

	machinit();
	active.exiting = 0;
	active.machs = 1;

	kmapinit();
	xinit();
	screeninit();

	ckpagemask(PGSZ, BY2PG);
	tlbinit();
	machwire();

	timersinit();
	fmtinit();
	vecinit();

	normalprint = 1;
	print("\nPlan 9\n");
	print("\0\0\0\0\0\0\0"); // XXX 0c bug fixed -- leave this for sanity
	prcpuid();
	if(PTECACHABILITY == PTENONCOHERWT)
		print("caches configured as write-through\n");
//	xsummary();

	i8259init();
	kbdinit();
	if(conf.monitor)
		swcursorinit();

	pageinit();
	procinit0();
	initseg();
	links();
	chandevreset();

	swapinit();
	userinit();
	parseboothdr = parsemipsboothdr;
	schedinit();
	panic("schedinit returned");
}

static void
writeconf(void)
{
	char *p, *q;
	int n;

	p = getconfenv();

	if(waserror()) {
		free(p);
		nexterror();
	}

	/* convert to name=value\0 format */
	_argc = 1;		/* skip _argv[0] */
	_argv = (void*)CONFARGV;
	_argv[0] = BOOTARGS;
	for(q=p; *q; q++) {
		_argv[_argc] = (char*)((ulong)BOOTARGS + (q - p));
		_argc++;
		q += strlen(q);
		*q = '=';
		q += strlen(q);
	}
	n = q - p + 1;
	_env = &_argv[_argc];

	if(n >= BOOTARGSLEN || (ulong)_env >= CONFARGV + CONFARGVLEN)
		error("kernel configuration too large");
	memmove(BOOTARGS, p, n);
	memset(BOOTARGS + n, '\0', BOOTARGSLEN - n);
	memset(_env, 0, CONFARGV + CONFARGVLEN - (ulong)_env);

	poperror();
	free(p);
}

static void
shutdown(int ispanic)
{
	int ms, once;

	ilock(&active);
	if(ispanic)
		active.ispanic = ispanic;
	else if(m->machno == 0 && (active.machs & (1<<m->machno)) == 0)
		active.ispanic = 0;
	once = active.machs & (1<<m->machno);
	/*
	 * setting exiting will make hzclock() on each processor call exit(0),
	 * which calls shutdown(0) and idles non-bootstrap cpus and returns
	 * on bootstrap processors (to permit a reboot).  clearing our bit
	 * in machs avoids calling exit(0) from hzclock() on this processor.
	 */
	active.machs &= ~(1<<m->machno);
	active.exiting = 1;
	iunlock(&active);

	if(once) {
		delay(m->machno*1000);		/* stagger them */
		iprint("cpu%d: exiting\n", m->machno);
	}
	spllo();
	for(ms = MAXMACH * 1000; ms > 0; ms -= TK2MS(2)){
		delay(TK2MS(2));
		if(active.machs == 0 && consactive() == 0)
			break;
	}
	delay(100);
}

void
exit(int ispanic)
{
	int timer;

	delay(1000);
	shutdown(ispanic);
	timer = 0;
	while(active.machs || consactive()) {
		if(timer++ > 400)
			break;
		delay(10);
	}
	delay(1000);
	splhi();

	setstatus(BEV);
	coherence();

	iprint("exit: awaiting reset\n");
	delay(1000);			/* await a reset */

	if(!active.ispanic) {
		iprint("exit: jumping to rom\n");
		*Reset |= Rstcpucold;		/* cpu cold reset */
	}

	iprint("exit: looping\n");
	for(;;);
}

/*
 * the new kernel is already loaded at address `code'
 * of size `size' and entry point `entry'.
 */
void
reboot(void *entry, void *code, ulong size)
{
	void (*f)(ulong, ulong, ulong, ulong);

	writeconf();

	/*
	 * the boot processor is cpu0.  execute this function on it
	 * so that the new kernel has the same cpu0.
	 */
	if(m->machno != 0) {
		procwired(up, 0);
		sched();
	}
	if(m->machno != 0)
		print("on cpu%d (not 0)!\n", m->machno);

	if(code == nil || entry == nil)
		exit(1);

	shutdown(0);

	/*
	 * should be the only processor running now
	 */
	iprint("reboot: entry %#p code %#p size %ld\n", entry, code, size);
	iprint("code[0] = %#lux\n", *(ulong *)code);

	/* turn off buffered serial console */
	serialoq = nil;
	kprintoq = nil;
	screenputs = nil;

	/* shutdown devices */
	chandevshutdown();

	splhi();
	intrshutdown();

	/* setup reboot trampoline function */
	f = (void*)REBOOTADDR;
	memmove(f, rebootcode, sizeof(rebootcode));
	icflush(f, sizeof(rebootcode));

	setstatus(BEV);		/* also, kernel mode, no interrupts */
	coherence();

	/* off we go - never to return */
	(*f)((ulong)entry, (ulong)code, size, _argc);

	panic("loaded kernel returned!");
}

/*
 * stub for ../port/devpnp.c
 */
int
isaconfig(char *class, int ctlrno, ISAConf *isa)
{
	USED(class, ctlrno, isa);
	return 0;
}
