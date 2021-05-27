#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"init.h"
#include	"pool.h"
#include	"../ip/ip.h"
#include	<tos.h>
#include	<bootexec.h>
#include	"reboot.h"

enum {
	/* space for syscall args, return PC, top-of-stack struct */
	Stkheadroom	= sizeof(Sargs) + sizeof(uintptr) + sizeof(Tos),
};

typedef struct mipsexec Mipsexec;

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

Conf	conf;
FPsave	initfp;
Rbconf	rbconf;

int normalprint;
ulong memsize;

char *
getconf(char *)
{
	return nil;	/* stub */
}

static void
optionsinit(char* s)
{
	strecpy(oargb, oargb+sizeof(oargb), s);

	oargblen = strlen(oargb);
	oargc = tokenize(oargb, oargv, nelem(oargv)-1);
	oargv[oargc] = nil;
}

static void
prcpuid(void)
{
	ulong cpuid, cfg1;
	char *cpu;

	cpuid = prid();
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
	delay(20);
	print("cpu%d: %ldMHz %s %se v%ld.%ld rev %ld, ",
		m->machno, m->hz / Mhz, cpu, getconfig() & (1<<15)? "b": "l",
		(cpuid>>5) & MASK(3), (cpuid>>2) & MASK(3), cpuid & MASK(2));
	delay(200);
	cfg1 = getconfig1();
	print("%s fpu\n", (cfg1 & 1? "has": "no"));
	print("cpu%d: %ld tlb entries, using %dK pages\n", m->machno,
		((cfg1>>25) & MASK(6)) + 1, BY2PG/1024);
	delay(50);
	print("cpu%d: l1 i cache: %d sets 4 ways 32 bytes/line\n", m->machno,
		64 << ((cfg1>>22) & MASK(3)));
	delay(50);
	print("cpu%d: l1 d cache: %d sets 4 ways 32 bytes/line\n", m->machno,
		64 << ((cfg1>>13) & MASK(3)));
	delay(500);
	if (0)
		print("cpu%d: cycle counter res = %ld\n",
			m->machno, gethwreg3());
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

/*
 * parse args kmac=AA:BB:CC:DD:EE:FF for ether0 mac, mem=256M,
 * HZ=340000000, console=ttyS0,115200.
 * args seem to live above 8MB, where they are likely to be clobbered
 * by kernel allocations, so make copies for rebooting.
 *
 * at this early stage, we can't use the usual kernel memory allocators,
 * thus we can't use kstrdup.
 */
static void
saverbconfig(int argc, char **argv)
{
	int i;
	char *p;
	static char ether0mac[32], mem[16], hz[16], console[32];

	if (argv == nil)
		return;
	rbconf.ether0mac = ether0mac;
	rbconf.memsize = mem;
	rbconf.hz = hz;
	rbconf.console = console;
	for (i = 0; i < argc; i++) {
		p = argv[i];
		if (strncmp(p, "kmac=", 5) == 0)
			strncpy(rbconf.ether0mac, p, sizeof ether0mac);
		else if (strncmp(p, "mem=", 4) == 0)
			strncpy(rbconf.memsize, p, sizeof mem);
		else if (strncmp(p, "HZ=", 3) == 0)
			strncpy(rbconf.hz, p, sizeof hz);
		else if (strncmp(p, "console=", 8) == 0)
			strncpy(rbconf.console, p, sizeof console);
	}
	memsize = 256*MB;			/* routerboard default */
	p = strchr(rbconf.memsize, '=');
	if (p)
		memsize = atoi(p+1) * MB;
}

/* arguments come from routerboot; only argc and argv are non-zero. */
void
main(int argc, char **argv, char **envp, ulong memsz)
{
	stopwdog();			/* tranquilise the dog */
	optionsinit("/boot/boot boot");
	saverbconfig(argc, argv);
	USED(envp, memsz);

	confinit();
	savefpregs(&initfp);
	machinit();			/* calls clockinit */
	active.exiting = 0;
	cpuactive(0);

	kmapinit();
	xinit();

	timersinit();
	fmtinit();
	vecinit();

	normalprint = 1;
	print("\nPlan 9 from Bell Labs (mips)\n");
	prcpuid();
	if (PTECACHABILITY == PTENONCOHERWT)
		print("caches configured as write-through\n");
	if (0)
		xsummary();

	ckpagemask(PGSZ, BY2PG);
	tlbinit();
	machwire();
	pageinit();
	procinit0();
	initseg();
	links();
	chandevreset();

	swapinit();
	userinit();
	sicwdog();
	parseboothdr = parsemipsboothdr;
	schedinit();
	panic("schedinit returned");
}

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
	memmove((ulong*)(KSEG0+0x200), (ulong*)vector180, 0x80);
	icflush((ulong*)UTLBMISS, 4*1024);

	setstatus(getstatus() & ~BEV);
}

void
init0(void)
{
	char buf[128];

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
		ksetenv("cputype", "mips", 0);
		snprint(buf, sizeof buf, "mips %s", conffile);
		ksetenv("terminal", buf, 0);
		if(cpuserver)
			ksetenv("service", "cpu", 0);
		else
			ksetenv("service", "terminal", 0);
		/*
		 * we don't have a good way to read our cfg file in
		 * RouterBOOT, so set the configuration here.
		 */
		ksetenv("nobootprompt", "tcp", 0);
		ksetenv("nvram", "/boot/nvram", 0);
		poperror();
	}
	kproc("alarm", alarmkproc, 0);
	i8250console();
	touser(sp);
}

FPsave	initfp;

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
	p = (char *)STACKALIGN(base + BY2PG - Stkheadroom - i);
	memmove(p, oargb, i);

	/*
	 * Now push the argv pointers.
	 * The code jumped to by touser in lproc.s expects arguments
	 *	main(char* argv0, ...)
	 * and calls
	 * 	startboot("/boot/boot", &argv0)
	 * not the usual (int argc, char* argv[])
	 */
	av = (char**)(p - (oargc+1)*sizeof(char*));
	ssize = base + BY2PG - (uintptr)av;
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
	memset((void *)VA(k), 0, BY2PG);
	memmove((ulong*)VA(k), initcode, sizeof initcode);
	kunmap(k);

	ready(p);
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
	/* no fpu, so no fp state to save */
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

	/* convert to name=value\n format */
	for(q=p; *q; q++) {
		q += strlen(q);
		*q = '=';
		q += strlen(q);
		*q = '\n';
	}
	n = q - p + 1;
#ifdef BOOTARGS_EXIST
	if(n >= BOOTARGSLEN)
		error("kernel configuration too large");
	memmove(BOOTARGS, p, n);
	memset(BOOTARGS + n, '\n', BOOTARGSLEN - n);
#endif
	USED(n);
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
	else if(m->machno == 0 && !iscpuactive(m->machno))
		active.ispanic = 0;
	once = iscpuactive(m->machno);
	/*
	 * setting exiting will make hzclock() on each processor call exit(0),
	 * which calls shutdown(0) and idles non-bootstrap cpus and returns
	 * on bootstrap processors (to permit a reboot).  clearing our bit
	 * in machs avoids calling exit(0) from hzclock() on this processor.
	 */
	cpuinactive(m->machno);
	active.exiting = 1;
	iunlock(&active);

	if(once) {
		delay(m->machno*1000);		/* stagger them */
		iprint("cpu%d: exiting\n", m->machno);
	}
	spllo();
	ms = MAXMACH * 1000;
	for(; ms > 0; ms -= TK2MS(2)){
		delay(TK2MS(2));
		if(active.nmachs == 0 && consactive() == 0)
			break;
	}
	delay(100);
}

/*
 * the new kernel is already loaded at address `code'
 * of size `size' and physical entry point `entry'.
 */
void
reboot(void *entry, void *code, ulong size)
{
	Rbconf *rbc;
	void (*f)(void *, ulong, ulong, ulong);

	writeconf();
	/*
	 * copy rbconf and contents into allocated memory, thus safe from
	 * being overwritten by the new kernel in the reboot trampoline
	 * code below.
	 */
	rbc = smalloc(sizeof *rbc);
	kstrdup(&rbc->ether0mac, rbconf.ether0mac);
	kstrdup(&rbc->memsize, rbconf.memsize);
	kstrdup(&rbc->hz, rbconf.hz);
	kstrdup(&rbc->console, rbconf.console);

	/*
	 * the boot processor is cpu0.  execute this function on it
	 * so that the new kernel has the same cpu0.
	 */
	if (m->machno != 0) {
		procwired(up, 0);
		sched();
	}
	if (m->machno != 0)
		print("on cpu%d (not 0)!\n", m->machno);

	shutdown(0);

	/*
	 * should be the only processor running now
	 */
//	iprint("reboot: entry %#p code %#p size %ld\n", entry, code, size);
//	iprint("code[0] = %#lux\n", *(ulong *)code);

	/* turn off buffered serial console */
	serialoq = nil;
	kprintoq = nil;
	screenputs = nil;

	/* shutdown devices */
	chandevshutdown();

	/* call off the dog */
	clockshutdown();

	splhi();
	intrshutdown();

	/* is the watchdog tied into the usb machinery? */
//	*Reset |= Rstusbohcidll | Rstusbhost | Rstusbphy;
//		Rstge0mac | Rstge0phy |
//		Rstge1mac | Rstge1phy;

	/* setup reboot trampoline function */
	f = (void*)REBOOTADDR;
	memmove(f, rebootcode, sizeof(rebootcode));
	dcflush(f, sizeof(rebootcode));
	icflush(f, sizeof(rebootcode));

	setstatus(BEV);		/* also, kernel mode, no interrupts */
	coherence();

	/* off we go - never to return */
	if (((ulong)entry & KSEGM) == 0)	/* physical address? */
		entry = KADDR(entry);		/* make it kernel virtual */
	(*f)(rbc, (ulong)entry, (ulong)code, size);

	panic("loaded kernel returned!");
}

void
exit(int type)
{
	int timer;
	void (*fnp)(void);

	stopwdog();

	delay(1000);
	lock(&active);
	cpuinactive(m->machno);
	active.exiting = 1;
	unlock(&active);
	spllo();

	print("cpu %d exiting\n", m->machno);
	timer = 0;
	while(active.nmachs || consactive()) {
		if(timer++ > 400)
			break;
		delay(10);
	}
	delay(1000);
	splhi();
	USED(type);

	setstatus(BEV);
	coherence();

	iprint("exit: awaiting reset\n");
	wdogreset();			/* wake the dog with v. short timeout */

//	*Reset |= Rstfullchip;
//	*Reset |= Rstcpucold;

	delay(1000);			/* await a reset */

	iprint("exit: jumping to rom\n");
	fnp = (void (*)(void))ROM;
	(*fnp)();

	iprint("exit: looping\n");
	for (;;)
		;
}

void
idlehands(void)
{
	stopwdog();
	idle();
	sicwdog();			/* wake the dog */
}

void
confinit(void)
{
	char *p;
	ulong kpages, ktop;

	/*
	 *  divide memory twixt user pages and kernel.
	 */
	conf.mem[0].base = ktop = PADDR(PGROUND((ulong)end));
	assert(memsize > 16*MB);
	conf.mem[0].npage = memsize/BY2PG - ktop/BY2PG;
	conf.npage = conf.mem[0].npage;
	conf.nuart = 1;

	kpages = conf.npage - (conf.npage*80)/100;
	if(kpages > (64*MB + conf.npage*sizeof(Page))/BY2PG){
		kpages = (64*MB + conf.npage*sizeof(Page))/BY2PG;
		kpages += (conf.nproc*KSTACK)/BY2PG;
	}
	conf.upages = conf.npage - kpages;
	conf.ialloc = (kpages/2)*BY2PG;

	kpages *= BY2PG;
	kpages -= conf.upages*sizeof(Page)	/* palloc.pages in pageinit */
		+ conf.nproc*sizeof(Proc)  /* procalloc.free in procinit0 */
		+ conf.nimage*sizeof(Image)	/* imagealloc.free in initseg */
		+ conf.nswap		/* swapalloc.swmap in swapinit */
		+ conf.nswppo*sizeof(Page*);	/* iolist in swapinit */
	mainmem->maxsize = kpages;

	/*
	 *  set up CPU's mach structure
	 *  cpu0's was zeroed in l.s and our stack is in Mach, so don't zero it.
	 */
	m->machno = 0;
	m->speed = 680;			/* initial guess at MHz, for rb450g */
	m->hz = 680 * Mhz;
	p = strchr(rbconf.hz, '=');
	if (p) {
		m->hz = 2 * strtol(p+1, 0, 10);
		m->speed = m->hz / Mhz;
	}
	conf.nmach = 1;

	/* set up other configuration parameters */
	conf.nproc = 2000;
	conf.nswap = 262144;
	conf.nswppo = 4096;
	conf.nimage = 200;

	conf.copymode = 0;		/* copy on write */
}
