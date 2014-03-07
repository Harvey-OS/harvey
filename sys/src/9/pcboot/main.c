/*
 * 9boot - load next 386 or amd64 kernel from disk and start it
 *	and
 * 9load - load next 386 or amd64 kernel via pxe (bootp, tftp) and start it
 *
 * intel says that pxe can only load into the bottom 640K, and
 * intel's pxe boot agent takes 128K, leaving only 512K for 9boot.
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"pool.h"
#include	"reboot.h"
#include	"ip.h"		/* for eipfmt */
#include	<tos.h>

enum {
	Datamagic = 0xbabeabed,
};

Mach *m;

ulong* mach0pdb;
Mach* mach0m;
Segdesc* mach0gdt;
u32int memstart;
u32int memend;
int noclock;

extern int pcivga;
extern char hellomsg[];

/*
 * Where configuration info is left for the loaded programme.
 */
char bootdisk[KNAMELEN];
Conf conf;

uchar *sp;	/* user stack of init proc */
int delaylink;
int debug;
int v_flag;

static void
sanity(void)
{
	uintptr cr3;

	cr3 = (uintptr)KADDR(getcr3());
	if (cr3 == 0)
		panic("zero cr3");
	if ((uintptr)m->pdb != cr3 || (uintptr)mach0pdb != cr3)
		panic("not all same: cr3 %#p m->pdb %#p mach0pdb %#p",
			cr3, m->pdb, mach0pdb);
	if (m != mach0m)
		panic("m %#p != mach0m %#p", m, mach0m);
	if (m->gdt != mach0gdt)
		panic("m->gdt %#p != mach0gdt %#p", m->gdt, mach0gdt);
	if (0)
		iprint("m->pdb %#p m %#p sp %#p m->gdt %#p\n",
			m->pdb, m, &cr3, m->gdt);
}

enum {
	/* system control port a */
	Sysctla=	0x92,
	 Sysctlreset=	1<<0,
	 Sysctla20ena=	1<<1,
};

static int
isa20on(void)
{
	int r;
	ulong o;
	ulong *zp, *mb1p;

	zp = 0;
	mb1p = (ulong *)MB;
	o = *zp;

	*zp = 0x1234;
	*mb1p = 0x8765;
	mb586();
	wbinvd();
	r = *zp != *mb1p;

	*zp = o;
	return r;
}

void
a20init(void)
{
	int b;

	if (isa20on())
		return;

	i8042a20();			/* original method, via kbd ctlr */
	if (isa20on())
		return;

	/* newer method, last resort */
	b = inb(Sysctla);
	if (!(b & Sysctla20ena))
		outb(Sysctla, (b & ~Sysctlreset) | Sysctla20ena);
	if (!isa20on()){
		iprint("a20 didn't come on!\n");
		for(;;)
			;
	}
}

void
main(void)
{
	Proc *savup;
	static ulong vfy = Datamagic;
	static char novga[] = "\nno vga; serial console only\n";

	savup = up;
	up = nil;
	/* m has been set by l32v.s */

	/*
	 * disable address wraps at 1MB boundaries.
	 * if we're 9boot, ldecomp.s already did this.
	 */
	a20init();

	mach0init();
//	options();		/* we don't get options passed to us */
	ioinit();
	/* we later call i8250console after plan9.ini has been read */
	i8250config("0");	/* configure serial port 0 with defaults */
	quotefmtinstall();
 	fmtinstall('i', eipfmt);
 	fmtinstall('I', eipfmt);
 	fmtinstall('E', eipfmt);
 	fmtinstall('V', eipfmt);
 	fmtinstall('M', eipfmt);
	screeninit();			/* cga setup */
	cgapost(0xc);

	trapinit0();
	mmuinit0();

	kbdinit();
	i8253init();
	cpuidentify();
	readlsconf();
	meminit();
	confinit();
	archinit();
	xinit();
	if(i8237alloc != nil)
		i8237alloc();		/* dma (for floppy) init */
	trapinit();
	printinit();
	sanity();
	cgapost(1);

	/*
	 * soekris servers have no built-in video but each has a serial port.
	 * they must see serial output, if any, before cga output because
	 * otherwise the soekris bios will translate cga output to serial
	 * output, which will garble serial console output.
	 */
	pcimatch(nil, 0, 0);		/* force scan of pci table */
	if (!pcivga) {
		screenputs = nil;
		uartputs(novga, sizeof novga - 1);
	}
	print(" %s\n\n", hellomsg);

	if (vfy != Datamagic)
		panic("data segment incorrectly aligned or loaded");
	if (savup)
		print("up was non-nil (%#p) upon entry to main; bss wasn't zeroed!\n",
			savup);

//	xsummary();
	cpuidprint();
	mmuinit();
	if(arch->intrinit)	/* launches other processors on an mp */
		arch->intrinit();
	timersinit();
	mathinit();
	kbdenable();
	/*
	 * 9loadusb runs much faster if we don't use the clock.
	 * perhaps we're competing with the bios for the use of it?
	 */
	if(!noclock && arch->clockenable)
		arch->clockenable();
	procinit0();
	initseg();
	if(delaylink){
		bootlinks();
		pcimatch(0, 0, 0);
	}else
		links();
	conf.monitor = 1;
	cgapost(0xcd);
	chandevreset();
	cgapost(2);
	pageinit();	/* must follow xinit, and conf.mem must be populated */
	i8253link();
	userinit();

	active.thunderbirdsarego = 1;
	cgapost(0xb0);
	schedinit();
}

void
mach0init(void)
{
	conf.nmach = 1;
	MACHP(0) = mach0m;
	m->machno = 0;
	m->pdb = mach0pdb;
	m->gdt = mach0gdt;

	machinit();

	active.machs = 1;
	active.exiting = 0;
}

void
machinit(void)
{
	int machno;
	ulong *pdb;
	Segdesc *gdt;

	machno = m->machno;
	pdb = m->pdb;
	gdt = m->gdt;
	memset(m, 0, sizeof(Mach));
	m->machno = machno;
	m->pdb = pdb;
	m->gdt = gdt;
	m->perf.period = 1;

	/*
	 * For polled uart output at boot, need
	 * a default delay constant. 100000 should
	 * be enough for a while. Cpuidentify will
	 * calculate the real value later.
	 */
	m->loopconst = 100000;
}

void
init0(void)
{
	int i;
	char buf[2*KNAMELEN];

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

	if(0 && !waserror()){			/* not needed by boot */
		snprint(buf, sizeof(buf), "%s %s", arch->id, conffile);
		ksetenv("terminal", buf, 0);
		ksetenv("cputype", "386", 0);
		if(cpuserver)
			ksetenv("service", "cpu", 0);
		else
			ksetenv("service", "terminal", 0);
		for(i = 0; i < nconf; i++){
			if(confname[i][0] != '*')
				ksetenv(confname[i], confval[i], 0);
			ksetenv(confname[i], confval[i], 1);
		}
		poperror();
	}
	kproc("alarm", alarmkproc, 0);

	conschan = enamecopen("#c/cons", ORDWR);
	bootloadproc(0);
	panic("bootloadproc returned");
}

void
userinit(void)
{
	Proc *p;

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
	fpoff();

	/*
	 * Kernel Stack
	 *
	 * N.B. make sure there's enough space for syscall to check
	 *	for valid args and 
	 *	4 bytes for gotolabel's return PC
	 */
	p->sched.pc = (ulong)init0;
	p->sched.sp = (ulong)p->kstack+KSTACK-(sizeof(Sargs)+BY2WD);

	/* NB: no user stack nor text segments are set up */

	ready(p);
}

void
confinit(void)
{
	int i, userpcnt;
	ulong kpages;

	userpcnt = 0;			/* bootstrap; no user mode  */
	conf.npage = 0;
	for(i=0; i<nelem(conf.mem); i++)
		conf.npage += conf.mem[i].npage;

	conf.npage = MemMax / BY2PG;
	conf.nproc = 20;		/* need a few kprocs */
	if(cpuserver)
		conf.nproc *= 3;
	if(conf.nproc > 2000)
		conf.nproc = 2000;
	conf.nimage = 40;
	conf.nswap = conf.nproc*80;
	conf.nswppo = 4096;

	kpages = conf.npage - (conf.npage*userpcnt)/100;

	/*
	 * can't go past the end of virtual memory
	 * (ulong)-KZERO is 2^32 - KZERO
	 */
	if(kpages > ((ulong)-KZERO)/BY2PG)
		kpages = ((ulong)-KZERO)/BY2PG;

	conf.upages = conf.npage - kpages;
	conf.ialloc = (kpages/2)*BY2PG;

	/*
	 * Guess how much is taken by the large permanent
	 * datastructures. Mntcache and Mntrpc are not accounted for
	 * (probably ~300KB).
	 */
	kpages *= BY2PG;
	kpages -= conf.upages*sizeof(Page)
		+ conf.nproc*sizeof(Proc)
		+ conf.nimage*sizeof(Image)
		+ conf.nswap
		+ conf.nswppo*sizeof(Page);
	mainmem->maxsize = kpages;
	if(!cpuserver){
		/*
		 * give terminals lots of image memory, too; the dynamic
		 * allocation will balance the load properly, hopefully.
		 * be careful with 32-bit overflow.
		 */
		imagmem->maxsize = kpages;
	}
}

/*
 *  math coprocessor segment overrun
 */
static void
mathover(Ureg*, void*)
{
	pexit("math overrun", 0);
}

void
mathinit(void)
{
}

/*
 *  set up floating point for a new process
 */
void
procsetup(Proc*p)
{
	p->fpstate = FPinit;
	fpoff();
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

	/*
	 * While this processor is in the scheduler, the process could run
	 * on another processor and exit, returning the page tables to
	 * the free list where they could be reallocated and overwritten.
	 * When this processor eventually has to get an entry from the
	 * trashed page tables it will crash.
	 *
	 * If there's only one processor, this can't happen.
	 * You might think it would be a win not to do this in that case,
	 * especially on VMware, but it turns out not to matter.
	 */
	mmuflushtlb(PADDR(m->pdb));
}

static void
shutdown(int ispanic)
{
	int ms, once;

	lock(&active);
	if(ispanic)
		active.ispanic = ispanic;
	else if(m->machno == 0 && (active.machs & (1<<m->machno)) == 0)
		active.ispanic = 0;
	once = active.machs & (1<<m->machno);
	/*
	 * setting exiting will make hzclock() on each processor call exit(0),
	 * which calls shutdown(0) and arch->reset(), which on mp systems is
	 * mpshutdown, which idles non-bootstrap cpus and returns on bootstrap
	 * processors (to permit a reboot).  clearing our bit in machs avoids
	 * calling exit(0) from hzclock() on this processor.
	 */
	active.machs &= ~(1<<m->machno);
	active.exiting = 1;
	unlock(&active);

	if(once)
		iprint("cpu%d: exiting\n", m->machno);

	/* wait for any other processors to shutdown */
	spllo();
	for(ms = 5*1000; ms > 0; ms -= TK2MS(2)){
		delay(TK2MS(2));
		if(active.machs == 0 && consactive() == 0)
			break;
	}

	if(active.ispanic){
		if(!cpuserver)
			for(;;)
				halt();
		if(getconf("*debug"))
			delay(5*60*1000);
		else
			delay(10000);
	}else
		delay(1000);
}

void
reboot(void *entry, void *code, ulong size)
{
	int i;
	void (*f)(ulong, ulong, ulong);
	ulong *pdb;

	/* we do pass options to the kernel we loaded, however, at CONFADDR. */
	// writeconf();

	/*
	 * the boot processor is cpu0.  execute this function on it
	 * so that the new kernel has the same cpu0.  this only matters
	 * because the hardware has a notion of which processor was the
	 * boot processor and we look at it at start up.
	 */
	if (m->machno != 0) {
		procwired(up, 0);
		sched();
	}

	if(conf.nmach > 1) {
		/*
		 * the other cpus could be holding locks that will never get
		 * released (e.g., in the print path) if we put them into
		 * reset now, so force them to shutdown gracefully first.
		 */
		lock(&active);
		active.rebooting = 1;
		unlock(&active);
		shutdown(0);
		if(arch->resetothers)
			arch->resetothers();
		delay(20);
	}

	/*
	 * should be the only processor running now
	 */
	active.machs = 0;
	if (m->machno != 0)
		print("on cpu%d (not 0)!\n", m->machno);

	print("shutting down...\n");
	delay(200);

	splhi();

	/* turn off buffered serial console */
	serialoq = nil;

	/* shutdown devices */
	chandevshutdown();
	arch->introff();

	/*
	 * Modify the machine page table to directly map low memory
	 * This allows the reboot code to turn off the page mapping
	 */
	pdb = m->pdb;
	for (i = 0; i < LOWPTEPAGES; i++)
		pdb[PDX(i*4*MB)] = pdb[PDX(KZERO + i*4*MB)];
	mmuflushtlb(PADDR(pdb));

	/* setup reboot trampoline function */
	f = (void*)REBOOTADDR;
	memmove(f, rebootcode, sizeof(rebootcode));

	print("rebooting...\n");

	/* off we go - never to return */
	coherence();
	(*f)(PADDR(entry), PADDR(code), size);
}


void
exit(int ispanic)
{
	shutdown(ispanic);
	spllo();
	arch->reset();
}

int
isaconfig(char *class, int ctlrno, ISAConf *isa)
{
	char cc[32], *p;
	int i;

	snprint(cc, sizeof cc, "%s%d", class, ctlrno);
	p = getconf(cc);
	if(p == nil)
		return 0;

	isa->type = "";
	isa->nopt = tokenize(p, isa->opt, NISAOPT);
	for(i = 0; i < isa->nopt; i++){
		p = isa->opt[i];
		if(cistrncmp(p, "type=", 5) == 0)
			isa->type = p + 5;
		else if(cistrncmp(p, "port=", 5) == 0)
			isa->port = strtoul(p+5, &p, 0);
		else if(cistrncmp(p, "irq=", 4) == 0)
			isa->irq = strtoul(p+4, &p, 0);
		else if(cistrncmp(p, "dma=", 4) == 0)
			isa->dma = strtoul(p+4, &p, 0);
		else if(cistrncmp(p, "mem=", 4) == 0)
			isa->mem = strtoul(p+4, &p, 0);
		else if(cistrncmp(p, "size=", 5) == 0)
			isa->size = strtoul(p+5, &p, 0);
		else if(cistrncmp(p, "freq=", 5) == 0)
			isa->freq = strtoul(p+5, &p, 0);
	}
	return 1;
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

int
cistrncmp(char *a, char *b, int n)
{
	unsigned ac, bc;

	while(n > 0){
		ac = *a++;
		bc = *b++;
		n--;

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

int less_power_slower;

/*
 *  put the processor in the halt state if we've no processes to run.
 *  an interrupt will get us going again.
 */
void
idlehands(void)
{
	/*
	 * we used to halt only on single-core setups. halting in an smp system 
	 * can result in a startup latency for processes that become ready.
	 * if less_power_slower is true, we care more about saving energy
	 * than reducing this latency.
	 */
	if(conf.nmach == 1 || less_power_slower)
		halt();
}

void
trimnl(char *s)
{
	char *nl;

	nl = strchr(s, '\n');
	if (nl != nil)
		*nl = '\0';
}
