/*
 * initialise a risc-v RV64G system, start all cpus, and
 * start scheduling processes on them, notably /boot/boot as process 1.
 * also contains graceful shutdown and reboot code.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "io.h"
#include "ureg.h"
#include "riscv.h"

#include "init.h"
#include "reboot.h"
#include <a.out.h>
#include <ctype.h>

//#undef DBG
//#define DBG(...) iprint(__VA_ARGS__)

int	sifivegetc(Uart *);
void	sifiveputc(Uart *, int);

Uart	sifiveuart[];

enum {
	Livedangerously = 1,		/* flag: don't panic when sbi lies */
	Nosbiipi = 1,			/* flag: don't use sbi to send ipis */
};

enum Sbihartsts {			/* results of sbihartstatus */
	Hstarted, Hstopped, Hstartpend, Hstoppend,
	Hsuspended, Hsuspendpend, Hresumepend,
};

char *hartstates[] = {
	"started", "stopped", "start_pending", "stop_pending",
	"suspended", "suspend_pending", "resume_pending",
};

/* from kernel config */
extern char defnvram[];
extern uvlong cpuhz;

int	hartcnt = 0;		/* machno allocator; init to avoid bss */
int	bootmachmode = MACHINITIAL;	/* init to avoid bss */
uintptr	phyuart = PAUart0;
void	(*prstackmax)(void);

static uintptr usp;		/* user stack of init proc */

Sys* sys = nil;			/* initializer keeps sys out of bss */
usize sizeofSys = sizeof(Sys);
usize sizeofSyscomm = sizeof(Syscomm);

/*
 * Option arguments from the command line (obsolete).
 * oargv[0] is the boot file (/boot/boot).
 * mboptinit() was called from multiboot() on amd64 to set it all up.
 */
static int oargc;
static char* oargv[20];
static char oargb[128];
static int oargblen;

char cant[] = "can't happen: notreached reached\n";
char dbgflg[256] = { 0, };
char cputype[] = "riscv64";

static int vflag = 0;
static int secschedok;
static char havehartstate[HARTMAX];
static char hartstate[HARTMAX];
static char hartrunning[HARTMAX];

#ifdef FAKEPCI
void
pcireset(void)
{
}
#endif

/* dreg from parsing multiboot options on pcs */
void
mboptinit(char* s)
{
	oargblen = strecpy(oargb, oargb+sizeof(oargb), s) - oargb;
	oargc = tokenize(oargb, oargv, nelem(oargv)-1);
	oargv[oargc] = nil;
}

static void
fakecpuhz(void)
{
	/*
	 * Need something for initial delays
	 * until a timebase is worked out.
	 */
	m->cpuhz = cpuhz;
	m->cpumhz = cpuhz/1000000;
	m->perf.period = 1;
}

static vlong
sethz(void)
{
	vlong hz;

	hz = archhz();
	if(hz != 0){
		m->cpuhz = hz;
		m->cpumhz = hz/1000000;
	}
	return hz;
}

static void
setepoch(void)
{
	Clint *clint;

	if (m->machno == 0 && sysepoch == 0) {
		sys->ticks = m->ticks = 0;
		clint = clintaddr();
		/* clint->mtime is only zeroed upon reset on some systems */
		WRCLTIME(clint, 1);		/* attempt to reset */
		coherence();
		sysepoch = RDCLTIME(clint);
	}
}

static void
mptimesync(void)
{
	if (m->machno == 0) {
		sys->timesync = RDCLTIME(clintaddr());
		coherence();
	}
}

/*
 * start a cpu other than 0.  entered splhi with initial id map page table
 * and with PC in KSEG0.
 */
void
squidboy(uint machno)
{
	lock(&active);
	cpuactive(machno);
	unlock(&active);

	/*
	 * starting here, we are using kernel addresses, and they should
	 * be upper addresses, but won't be until mmuinitap.
	 */
	archmmu();			/* work out page sizes available */
	assert(m->npgsz >= 1);

	/* mmuinitap switches to private copy of cpu0's page table */
	mmuinitap();	/* returns executing in high addresses with vmap */
	fakecpuhz();

	DBG("Hello Squidboy %d\n", machno);

	/*
	 * Beware the Curse of The Non-Interruptable Were-Temporary.
	 */
	if(sethz() == 0) {
		print("squidboy: cpu%d: 0Hz\n", machno);
		notreached();
	}

	fpuinit();

	/*
	 * Handshake with cpu to let it know the startup succeeded.
	 */
	m->splpc = 0;			/* unused */
	coherence();

	/*
	 * Handshake with main to proceed with initialisation.
	 */
	DBG("mach %d waiting for epoch\n", machno);
	/* sys->timesync is set on cpu0 in mptimesync from multiprocinit (1) */
	while(sys->timesync == 0)
		pause();
	mptimesync();

	/*
	 * Cannot allow interrupts while waiting for online.
	 * A clock interrupt here might call the scheduler,
	 * and that would be a mistake.
	 */
	DBG("mach%d: waiting for online flag\n", machno);
	while(!m->online)  /* set on cpu0 in onlinewaiting from schedcpus (2) */
		pause();

	DBG("mach%d: online color %d\n", machno, m->color);
	while (!secschedok)	/* set on cpu0 in main before schedinit (3) */
		pause();

	DBG("mach %d is go %#p %#p\n", machno, m, m->ptroot->va);
	timersinit();	/* uses m->cpuhz; set up HZ timers on this cpu */

	clocksanity();			/* enables clock & allows clock intrs */
	DBG("[cpu%d scheding after %lld cycles]\n", machno,
		rdtsc() - m->boottsc);

	delay(machno*100); /* stagger startup & let cpu0 get to schedinit 1st */
	schedinit();			/* no return */
	panic("cpu%d: schedinit returned", machno);
}

/*
 * for cpu 0 only.  sets m->machno and sys->machptr[], and
 * needs 'up' to be set to nil.
 */
static void
machsysinit(void)
{
//	memset(m, 0, sizeof(Mach));		/* done by start.s */
	m->machno = 0;
	m->online = 1;
	sys->machptr[m->machno] = &sys->mach;
	m->stack = PTR2UINT(sys->machstk);
//	sys->nmach = 1;
	sys->nonline = 0;
	cpuactive(0);
	sys->copymode = 0;			/* COW */
	fakecpuhz();
}

int
ipitohart(int hart)
{
	uvlong hartbm;

	if (Nosbiipi || nosbi) {
		clintaddr()->msip[hart] = 1;
		coherence();
		return 0;
	} else {
		hartbm = 1ull << hart;
		return sbisendipi(&hartbm) == 0? 0: -1;
	}
}

void
clearipi(void)
{
	clintaddr()->msip[m->hartid] = 0;
	coherence();
 	clrsipbit(Ssie);
//	if (!nosbi)
//		sbiclearipi();		/* deprecated */
}

static int
onlinewaiting(int machno)
{
	Mach *mp;

	mp = sys->machptr[machno];
	if(mp == nil || !mp->waiting || mp->online)
		return 0;
	if (mp->hartid == 0 && hobbled)	/* boot hart? */
		return 0;

	/* cpu is waiting but not yet on-line, so bring it up */
	mp->color = corecolor(machno);
	if(mp->color < 0)
		mp->color = 0;
	mp->online = 1;
	mp->waiting = 0;
	coherence();

	DBG("%d on w mach %#p...", machno, mp);
	iprint("%d ", machno);
	return 1;
}

static int
gethartstate(int hart)
{
	int state;

	if (havehartstate[hart])
		return hartstate[hart];		/* cached state */
	else {
		/*
		 * BUG? SBI sometimes reports a single, incorrect, random
		 * hart as started.  Could just start them all; will that
		 * work if status is wrong?  It would be good not to have
		 * renegade harts running random code, yet we currently
		 * have no way to suspend them.
		 */
		state = sbihartstatus(hart);
		if (state >= 0) {
			hartstate[hart] = state;
			havehartstate[hart] = 1;
		}
	}
	return state;
}

/*
 * start all stopped cpus.  some or all may already be started,
 * but we are going to make the unwarranted assumption that systems
 * with SBI HSM calls only start one cpu initially (the only other
 * legitimate configuration is to start them all at once).
 * stopped cpus will not be waiting but need to be started.
 * simulate the non-hsm spontaneous start up of all harts
 * except 0 if it's hobbled (thus running and managing the others).
 */
static void
hsmstartall(void)
{
	int hart, state;

	/* map harts' states */
	iprint("my machno %d hartid %d\n", m->machno, m->hartid);
	for (hart = 0; hart < HARTMAX; hart++) {
		state = gethartstate(hart);
		if (state >= 0) {
			iprint("hart %d in state %s%s\n", hart,
				hartstates[state],
				hart == m->hartid? " (cpu0)": "");
			if ((state == Hstarted) != (hartrunning[hart] == 1))
				iprint("sbi status for hart %d is wrong\n", hart);
			delay(50);
		}
	}

	/* start any stopped harts, and they should all be stopped except me */
	for (hart = hobbled? 1: 0; hart < HARTMAX; hart++) {
		if (hart == m->hartid)
			continue;
		state = gethartstate(hart);
		if (state == Hstopped) {
			iprint("sbi starting hart %d\n", hart);
			sbihartstart(hart, ensurelow((uintptr)_main), hart);
			/* give time to start & bump hartcnt; stagger startups */
			delay(150);
		} else if (state >= 0)
			/*
			 * probably started but not running our code.  sbi or
			 * u-boot bug?  we can't have a cpu executing random
			 * code.  if only there were a way to reset a hart...
			 */
			if (Livedangerously) {
				iprint("hart %d in state %s on sbi hsm system;"
					" sending it an IPI.\n",
					hart, hartstates[state]);
				ipitohart(hart);
				delay(200);
				state = gethartstate(hart);
				if (state == Hstopped) {
					iprint("sbi starting hart %d\n", hart);
					sbihartstart(hart, ensurelow((uintptr)
						_main), hart);
					/* give time to start & bump hartcnt */
					delay(150);
				} else
					iprint("can't stop hart %d\n", hart);
			} else
				panic("hart %d in state %s on sbi hsm system",
					hart, hartstates[state]);
	}
}

/*
 * Release the hounds.  After boot, secondary cpus will be
 * looping waiting for cpu0 to signal them.
 */
static void
schedcpus(int ncpus)
{
	int machno, running, tries;

	running = 1;			/* cpu0 */
	USED(running, ncpus);
	if (MACHMAX <= 1 || running == ncpus)
		return;

	/* if rebooting, let other cpus jump to entry from reboottramp */
	sys->secstall = 0;  /* let them start then poll mp->online */
	coherence();
	delay(50);
	sys->secstall = 0;
	coherence();
	/*
	 * even on a fairly slow machine, a cpu can start in under .6 s.
	 * the range on 600MHz icicle with debugging prints:
	 * 236244054-323670622 cycles = .39-.54 s.
	 */
	for (tries = 0; running < ncpus && tries <  100; tries++) {
		delay(100);	/* let them indicate presence; be patient */
		for(machno = 1; machno < MACHMAX; machno++) /* skip me (cpu0) */
			running += onlinewaiting(machno);
	}
	iprint("%d cpus running, %d missing\n", running, ncpus - running);
}

/*
 * allocate memory for and awaken any other cpus, if needed.
 * copy the page table root to a private copy for each cpu.
 * hartcnt global is incremented in start.s by each hart.
 */
static void
multiprocinit(void)
{
	int ncpus;

	if (havesbihsm)
		hsmstartall();

	/*
	 * when we get here, hartcnt should be correct and secondary
	 * cpus should be running, if only spinning while waiting.
	 */
	ncpus = hartcnt;
	cpusalloc(ncpus);
	mptimesync();		/* signal secondaries via sys->timesync (1) */
	schedcpus(ncpus);	/* signal secondaries via m->online (2) */
}

static int
probemembank(int bank, Membank *mbp)
{
	if (probeulong((ulong *)(mbp->addr + MB), 0) < 0) {
		iprint("start of bank %d (after %#p) missing\n",
			bank, mbp->addr);
		return -1;
	}
	if (probeulong((ulong *)(mbp->addr + mbp->size - MB), 0) < 0) {
		iprint("end of bank %d (before %#p) missing\n",
			bank, mbp->addr + mbp->size);
		return -1;
	}
	return 0;
}

static void
debuginit(void)
{
#ifdef SIFIVEUART
	int c;

	iprint("debug? ");
	for (;;) {
		c = sifivegetc(&sifiveuart[0]);
		if (c == '\n' || c == '\r')
			break;
		dbgflg[c] = 1;
		sifiveputc(&sifiveuart[0], c);		/* echo */
	}
#endif
}

/* early things after zeroing BSS */
static void
physmemcons(void)
{
	int i;
	uintptr nsize, banksize;
	Membank *mbp;

	/*
	 * make asmlist & palloc.mem from a memory map (membanks).
	 * one asmmapinit call per (usually discontiguous) memory bank.
	 * reserve topram of first bank only.
	 */
	sys->pmbase = PHYSMEM;		/* KTZERO */
	machsysinit();
	asminit();		/* carve out kernel AS from physical map */
	nsize = 0;
	for (i = 0; membanks[i].addr != 0; i++) {
		mbp = &membanks[i];
		banksize = mbp->size;
		if (i == 0)
			banksize -= topram();
		if (probemembank(i, mbp) >= 0) {
			asmmapinit(mbp->addr, banksize, AsmMEMORY);
			nsize += banksize;
		}
	}
	memtotal = nsize;
	/* we know physical memory size and end (sys->pmend) here */

	mboptinit("/boot/ipconfig ether");	/* dreg from pc multiboot */

	/* device discovery hasn't happened yet, so be careful. */
//	i8250console("0 b115200");	/* non-standard but u-boot default */
	i8250console("0");
	debuginit();
	kmesginit();
//	cpuactive(0);		/* now done by machsysinit */
	active.exiting = 0;
}

/* also calls printinit for console input */
static void
configcpumem(int)
{
	/*
	 * Mmuinit before meminit because it makes mappings and flushes
	 * the TLB.  It will get page table pages from Sys as needed.
	 */
	setkernmem();
	mmuinit();	/* uses kernmem; sets sys->vm*; vmaps soc devices */
	meminit();		/* map KSEG[02] to ram, call mallocinit */
	archinit();		/* populate #P with cputype */
	trapinit();

	/*
	 * Printinit should cause the first malloc call to happen (printinit->
	 * qopen->malloc).  If the system dies here, it's probably due to malloc
	 * not being initialised correctly, or the data segment is misaligned
	 * (it's amazing how far you can get with things like that completely
	 * broken).
	 */
	printinit();	/* actually, establish cooked console input queue */

	pcireset();		/* turn off bus masters & intrs */
}

static void
chooseidler(void)
{
	if (dbgflg['p'])
		idlepause = 1;
	if (dbgflg['w'])
		idlepause = 0;
	if (idlepause)
		print("using PAUSE to idle.\n");
	else if (havesbihsm)
		print("using SBI suspend to idle.\n");
	else
		print("using WFI to idle; may get stuck on tinyemu or U74.\n");
}

static void
prearlypages(void)
{
	if (sys->freepage)
		iprint("%lud/%d early pages allocated (probably by vmap)\n",
			(ulong)(((char *)sys->freepage - (char *)sys->pts)/PGSZ),
			EARLYPAGES);
}

/*
 * copy reboot trampoline function to its expected location.
 * a reboot could be requested at any time, and all cores will
 * jump to the trampoline.  some of them may still be there,
 * so only overwrite if necessary.
 */
static void
copyreboottramp(void)
{
	if (memcmp(sys->reboottramp, rebootcode, 250) != 0)
		memmove(sys->reboottramp, rebootcode, sizeof(rebootcode));
	cacheflush();
}

/* cpu-specific setup for this cpu */
static void
setupcpu(int cpu)
{
	int hartid;

	up = nil;
	hartid = m->hartid;
	m->virtdevaddrs = 0;		/* device vmaps not yet in effect */
	trapsclear();
	trapvecs();
	sys->machptr[cpu] = m;		/* publish for other cpus */
	hartstate[hartid] = Hstarted;
	havehartstate[hartid] = 1;
	hartrunning[hartid]++;
	coherence();
	DBG("[cpu%d hart%d]\n", cpu, hartid);
	m->boottsc = rdtsc();
}

/*
 * this is not entirely safe; if it crashes, comment it out.
 * nevertheless, it can be a help when documentation is unclear.
 */
static void
probemem(void)
{
	int i;
	uintptr maxgb;

	maxgb = ADDRSPCSZ / GB;
	print("memory map by 2 GB to %lld GB: ", maxgb);
	for (i = 0; i < maxgb; i += 2)
		if (probeulong((ulong *)((uvlong)i*GB + MB), 0) >= 0)
			print("%d ", i);
	print("\n");
}

/*
 * entered in supervisor mode with paging on, both identity map
 * and KSEG0->0 map in effect, and PC in KSEG0 space.
 * using initial page table until pageidmap then mmuinit (via configcpumem)
 * runs on cpu0, or mmuinitap runs on other cpus, which copies page table
 * root from cpu0.
 */
void
main(int cpu)
{
	uintptr caller;

	setupcpu(cpu);
	if (cpu != 0) {
		squidboy(cpu);
		notreached();
	}

	/* most of this set up is for system-wide resources */
	plicoff();			/* system wide, so only once */
	caller = getcallerpc(&cpu);
	pageidmap(); /* replicate init mappings w permanent pt, mach0ptrootpg */

	fmtinit();			/* install P, L, R, Q verbs */
	/*
	 * this also calls kmesginit but if we linked with prf.$O, logging will
	 * only start once mallocinit is called from configcpumem.
	 */
	physmemcons();
	/* we know physical memory size (including sys->pmend) here */

	/* if epoch is set after timersinit, timers will be confused */
	setepoch();		/* get notions of time right here */

	print("\n\nPlan 9 (risc-v RV%dG)\n", BI2BY*(int)sizeof(uintptr));
	if ((intptr)caller >= 0)
		print("called from low memory: %#p\n", caller);

	sanity();
	cpuidinit();			/* needs pagingmode set */
	sethz();			/* prints cpu id */
	calibrate();

	configcpumem(0);	/* mmuinit, vmaps soc devs, resets pci devs */
	chooseidler();

	timersinit();	/* uses m->cpuhz; set up HZ timers on this cpu */
	addclock0link(zerotrapcnts, 1000);
	fpuinit();
	psinit();
	initimage();

	clocksanity();		/* enables the clock */
//	probemem();		/* debugging on new hardware */
	links();
	devtabreset();		/* discover all devices */
	splhi();
	prearlypages();
	copyreboottramp();

	/*
	 * start all cpus.  assumes no kprocs will be started (e.g., due
	 * to devtabreset) until schedinit runs init0 (see userinit).
	 *
	 * kernel memory mappings (including in VMAP) must be fixed when
	 * multiprocinit is called, since they will be copied for each cpu
	 * before it starts.  drivers typically call vmap in their
	 * reset (`pnp') functions, called from devtabreset.
	 *
	 * we ensure that the secondary cpus do not schedule until userinit
	 * (and ideally schedinit) have run on cpu0.
	 */
	if(!dbgflg['S'])
		multiprocinit();
	/* pt w vmaps have been copied into each private pt. */
	m->virtdevaddrs = 1;
	mmuflushtlb(m->ptroot);		/* zero user mappings */

	pageinit();	/* make non-kernel memory available for users */
	userinit();	/* hand-craft init process; needs demand paging */
	secschedok = 1;	/* signal secondaries to schedule (3) */
	schedinit();	/* start cpu0 scheduling; will run user init process */
	panic("cpu0 schedinit returned");
}

/*
 * process 1 runs this (see userinit), so it's now okay for devtabinit to
 * spawn kprocs.
 */
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
		snprint(buf, sizeof(buf), "%s %s", cputype, conffile);
		ksetenv("terminal", buf, 0);
		ksetenv("cputype", cputype, 0);
		if(cpuserver)
			ksetenv("service", "cpu", 0);
		else
			ksetenv("service", "terminal", 0);
		ksetenv("nvram", defnvram, 0);
		ksetenv("nobootprompt", "tcp", 0);
//		confsetenv();			/* no conf to convert */
		poperror();
	}
	kproc("alarm", alarmkproc, 0);
	/*
	 * start user phase executing initcode[] from init.h, compiled
	 * from init9.c (main) and ../port/initcode.c (startboot),
	 * which in turn execs /boot/boot.
	 *
	 * sp is a result of bootargs.
	 */
	touser(usp);
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
	p = (void *)STACKALIGN(base + PGSZ - sizeof(up->arg) - i);
	memmove(p, oargb, i);

	/*
	 * Now push argc and the argv pointers.
	 * This isn't strictly correct as the code jumped to by
	 * touser in init9.[cs] calls startboot (port/initcode.c) which
	 * expects arguments
	 * 	startboot(char* argv0, char* argv[])
	 * not the usual (int argc, char* argv[]), but argv0 is
	 * unused so it doesn't matter (at the moment...).
	 * Added +1 to ensure nil isn't stepped on, another for vlong padding.
	 */
	av = (char**)(p - (oargc+2+1+1)*sizeof(char*));
	ssize = base + PGSZ - PTR2UINT(av);
	*av++ = (char*)oargc;
	for(i = 0; i < oargc; i++)
		*av++ = (oargv[i] - oargb) + (p - base) + (USTKTOP - PGSZ);
	*av = nil;
	return STACKALIGN(USTKTOP - ssize);
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
	 */
	p->sched.pc = PTR2UINT(init0);	/* proc 1 starts here in kernel phase */
	p->sched.sp = PTR2UINT(p->kstack+KSTACK-sizeof(up->arg));
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
	pg = newpage(Zeropage, s, USTKTOP-(1LL<<s->lg2pgsize), 0);
	segpage(s, pg);
	k = kmap(pg);
	usp = bootargs(VA(k));
	/* usp will be init0's stack pointer via touser */
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
	memmove((void *)VA(k), initcode, sizeof initcode);
	kunmap(k);

	wbinvd();
	/* not using address-space ids currently */
	putsatp(pagingmode | (m->ptroot->pa / PGSZ));

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

/* shutdown this cpu */
static void
shutdown(int ispanic)
{
	int ms, once;

	/* simplify life by shutting off any watchdog */
	if (watchdogon && watchdog) {
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
		iprint("cpu%d: %s...", m->machno, m->machno? "idling": "exiting");
	spllo();
	for(ms = 5*1000; ms > 0; ms -= TK2MS(2)){
		delay(TK2MS(2));
		if(sys->nonline <= 1 && consactive() == 0)
			break;
	}

	cacheflush();
	m->clockintrsok = 0;
	if(active.ispanic && m->machno == 0){
		if(cpuserver)
			delay(30000);
		else
			for(;;)
				idlehands();
	} else
		delay(1000);
}

/* will be called from hzclock if active.exiting is true */
void
exit(int ispanic)
{
	iprint("cpu%d: exit...", m->machno);
	shutdown(ispanic);
	mpshutdown();
	/* only cpu0 gets here, and only on reboot */
	archreset();
}

static int
okkernel(int magic)
{
	return magic == AOUT_MAGIC;
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

static void
shutothercpus(void)
{
	/*
	 * the other cpus could be holding locks that will never get
	 * released (e.g., in the print path) if we put them into
	 * reset now, so ask them to shutdown gracefully.
	 * once active.rebooting is set, any or all
	 * of the other cpus may be idling but not servicing interrupts.
	 */
	sys->secstall = RBFLAGSET; /* stall sec cores 'til sys->Reboot ready */
	lock(&active);
	active.rebooting = 1;		/* request other cpus shutdown */
	unlock(&active);
	shutdown(Shutreboot);

	/* any intrs to other cpus will not be delivered hereafter */
}

static void
settrampargs(void *phyentry, void *code, long size)
{
	/* set up args for trampoline */
	sys->tramp = (void (*)(Reboot *))PADDR(sys->reboottramp);
	/* jl doesn't produce an extended header, so entry is only 32 bits */
	sys->phyentry = (ulong)PADDR(phyentry);
	sys->phycode = PADDR(code);
	sys->rebootsize = size;
	coherence();
	sys->secstall = 0;  /* let cpus in mpshutdown proceed to trampoline */
	coherence();
}

/*
 * shutdown this kernel, jump to trampoline code in Sys, which copies
 * the next kernel (size @ code) into the addresses it was linked for,
 * and jumps to the new kernel's entry address.
 *
 * put other harts into wfi in trampoline in sys, jump into
 *	id map, jump to trampoline code which copies new kernel into place,
 *	start all harts running it.
 */
void
reboot(void *phyentry, void *code, long size)
{
	if (m->machno != 0 && up)
		runoncpu(0);

	/* other cpus may be idling; make them jump to trampoline */
	if (sys->nonline > 1)
		shutothercpus();		/* calls shutdown */

	if (prstackmax)
		prstackmax();

	/* there's no config and nowhere to store it */
	drainuart();

	/*
	 * interrupts (including uart) may be routed to any or all cpus, so
	 * shutdown devices, other cpus, and interrupts (rely upon iprint
	 * hereafter).
	 */
	devtabshutdown();
	drainuart();		/* before stopping cpus & interrupts */

	/*
	 * should be the only processor scheduling now.
	 * any intrs to other cpus will not be delivered hereafter.
	 */
	memset(active.machsmap, 0, sizeof active.machsmap);
	splhi();
	clockoff(clintaddr());
	plicoff();
	pcireset();			/* disable pci bus masters & intrs */
	putsie(0);
	clrsipbit(~0);

	/* we've been asked to just `halt'? */
	if (phyentry == 0 && code == 0 && size == 0) {
		spllo();
		archreset();	/* we can now use the uniprocessor reset */
		notreached();
	}

	mmulowidmap();			/* disables user mapping too */
	settrampargs(phyentry, code, size);

	delay(100);			/* let secondaries settle */
	while (sys->nonline > 1) {	/* paranoia */
		iprint("%d cpus...", sys->nonline);
		delay(50);
		coherence();
	}

	/* won't be seen */
	iprint("\nstarting new kernel via tramp @ %#p (%#p <- %#p size %,ld)\n",
		sys->tramp, sys->phyentry, sys->phycode,
		(ulong)sys->rebootsize);
	/*
	 * if we were entered in machine mode, we can get back to it from
	 * supervisor mode with an ECALL.  otherwise, we can't get back to it
	 * at all.  the reboot trampoline will adjust the entry point suitably.
	 */
	m->bootmachmode = bootmachmode;		/* for rebootcode.s */
	if (bootmachmode)
		/*
		 * to machine mode to trigger reboot with sys->Reboot args.
		 * in machine mode, it's unsafe to use sys.
		 */
		ecall();			/* see totramp in mtrap.s */
	else
		/* jump into the trampoline in the id map - never to return */
		(*sys->tramp)(&sys->Reboot);
	notreached();
}
