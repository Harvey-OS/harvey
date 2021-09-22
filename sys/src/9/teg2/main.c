#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include <tos.h>
#include <pool.h>
#include "arm.h"
#include "init.h"
#include "reboot.h"

/*
 * Where configuration info is left for the loaded programme.
 * This will turn into a structure as more is done by the boot loader
 * (e.g. why parse the .ini file twice?).
 */
#define BOOTARGS	((char*)CONFADDR)
#define	BOOTARGSLEN	(16*KB)			/* limit in devenv.c */
#define	MAXCONF		64
#define MAXCONFLINE	160

enum {
	Minmem	= 256*MB,			/* conservative default */

	/* space for syscall args, return PC, top-of-stack struct */
	Ustkheadroom	= sizeof(Sargs) + sizeof(uintptr) + sizeof(Tos),
};

#define isascii(c) ((uchar)(c) > 0 && (uchar)(c) < 0177)

Mach* machaddr[MAXMACH] = { (Mach *)MACHADDR };

/*
 * Option arguments from the command line.
 * oargv[0] is the boot file.
 * Optionsinit() is called from multiboot()
 * or some other machine-dependent place
 * to set it all up.
 */
static int oargc;
static char* oargv[20];
static char oargb[128];
static int oargblen;
static char oenv[4096];

static uintptr sp; /* XXX - must go. user stack of init proc; bootargs→touser */

int vflag;
char debug[256];

static Lock testlock;

/* store plan9.ini contents here at least until we stash them in #ec */
static char confname[MAXCONF][KNAMELEN];
static char confval[MAXCONF][MAXCONFLINE];
static int nconf;

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

void
addconf(char *name, char *val)
{
	int i;

	i = findconf(name);
	if(i < 0){
		if(val == nil || nconf >= MAXCONF)
			return;
		i = nconf++;
		strecpy(confname[i], confname[i]+sizeof(confname[i]), name);
	}
	/* overwrite current value in place */
	strecpy(confval[i], confval[i]+sizeof(confval[i]), val);
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
	if(n >= BOOTARGSLEN)
		error("kernel configuration too large");
	memmove(BOOTARGS, p, n);
	memset(BOOTARGS + n, '\n', BOOTARGSLEN - n);
	poperror();
	free(p);
}

/*
 * assumes that we have loaded our /cfg/pxe/mac file at CONFADDR
 * (usually 0x1000) with tftp in u-boot.  no longer uses malloc, so
 * can be called early.
 */
static void
plan9iniinit(void)
{
	char *k, *v, *next;

	k = (char *)CONFADDR;
	if(!isascii(*k))
		return;

	for(; k && *k != '\0'; k = next) {
		if (!isascii(*k))		/* sanity check */
			break;
		next = strchr(k, '\n');
		if (next)
			*next++ = '\0';

		if (*k == '\0' || *k == '\n' || *k == '#')
			continue;
		v = strchr(k, '=');
		if(v == nil)
			continue;		/* mal-formed line */
		*v++ = '\0';

		addconf(k, v);
	}
}

static void
optionsinit(char* s)
{
	char *o;

	strcpy(oenv, "");
	o = strecpy(oargb, oargb+sizeof(oargb), s)+1;
	if(getenv("bootargs", o, o - oargb) != nil)
		*(o-1) = ' ';

	oargblen = strlen(oargb);
	oargc = tokenize(oargb, oargv, nelem(oargv)-1);
	oargv[oargc] = nil;
}

char*
getenv(char* name, char* buf, int n)
{
	char *e, *p, *q;

	p = oenv;
	while(*p != 0){
		if((e = strchr(p, '=')) == nil)
			break;
		for(q = name; p < e; p++){
			if(*p != *q)
				break;
			q++;
		}
		if(p == e && *q == 0){
			strecpy(buf, buf+n, e+1);
			return buf;
		}
		p += strlen(p)+1;
	}

	return nil;
}

/* enable scheduling of this cpu */
void
machon(uint cpu)
{
	ilock(&active);
	if (!iscpuactive(cpu)) 		/* currently off? */
		cpuactive(cpu);
	iunlock(&active);
}

/* disable scheduling of this cpu */
void
machoff(uint cpu)
{
	ilock(&active);
	if (iscpuactive(cpu))		/* currently on? */
		cpuinactive(cpu);
	iunlock(&active);
}

void
machinit(void)
{
	Mach *m0;

	if (m == 0)
		panic("machinit: nil m");
	if(machaddr[m->machno] != m)
		panic("machinit: machaddr[%d] != %#p", m->machno, m);
	if (canlock(&testlock))
		panic("machinit: cpu%d: locks don't work", m->machno);

	m->ticks = 1;
	m->perf.period = 1;
	if (m->machno != 0) {
		/* synchronise with cpu 0 */
		m0 = MACHP(0);
		m->ticks = m0->ticks;
		m->fastclock = m0->fastclock;
		m->cpuhz = m0->cpuhz;
		m->delayloop = m0->delayloop;
	}
	if (m->machno != 0 &&
	    (m->fastclock == 0 || m->cpuhz == 0 || m->delayloop == 0))
		panic("buggered cpu 0 Mach");

	machon(m->machno);
	fpoff();
}

/* l.s has already zeroed Mach, which now contains our stack. */
void
mach0init(void)
{
	if (m == 0)
		panic("mach0init: nil m");

	machaddr[0] = m;
	m->machno = 0;

	conf.nmach = 1;
	cpuactive(0);
	lock(&testlock);		/* hold this forever */
	machinit();

	active.exiting = 0;
	up = nil;
}

/*
 *  count CPU's, set up their mach structures and l1 ptes.
 *  we're running on cpu 0 and our data structures were
 *  statically allocated.
 */
void
launchinit(void)
{
	int mach, cpu, navailcpus;
	Mach *mm;
	PTE *l1;

	navailcpus = getncpus();
	for(mach = 1; mach < navailcpus; mach++){
		machaddr[mach] = mm = mallocalign(MACHSIZE, MACHSIZE, 0, 0);
		if(mm == nil)
			panic("launchinit: no memory");
		memset(mm, 0, MACHSIZE);
		mm->machno = mach;
		/* load defaults from cpu 0 */
		mm->ticks = m->ticks;
		mm->fastclock = m->fastclock;
		mm->cpuhz = m->cpuhz;
		mm->delayloop = m->delayloop;

		l1 = (PTE *)(L1CPU0 + mach*L1SIZE);
		/* bootstrap this new cpu with our current L1 table */
		mmul1copy(l1, (char *)L1CPU0);
		allcacheswbse(l1, L1SIZE);

		/* l1 will be populated for real by mmuinit() */
		mmusetnewl1(mm, l1);
		allcacheswbse(mm, sizeof *mm);
	}
	allcacheswbse(machaddr, sizeof machaddr);

	/* SMP & FW are already on when we get here; u-boot set them? */
	for (cpu = 1; cpu < navailcpus && cpu < MAXMACH; cpu++)
		if (startcpu(cpu) < 0)
			print("launchinit: cpu%d didn't start\n", cpu);
		else
			conf.nmach++;

	/* reflects MP operation, not permission to go MP. */
	active.thunderbirdsarego = 1;
}

void
dump(void *vaddr, int words)
{
	ulong *addr;

	addr = vaddr;
	while (words-- > 0)
		print("%.8lux%c", *addr++, words % 8 == 0? '\n': ' ');
}

static void
launchproc(void *)
{
	launchinit();
	pexit("kproc exiting", 1);
}

int
inkzeroseg(void)
{
	return (getpc() & KSEGM) == KSEG0;
}

/* cpu 0 init: enable l1 caches (and scu) and mmu. */
void
l1cachesmmuon(void)
{
	if (m->machno != 0)
		panic("main entered on cpu other than 0");

	/* mmuinit maps high vectors later */
	memmove(0, vectors, (char *)_vrst - (char *)vectors);
	exceptinit();

	scuoff();		/* step 2: invalidate all scu tags */
	scuinval();
	cacheinit();		/* writes cacheconf for cache ops */
	l2pl310off();		/* step 3: disable PL310 L2 cache */
	scuon();		/* step 4: enable the scu */
	mmuinit0();		/* set up cpu0's page tables */
	cachedinv();		/* step 1b: inval. L1 D cache before enabling */
	cachedon();		/* step 5: turn my L1 caches on */

	/* We're supposed to wait until l1 & l2 are on before smpcoheron(). */
	/* now that l1 caches are on, include cacheability and shareability */
	mmuon(PADDR(L1CPU0) | TTBlow);
}

/* cpu0 step 6: enable the PL310. */
void
l2on(void)
{
	cachedwbinv();
	allcacheson();
	l2cache->on();
	l2cache->info(&cachel[2]);
	cortexa9cachecfg();		/* needs l2 cache on first */
}

/*
 * at entry, mmu, scu, and caches are off, and the pc is in the zero segment.
 * l.s has set m for cpu0, set up to nil, printed "\r\n", verified data segment
 * alignment, and zeroed bss & Mach, and established a stack in Mach.
 */
void
main(int cpuid)
{
	m->machno = cpuid;
	m->printok = 0;		/* actually: don't lock; lets iprint work */
	warpregs(0);		/* ensure that we're executing in the 0 seg */
	l1cachesmmuon();
	warpregs(KZERO);	/* warp PC, SB, SP & m into the virtual map */
	/*
	 * NB: now running at virtual KZERO+something, with
	 * PC, SB, m, and SP adjusted to KZERO space.
	 */
	watchdogoff();
	iprint("Plan 9 from Bell Labs (arm)\n\n");
	mmuzerouser();	/* vacate mapping 0 -> PHYSDRAM for user space */

	/*
	 * important: reset the interrupt controller.  without this,
	 * cpu1 doesn't get interrupts after a reboot.
	 */
	periphreset();
	poweron();

	l2on();			/* cpu0 step 6: enable the PL310. */
	cachedwbinv();
	smpcoheron();		/* cpu0 step 7: set smp mode; enables atomics */
	/*
	 * this cpu is now participating in cache coherence maintained by
	 * the scu.  as other cpus come on-line and call smpcoheron, they will
	 * participate too.
	 * NB: ldrex/strex and thus locks, ainc/adec, etc. will now work.
	 */

	mach0init();
	mmuinit();
	optionsinit("/boot/boot boot");
	/* want plan9.ini to be able to affect memory sizing in confinit */
	plan9iniinit();		/* before we step on plan9.ini in low memory */

	trapinit();		/* so confinit can probe memory to size it */
	confinit();		/* figures out amount of memory */
	navailcpus = getncpus();
	xinit();		/* xinit prints (if it can) */
	irqtooearly = 0;	/* now that xinit and trapinit have run */
	sgienable();

	/*
	 * Printinit will cause the first malloc call.
	 * (printinit->qopen->malloc) unless any of the
	 * above (like clockinit) do an irqenable, which
	 * will call malloc.
	 * If the system dies here it's probably due
	 * to malloc(->xalloc) not being initialised
	 * correctly, or the data segment is misaligned
	 * (it's amazing how far you can get with
	 * things like that completely broken).
	 *
	 * (Should be) boilerplate from here on.
	 */
	printinit();
	kmesginit();
	quotefmtinstall();
	kbdenable();

	archreset();			/* cfg clock signals, print cache cfg */
	clockinit();			/* start clocks, including watchdog */
	timersinit();

	if (vfyintrs() < 0)
		panic("cpu0: not receiving interrupts");
	cpuidprint();
	chkmissing();

	procinit0();
	initseg();

	links();
	conf.monitor = 0;
//	screeninit();			/* hdmi video not implemented yet */

	print("pcireset...");
	pcireset();		/* this infrequently hangs after reboot */
	print("\n");

	chandevreset();			/* most devices are discovered here */
	i8250console();
	m->printok = 1;			/* serial cons is on, cpu0 locks work */

	pageinit();			/* prints "1020M memory: ⋯ */
	swapinit();
	userinit();

	/* moved launchinit into a kproc started by userinit */
	schedinit();
	panic("cpu%d: schedinit returned", m->machno);
}

/*
 * called on a cpu other than 0 from _vrst in lexception.s,
 * still in the zero segment.  L2 cache and scu are on (cpu0 did that),
 * but our L1 D cache, mmu, SMP coherency, and interrupts should be off.
 * our mmu will start with an exact copy of cpu0's l1 page table
 * as it was after userinit ran.  m has been set to Mach* for this cpu.
 */
void
seccpustart(void)
{
	m->printok = 0;		/* actually: don't lock; lets iprint work */
	up = nil;
	if (m->machno == 0)
		panic("seccpustart called on cpu 0");
	if (iscpuactive(m->machno))
		panic("cpu%d: resetting after started", m->machno);
	if (iscpureset(m->machno))
		panic("cpu%d: in reset in seccpustart", m->machno);

	warpregs(0);		/* ensure that we're executing in the 0 seg */
	errata();		/* work around cortex-a9 bugs */
	cortexa9cachecfg();
	exceptinit();
	watchdogoff();
	/*
	 * starting from an old copy of cpu0's L1 ptes,
	 * redo double map of PHYSDRAM, KZERO in this cpu's ptes.
	 * map v 0 -> p 0 so we can run after enabling mmu.
	 */
	mmudoublemap();
	cachedinv(); /* cpu non-0 step 1b: inval. L1 D cache before enabling */

	/*
	 * step 2: turn my L1 D cache on; need it (and mmu) for tas below.
	 * need branch prediction to make delay() timing right.
	 */
	cachedon();
	smpcoheron();	/* step 3: enable cache coherency, for atomics */

	/* mystery: mmuon(x | TTBlow) stops cpu1 from coming up, ... */
	mmuon(PADDR(m->mmul1));
	m->printok = 1;		/* serial cons is on, locks work on this cpu */
	ttbput(ttbget() | TTBlow);	/* ... but this is okay.  huh? */

	warpregs(KZERO);  /* warp the PC, SB, SP and m into the virtual map */

	/*
	 * now running at virtual KZERO+something with
	 * PC, SB, m, and SP adjusted to KZERO space.
	 */
	hivecs();
	machinit();			/* bumps nmach, adds bit to machs */
	machoff(m->machno);		/* not ready to schedule yet */

	/* establish new kernel stack for this cpu in Mach */
	setsp((uintptr)m + MACHSIZE - 4 - ARGLINK);

	/* clock signals and scu are system-wide and already on */
	clockshutdown();

	poweron();

	trapinit();
	sgienable();
	clockenable(m->machno);
	clockinit();			/* sets loop delay */
	timersinit();
	confirmup();		/* notify cpu0 that we're up (or down) */
	cpuidprint();

	/*
	 * 8169 has to initialise before we get past this, thus cpu0
	 * has to schedule processes first.
	 */
	awaitstablel1pt();
	mmuinit();	/* update our l1 pt from cpu0's, zero user ptes */

	fpoff();
	machon(m->machno);		/* now ready to schedule */
	irqreassign();
	schedinit();
	panic("cpu%d: schedinit returned", m->machno);
}

static int rebooting;

/*
 * take this cpu out of service and wait for others to shut down.
 * if we're cpu0 and they don't, put them into reset.
 */
static void
shutdown(int ispanic)
{
	int ms, once, cpu, want;

	want = 0;
	if (m->machno == 0)
		/*
		 * the other cpus could be holding locks that will never get
		 * released (e.g., in the print path) if we put them into
		 * reset now, so ask them to shutdown gracefully first.
		 */
		for (want = 0, cpu = 1; cpu < navailcpus; cpu++)
			want |= 1 << cpu;

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
		delay((MAXMACH - m->machno)*100);	/* stagger them */
		print("cpu%d: exiting in shutdown\n", m->machno);
	}
	spllo();
	if (m->machno == 0)
		ms = 10*1000;
	else
		ms = 1000;
	for(; ms > 0; ms -= TK2MS(2)){
		delay(TK2MS(2));
		if(active.nmachs == 0 && consactive() == 0)
			break;
	}
	iprint("cpu%d: shutdown done: active.nmachs %#d\n",
		m->machno, active.nmachs);
	if (m->machno == 0)
		forcedown(want);
}

/*
 *  exit kernel either on a panic or user request
 */
void
exit(int ispanic)
{
	splhi();
	if (m->machno == 0) {
		delay(2000);
		shutdown(ispanic);
		iprint("cpu%d: rebooting in exit\n", m->machno);
		prstkuse();
		delay(3000);
		cpucleanup();
		archreboot();		/* no return */
	} else {
		shutdown(ispanic);
		if (!rebooting) {
			ilock(&active);
			cpuactive(0);		/* make sure cpu0 runs exit */
			iunlock(&active);
		}
		cpucleanup();
		stopcpu(m->machno);
		resetcpu(m->machno);	/* no return */
	}
	cpuwfi();
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

/* try to reschedule this process onto the designated cpu. */
int
runoncpu(uint cpu)
{
	if (m->machno == cpu)
		return 0;
	/* if it's marked as out-of-service, sched won't schedule on it. */
	if (!iscpuactive(cpu)) {
		iprint("runoncpu: cpu%d not active, can't switch to it\n", cpu);
		return -1;
	}
	if (up != nil) {
		procwired(up, cpu);
		sched();
		coherence();
	}
	return m->machno == cpu? 0: -1;
}

/* shut down the other cpus by forcing them into reset. */
void
forcedown(uint want)
{
	int cpu, ms, nmach;

	nmach = conf.nmach;
	for (ms = 3*1000; ms > 0 && (cpusinreset() & want) != want; ms--) {
		delay(1);
		coherence();
	}
	if ((cpusinreset() & want) == want) {
		iprint("secondary cpus shutdown voluntarily\n");
		return;
	}

	for (cpu = 1; cpu < nmach; cpu++)
		if (!iscpureset(cpu) && want & (1<<cpu)) {
			iprint("resetting cpu%d\n", cpu);
			stopcpu(cpu);		/* make really sure */
		}
	if ((cpusinreset() & want) == want)
		return;

	/* take no chances.  force other cpus into reset. */
	delay(10);
	for (cpu = 1; cpu < nmach; cpu++)
		resetcpu(cpu);
	delay(10);
	if (!ckcpurst())
		iprint("secondary cpus refused to reset!\n");
}

/*
 * the new kernel is already loaded at address `code'
 * of size `size' and entry point `entry'.
 */
void
reboot(void *phyentry, void *code, ulong size)
{
	void (*f)(ulong, ulong, ulong);

	watchdogoff();
	writeconf();

	/*
	 * the boot processor is cpu0.  execute this function on it
	 * so that the new kernel has the same cpu0.
	 */
	if (m->machno != 0)
		cachedwb();
	if (runoncpu(0) < 0)
		print("reboot: running on cpu%d instead of cpu0.  beware!\n",
			m->machno);
	watchdogoff();
	rebooting = 1;
	shutdown(0);

	/*
	 * must be the only processor running now
	 */
	prstkuse();
	print("pcireset...");
	pcireset();
	print("\n");
	print("cpu%d: reboot entry %#p, code @ %#p size %ld\n",
		m->machno, phyentry, PADDR(code), size);
	delay(100);

	/* turn off buffered serial console */
	serialoq = nil;
	kprintoq = nil;
	screenputs = nil;

	/* shutdown devices */
	chandevshutdown();

	clockshutdown();

	splhi();
	intrshutdown();

	/* setup reboot trampoline function */
	f = (void*)REBOOTADDR;
	memmove(f, rebootcode, sizeof(rebootcode));

	allcacheswbinv();
	l2cache->off();
	cachedwbinv();
	/*
	 * rebootcode expects L1 to be on.
	 */
	cacheiinv();

	/* off we go - never to return */
	(*f)((ulong)phyentry, PADDR(code), size);

	iprint("loaded kernel returned!\n");
	archreboot();
}

/*
 *  starting place for first process
 */
void
init0(void)
{
	int i;
	char buf[2*KNAMELEN];

	up->nerrlab = 0;
	coherence();
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
		snprint(buf, sizeof(buf), "%s %s", "ARM", conffile);
		ksetenv("terminal", buf, 0);
		ksetenv("cputype", "arm", 0);
		if(cpuserver)
			ksetenv("service", "cpu", 0);
		else
			ksetenv("service", "terminal", 0);

		/* convert plan9.ini variables to #e and #ec */
		for(i = 0; i < nconf; i++) {
			ksetenv(confname[i], confval[i], 0);
			ksetenv(confname[i], confval[i], 1);
		}
		poperror();
	}
	kproc("alarm", alarmkproc, 0);
	kproc("launchproc", launchproc, 0);

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
	p = UINT2PTR(STACKALIGN(base + BY2PG - Ustkheadroom - i));
	memmove(p, oargb, i);

	/*
	 * Now push argc and the argv pointers.
	 * This isn't strictly correct as the code jumped to by
	 * touser in init9.s calls startboot (port/initcode.c) which
	 * expects arguments
	 * 	startboot(char *argv0, char **argv)
	 * not the usual (int argc, char* argv[]), but argv0 is
	 * unused so it doesn't matter (at the moment...).
	 */
	av = (char**)(p - (oargc+2)*sizeof(char*));
	ssize = base + BY2PG - PTR2UINT(av);
	*av++ = (char*)oargc;
	for(i = 0; i < oargc; i++)
		*av++ = (oargv[i] - oargb) + (p - base) + (USTKTOP - BY2PG);
	*av = nil;

	/*
	 * Leave space for the return PC of the
	 * caller of initcode.  sp is for touser.
	 */
	sp = USTKTOP - ssize - sizeof(void*);
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
	p->sched.pc = PTR2UINT(init0);
	p->sched.sp = PTR2UINT(p->kstack+KSTACK-sizeof(up->s.args)-sizeof(uintptr));
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
	s->flushme++;
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
	memmove(UINT2PTR(VA(k)), initcode, sizeof initcode);
	kunmap(k);

	allcacheswb();

	ready(p);
}

Conf conf;			/* XXX - must go - gag */

Confmem tsmem[nelem(conf.mem)] = {
	/*
	 * Memory available to Plan 9:
	 */
	{ .base = PHYSDRAM, .limit = PHYSDRAM + Minmem, },
};
ulong memsize = DRAMSIZE;

static int
gotmem(uintptr sz)
{
	uintptr addr;

	/* back off a little from the end */
	addr = (uintptr)KADDR(PHYSDRAM + sz - BY2WD);
	if (probeaddr(addr) >= 0)	/* didn't trap? memory present */
		return sz;
	return -1;
}

void
confinit(void)
{
	int i;
	ulong kpages, npages;
	uintptr pa;
	char *p;
	Confmem *cm;

	/*
	 * Copy the physical memory configuration to Conf.mem.
	 */
	if(nelem(tsmem) > nelem(conf.mem))
		panic("memory configuration botch");
	if(0 && (p = getconf("*maxmem")) != nil) {
		memsize = strtoul(p, 0, 0) - PHYSDRAM;
		if (memsize < 16*MB)		/* sanity */
			memsize = 16*MB;
	}

	/*
	 * see if all that memory exists; if not, find out how much does.
	 * trapinit must have been called first.
	 */
	memsize -= RESRVDHIMEM;			/* reserve end of memory */
	i = gotmem(memsize);
	if (i < 0)
		panic("can't find %ld bytes of memory", memsize);
	memsize = i;

	tsmem[0].limit = PHYSDRAM + memsize;
	memmove(conf.mem, tsmem, sizeof(tsmem));

	/*
	 *  we assume that the kernel is at the beginning of one of the
	 *  contiguous chunks of memory and entirely fits therein.
	 */
	conf.npage = 0;
	pa = PADDR(PGROUND(PTR2UINT(end)));
	for(cm = conf.mem; cm < conf.mem + nelem(conf.mem); cm++){
		/* take kernel out of allocatable space */
		if(pa > cm->base && pa < cm->limit)
			cm->base = pa;

		cm->npage = (cm->limit - cm->base)/BY2PG;
		conf.npage += cm->npage;
		if (cm->npage != 0)
			kmsgbase = (uchar *)cm->limit -
				BY2PG*(HOWMANY(KMESGSIZE, BY2PG) + 1);
	}
	/* omit top pages for kmesg buffer */
	npages = conf.npage - (HOWMANY(KMESGSIZE, BY2PG) + 1);
	conf.upages = (npages*80)/100;
	conf.ialloc = ((npages-conf.upages)/2)*BY2PG;

	/* set up other configuration parameters */
	conf.nproc = 100 + ((conf.npage*BY2PG)/MB)*5;
	if(cpuserver)
		conf.nproc *= 3;
	if(conf.nproc > 2000)
		conf.nproc = 2000;
	conf.nswap = conf.npage*3;
	conf.nswppo = 4096;
	conf.nimage = 200;

	/*
	 * it's simpler on mp systems to take page-faults early,
	 * on reference, rather than later, on write, which might
	 * require tlb shootdowns.
	 */
	conf.copymode = 1;		/* copy on reference */

	/*
	 * Guess how much is taken by the large permanent
	 * datastructures. Mntcache and Mntrpc are not accounted for
	 * (probably ~300KB).
	 */
	kpages = npages - conf.upages;
	kpages *= BY2PG;
	kpages -= conf.upages*sizeof(Page)	/* palloc.pages in pageinit */
		+ conf.nproc*sizeof(Proc)  /* procalloc.free in procinit0 */
		+ conf.nimage*sizeof(Image)	/* imagealloc.free in initseg */
		+ conf.nswap		/* swapalloc.swmap in swapinit */
		+ conf.nswppo*sizeof(Page*);	/* iolist in swapinit */
	mainmem->maxsize = kpages;
	if(!cpuserver)
		/*
		 * give terminals lots of image memory, too; the dynamic
		 * allocation will balance the load properly, hopefully.
		 * be careful with 32-bit overflow.
		 */
		imagmem->maxsize = kpages;

	/* don't call archconfinit here; see archreset */
}

void
advertwfi(void)			/* advertise my in-wfi status */
{
	active.wfi[m->machno] = 1;
	coherence();
}

void
unadvertwfi(void)		/* advertise my out-of-wfi status */
{
	active.wfi[m->machno] = 0;
	coherence();
}

void
idlehands(void)
{
	int s, advertised;

	/* don't go into wfi until my local timer is ticking */
	if (m->ticks == 0)
		return;

	s = splhi();
	m->inidlehands++;

	/* advertise iff the kernel on this cpu has initialised */
	advertised = m->inidlehands == 1 &&
		active.thunderbirdsarego && !active.exiting;
	if (advertised)
		advertwfi();

	l2pl310sync();			/* erratum 769419 */
	wfi();

	if (advertised)
		unadvertwfi();

	m->inidlehands--;
	splx(s);
}

/*
 * interrupt any cpu, other than me, currently in wfi.
 * need not be exact.
 */
void
wakewfi(void)
{
	uint cpu;

	if (!active.thunderbirdsarego || active.exiting)
		return;
	for (cpu = 0; cpu < conf.nmach; cpu++)
		if (active.wfi[cpu] && cpu != m->machno) {
			intrcpu(cpu, 0);
			break;
		}
}

void (*lockwake)(void) = wakewfi;
