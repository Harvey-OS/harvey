/*
 * initialise an amd64 system, start all cpus, and
 * start scheduling processes on them, notably /boot/boot.
 * also, graceful shutdown and reboot code.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "io.h"
#include "amd64.h"

#include "init.h"
#include "reboot.h"
#include <a.out.h>
#include <ctype.h>

enum {
	Ps2 = 1,	/* flag: allow PS/2 keyboard (for VMs) */
	Useipi = 0,	/* flag: reboot using ipi (untested) */
};

extern void inioptions(void);
extern void confsetenv(void);

static uintptr sp;		/* user stack of init proc */

Sys* sys = nil;
usize sizeofSys = sizeof(Sys);
usize sizeofSyscomm = sizeof(Syscomm);

/*
 * Option arguments from the command line.
 * oargv[0] is the boot file.
 * Optionsinit() is called from multiboot() to
 * set it all up.
 */
static int oargc;
static char* oargv[20];
static char oargb[128];
static int oargblen;

char cant[] = "can't happen: notreached reached\n";
char dbgflg[256];
static int vflag = 0;

void
mboptinit(char* s)
{
	oargblen = strecpy(oargb, oargb+sizeof(oargb), s) - oargb;
	oargc = tokenize(oargb, oargv, nelem(oargv)-1);
	oargv[oargc] = nil;
}

static void
mbopts(int argc, char* argv[])
{
	char *p;
	int n, o;

	/*
	 * Process multiboot flags.
	 * Flags [A-Za-z] may be optionally followed by
	 * an integer level between 1 and 127 inclusive
	 * (no space between flag and level).
	 * '--' ends flag processing.
	 */
	while(--argc > 0 && (*++argv)[0] == '-' && (*argv)[1] != '-'){
		while(o = *++argv[0]){
			if (!isalpha(o))
				continue;
			n = strtol(argv[0]+1, &p, 0);
			if(p == argv[0]+1 || n < 1 || n > 127)
				n = 1;
			argv[0] = p-1;
			dbgflg[o] = n;
		}
	}
	vflag = dbgflg['v'];
}

static void
fakecpuhz(void)
{
	/*
	 * Need something for initial delays
	 * until a timebase is worked out.
	 */
	m->cpuhz = 2000000000ll;
	m->cpumhz = 2000;
	m->perf.period = 1;
}

static vlong
sethz(void)
{
	vlong hz;

	if((hz = archhz()) != 0){
		m->cpuhz = hz;
		m->cpumhz = hz/1000000;
	}
	return hz;
}

static void
setepochtsc(void)
{
	if (m->machno == 0)
		sys->epoch = rdtsc();
	wrmsr(Tscr, sys->epoch);
	m->rdtsc = rdtsc();
}

extern Sipi *sipis;

/* start a cpu other than 0, at apicno.  entered splhi, i believe. */
void
squidboy(int apicno)
{
	vlong hz;
	Sipi *sipi;

	up = nil;
	sipi = &sipis[apicno];
	m = sipi->mach;
	sys->machptr[m->machno] = m;
	fakecpuhz();

	DBG("Hello Squidboy %d %d\n", apicno, m->machno);
	vsvminit(MACHSTKSZ);

	/*
	 * Beware the Curse of The Non-Interruptable Were-Temporary.
	 */
	hz = sethz();
	if(hz == 0) {
		print("squidboy: cpu%d: 0Hz\n", m->machno);
		ndnr();
	}

	mmuinit();
	if(!apiconline()) {
		print("squidboy: cpu%d: apic %d off-line\n", m->machno, apicno);
		ndnr();
	}
	fpuinit();

	/*
	 * Handshake with sipi to let it know the Startup IPI succeeded.
	 */
	m->splpc = 0;
	coherence();

	/*
	 * Handshake with main to proceed with initialisation.
	 */
	while(sys->epoch == 0)
		pause();
	setepochtsc();

	DBG("mach %d is go %#p %#p %#p\n", m->machno, m, m->pml4->va, &apicno);
	/*
	 * we used to switch on m->mode here, but nothing in the current
	 * system sets it.
	 */
//	vsvminit(MACHSTKSZ);
	timersinit();

	/*
	 * Cannot allow interrupts while waiting for online,
	 * if this were a real O/S, it would perhaps allow a clock
	 * interrupt to call the scheduler, and that would be a mistake.
	 */
	while(!m->online)
		pause();
	apictimerenable();		/* start my clock */
	apictprput(0);

	DBG("mach%d: online color %d\n", m->machno, m->color);
	schedinit();			/* no return */
	panic("squidboy: schedinit returns on cpu%d", m->machno);
}

/*
 * ilock via i8250enable via i8250console
 * needs m->machno, sys->machptr[] set, and
 * also 'up' set to nil.
 */
static void
machsysinit(void)
{
	memset(m, 0, sizeof(Mach));
	m->machno = 0;
	m->online = 1;
	sys->machptr[m->machno] = &sys->mach;
	m->stack = PTR2UINT(sys->machstk);
	m->vsvm = sys->vsvmpage;
	sys->nmach = 1;
	sys->nonline = 0;
	cpuactive(0);
	sys->copymode = 0;			/* COW */
	up = nil;
	fakecpuhz();
}

/*
 * Release the hounds.
 * Don't delay here; a 2 s. delay prevents interrupts on other cpus.
 */
static void
schedcpus(void)
{
	int i;
	Mach *mp;

	for(i = 1; i < MACHMAX; i++){
		mp = sys->machptr[i];
		if(mp == nil)
			continue;

		lock(&active);
		cpuactive(i);
		unlock(&active);

		mp->color = corecolor(i);
		if(mp->color < 0)
			mp->color = 0;
		mp->online = 1;
	}
	coherence();
	print("%d cpus running\n", sys->nonline);
}

static void
consearlyinit(void)
{
	cgainit();
	delay(500);
	if (getconf("*envloaded"))	/* 9boot loaded me? */
		i8250console("0");
	else
		i8250console("0 b9600");
	consputs = cgaconsputs;
}

/* set size of memory for page tables & qmalloc */
static void
setkernmem(void)
{
	if (sys->pmend < GB) {		/* sanity: must be at least 1 GB */
		print("sys->pmend %,llud; assuming 1 GB of ram\n", sys->pmend);
		sys->pmend = GB;
	}
	if (sys->pmend <= KSEG0SIZE)
		kernmem = 600*MB;		/* arbitrary */
	if (kernmem >= sys->pmend/4)
		kernmem = ROUNDUP(sys->pmend/4, PGSZ);
//	print("kernel space = %,lld\n", kernmem);
}

static void
setupapic(void)
{
	apiconline();
	intrenable(IdtTIMER, apictimerintr, 0, -1, "APIC timer");
	apictimerenable();		/* start my clock ticking */
	apictprput(0);
}

static void
multiprocinit(void)
{
	sipi();				/* awaken any other cpus */
	setepochtsc();
	schedcpus();
}

static void
dumpmbi(u32int mbmagic, u32int mbi)
{
	if(vflag){
		print("multiboot: &ax = %#p = %#ux, bx = %#ux\n",
			&mbmagic, mbmagic, mbi);
		multiboot(mbmagic, mbi, 1);
	}
}

/* first things after zeroing BSS */
static void
physmemconsopts(u32int mbmagic, u32int mbi)
{
	cgapost(sizeof(uintptr)*8);
	machsysinit();
	asminit();
	/* make asmlist & palloc.mem from multiboot memory map */
	if (multiboot(mbmagic, mbi, 0) < 0)
		panic("no multiboot info");
	/* we should know physical memory size (sys->pmend) here */

	mbopts(oargc, oargv);
	inioptions();
	consearlyinit();	/* start cga & serial output */
	kmesginit();

	vsvminit(MACHSTKSZ);

//	cpuactive(0);		/* now done by machsysinit */
	active.exiting = 0;
}

/* also calls printinit for console input */
static void
configcpumem(int apu)
{
	/*
	 * Mmuinit before meminit because it makes mappings and
	 * flushes the TLB via m->pml4->pa.
	 */
	setkernmem();
	mmuinit();			/* uses kernmem; sets sys->vm* */

	ioinit();			/* sets up maps for 8080 I/O ports */
	if (Ps2)
		kbdinit();

	meminit();			/* map KSEG[02] to ram */
	archinit();			/* populate #P with cputype */
	mallocinit();
	if (apu)
		apueccon();		/* uses pci & malloc */
	trapinit();

	/*
	 * Printinit will cause the first malloc call to happen (printinit->
	 * qopen->malloc).  If the system dies here, it's probably due to malloc
	 * not being initialised correctly, or the data segment is misaligned
	 * (it's amazing how far you can get with things like that completely
	 * broken).
	 */
	printinit();	/* actually, establish cooked console input queue */

	pcireset();			/* turn off bus masters & intrs */
	/*
	 * This is necessary with GRUB and QEMU.  Without it an interrupt can
	 * occur at a weird vector, because the vector base is likely different,
	 * causing havoc.  Do it before any APIC initialisation.
	 */
	i8259init(IdtPIC);

//	acpiinit();			/* no-op generated by ../mk/parse */
	mpsinit();
	setupapic();			/* starts clock ticking */
}

extern int warmboot;

static void
iswarmboot(void)
{
	/* in case of direct load reboot from 9k that bypasses bios */
	if (nvramread(Cmosreset) == Rstwarm)
		nvramwrite(Cmosreset, Rstpwron);
	print(" warm restart from Plan 9");
}

int isapu(void);

void
main(u32int mbmagic, u32int mbi)
{
	static ulong align = 1234;

	m = (Mach *)sys->machpage;
	up = nil;
	memset(edata, 0, end - edata);	/* zero BSS */
	physmemconsopts(mbmagic, mbi);
	/* we know physical memory size (sys->pmend) here */

	fmtinit();
	print("\nPlan 9 (amd64)");
	if (align != 1234)
		panic("mis-aligned data segment");
	if (warmboot)
		iswarmboot();
	print("\n");
	dumpmbi(mbmagic, mbi);

	sethz();			/* may use timer I/O ports */
	configcpumem(isapu());		/* also resets pci devices */

	timersinit();
	if (Ps2)
		kbdenable();
	fpuinit();
	psinit();
	initimage();
	cgapost(3);

	links();
	devtabreset();			/* discover all devices */
	cgapost(2);		/* hardware but other cpus is initialised */

	pageinit();		/* make non-kernel memory available for users */
	cgapost(1);
	userinit();			/* hand-craft init process */
	cgapost(0);

	multiprocinit();
	cgapost(9);

	schedinit();			/* start user init process; no return */
	panic("cpu0 schedinit returned");
}

void
init0(void)
{
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

	devtabinit();

	if(!waserror()){
		snprint(buf, sizeof(buf), "%s %s", "AMD64", conffile);
		ksetenv("terminal", buf, 0);
		ksetenv("cputype", "amd64", 0);
		if(cpuserver)
			ksetenv("service", "cpu", 0);
		else
			ksetenv("service", "terminal", 0);
		confsetenv();
		poperror();
	}
	kproc("alarm", alarmkproc, 0);
	/* sp is result of bootargs */
	touser(sp);
}

uintptr
bootargs(uintptr base)
{
	int i;
	uintptr ssize;
	char **av, *p;

	/*
	 * Push the boot args onto the stack.
	 * Make sure the validaddr check in syscall won't fail
	 * because there are fewer than the maximum number of
	 * args by subtracting sizeof(up->arg).
	 */
	i = oargblen+1;
	p = UINT2PTR(STACKALIGN(base + PGSZ - sizeof(up->arg) - i));
	memmove(p, oargb, i);

	/*
	 * Now push argc and the argv pointers.
	 * This isn't strictly correct as the code jumped to by
	 * touser in init9.[cs] calls startboot (port/initcode.c) which
	 * expects arguments
	 * 	startboot(char* argv0, char* argv[])
	 * not the usual (int argc, char* argv[]), but argv0 is
	 * unused so it doesn't matter (at the moment...).
	 */
	av = (char**)(p - (oargc+2)*sizeof(char*));
	ssize = base + PGSZ - PTR2UINT(av);
	*av++ = (char*)oargc;
	for(i = 0; i < oargc; i++)
		*av++ = (oargv[i] - oargb) + (p - base) + (USTKTOP - PGSZ);
	*av = nil;

	return USTKTOP - ssize;
}

void
userinit(void)
{
	Proc *p;
	Segment *s;
	KMap *k;
	Page *pg;

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
	 *
	 * N.B. make sure there's enough space for syscall to check
	 *	for valid args and
	 *	space for gotolabel's return PC
	 * AMD64 stack must be quad-aligned.
	 */
	p->sched.pc = PTR2UINT(init0);
	p->sched.sp = PTR2UINT(p->kstack+KSTACK-sizeof(up->arg)-sizeof(uintptr));
	p->sched.sp = STACKALIGN(p->sched.sp);

	/*
	 * User Stack
	 *
	 * Technically, newpage can't be called here because it
	 * should only be called when in a user context as it may
	 * try to sleep if there are no pages available, but that
	 * shouldn't be the case here.
	 */
	s = newseg(SG_STACK, USTKTOP-USTKSIZE, USTKTOP);
	p->seg[SSEG] = s;
	pg = newpage(Zeropage, s, USTKTOP-(1<<s->lg2pgsize), 0);
	segpage(s, pg);
	k = kmap(pg);
	sp = bootargs(VA(k));
	/* sp will be init0's stack pointer via touser */
	kunmap(k);

	/*
	 * Text
	 */
	s = newseg(SG_TEXT, UTZERO, UTZERO+PGSZ);
	s->flushme++;
	p->seg[TSEG] = s;
	pg = newpage(Zeropage, s, UTZERO, 0);
	memset(pg->cachectl, PG_TXTFLUSH, sizeof(pg->cachectl));
	segpage(s, pg);
	k = kmap(s->map[0]->pages[0]);
	memmove(UINT2PTR(VA(k)), initcode, sizeof initcode);
	kunmap(k);

	ready(p);
}

static void
drainuart(void)
{
	int i;

	if (!islo()) {
		iprint("drainuart: called splhi\n");
		delay(100);
		return;
	}
	for (i = 300; i > 0 && consactive(); i--)
		delay(10);
}

static void
shutdown(int ispanic)
{
	int ms, once;

	/* simplify life by shutting off any watchdog */
	if (watchdogon) {
		watchdog->disable();
		watchdogon = 0;
	}

	lock(&active);
	if(ispanic)
		active.ispanic = ispanic;
	else if(m->machno == 0 && !iscpuactive(0))
		active.ispanic = 0;		/* reboot */
	once = iscpuactive(m->machno);
	/*
	 * setting exiting will make hzclock() on each processor call exit(0),
	 * which calls shutdown(0) and mpshutdown(), which idles non-bootstrap
	 * cpus and returns on bootstrap processors (to permit a reboot).
	 * clearing our bit in active.machsmap avoids calling exit(0) from
	 * hzclock() on this processor.
	 */
	cpuinactive(m->machno);
	active.exiting = 1;
	unlock(&active);

	if(once)
		iprint("cpu%d: idling...", m->machno);
	spllo();
	for(ms = 5*1000; ms > 0; ms -= TK2MS(2)){
		delay(TK2MS(2));
		if(sys->nonline <= 1 && consactive() == 0)
			break;
	}

	wbinvd();
	if(active.ispanic && m->machno == 0){
		if(cpuserver)
			delay(30000);
		else
			for(;;)
				idlehands();
	} else
		delay(1000);
}

void
exit(int ispanic)
{
	shutdown(ispanic);
	mpshutdown();
}

static int
okkernel(int magic)
{
	/* we can load amd64 plan 9 kernels too */
	return magic == I_MAGIC || magic == S_MAGIC;
}

int (*isokkernel)(int) = okkernel;

/*
 * if we have to reschedule, up must be set (i.e., we must be in a
 * process context).
 */
void
runoncpu(int cpu)
{
	if (m->machno == cpu)
		return;			/* done! */
	if (up == nil)
		panic("runoncpu: nil up");
	if (up->nlocks)
		print("runoncpu: holding locks, so sched won't work\n");
	procwired(up, cpu);
	sched();
	if (m->machno != cpu)
		iprint("cpu%d: can't switch proc to cpu%d\n", m->machno, cpu);
}

void	apicresetothers(void);

static void
shutothercpus(void)
{
	/*
	 * the boot processor is cpu0.  execute this process on it
	 * so that the new kernel has the same cpu0.  this only matters
	 * because the hardware has a notion of which processor was the
	 * boot processor and we look at it at start up.
	 */
	if (m->machno != 0 && up)
		runoncpu(0);

	/*
	 * the other cpus could be holding locks that will never get
	 * released (e.g., in the print path) if we put them into
	 * reset now, so ask them to shutdown gracefully, then force
	 * them into reset.  once active.rebooting is set, any or all
	 * of the other cpus may be idling but not servicing interrupts.
	 */
	lock(&active);
	active.rebooting = 1;		/* request other cpus shutdown */
	unlock(&active);
	shutdown(Shutreboot);

	delay(100);	/* let other cpus shut down gracefully */

	/* any intrs to other cpus will not be delivered hereafter */
	apicresetothers();
	delay(20);
	iprint("other cpus in reset\n");
	delay(20);
}

static void
doreset(void)
{
	/*
	 * Blame Intel.  Reverting to protected mode shouldn't be so messy.
	 * We instead reset and rely on pxe or disk booting.
	 */
	iprint("restart unimplemented (blame Intel); resetting system instead\n");
	delay(500);
	splhi();
	pcireset();		/* disable pci bus masters & intrs */
	ioapiconline();		/* clear intr redirections */
	spllo();
	archreset();		/* we can now use the uniprocessor reset */
	notreached();
}

#define REBOOTADDR 0x11000	/* reboot code - physical address; ~90 bytes */

Sipi *cpu0sipi;

static void
rebootviasipi(void *phyentry, void *code, long size)
{
	static Sipireboot cpu0rebootargs;

	/*
	 * ipi response routine arranges to get from real to protected mode,
	 * then to tramp with these args.
	 */
	popcpu0sipi(&cpu0rebootargs, cpu0sipi, (void *)REBOOTADDR,
		phyentry, (void *)PADDR(code), size);
	wbinvd();

	/*
	 * we can't send ourselves an INIT IPI, because that would suspend or
	 * reset us, so play it a little loose and send ourselves a STARTUP IPI.
	 * SIPIHANDLER was installed during MP startup.
	 */
	iprint("sending startup ipi to cpu0..."); delay(1000);
	apicsipi(m->apicno, mmuphysaddr(SIPIHANDLER));
	iprint("cpu0 waiting for startup ipi...\n"); delay(1000);
}

/*
 * shutdown this kernel (size @ code), copy another into place via
 * trampoline code, jump to the new kernel in long mode.
 */
void
reboot(void *phyentry, void *code, long size)
{
	uchar *bytes;
	void (*tramp)(void *, void *, ulong);

	writeconf();
	drainuart();

	/*
	 * interrupts (including uart) may be routed to any or all cpus, so
	 * shutdown devices, other cpus, and interrupts (rely upon iprint
	 * hereafter).
	 */
	devtabshutdown();
	drainuart();		/* before stopping cpus & interrupts */
	shutothercpus();	/* other cpus may be idling; reset them */

	/*
	 * should be the only processor running now.
	 * any intrs to other cpus will not be delivered hereafter.
	 */
	memset(active.machsmap, 0, sizeof active.machsmap);
	splhi();

	/* we've been asked to just `halt'? */
	if (phyentry == 0 && code == 0 && size == 0) {
		doreset();
		notreached();
	}

	/*
	 * Modify the machine page table to directly map low memory.
	 * This allows the reboot code to turn off the page mapping.
	 */
	mmuidentitymap();

	/* setup reboot trampoline function */
	tramp = (void *)REBOOTADDR;		/* phys addr */
	memmove(tramp, rebootcode, sizeof(rebootcode));
	coherence();
	wbinvd();

	spllo();
	bytes = code;
	if (bytes[0] != 0xfa)
		iprint("new kernel missing initial CLI instruction\n");
	iprint("starting new kernel at %#p, via trampoline at %#p...",
		phyentry, tramp);
	delay(1000);
	splhi();
	pcireset();			/* disable pci bus masters & intrs */
	ioapiconline();			/* clear intr redirections */

	/* off we go - never to return */
	if (Useipi)
		rebootviasipi(phyentry, code, size);
	else
		(*tramp)(phyentry, (void *)PADDR(code), size);
	for (;;)
		idlehands();
}
