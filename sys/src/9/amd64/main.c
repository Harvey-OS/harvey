/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "init.h"
#include "apic.h"
#include "io.h"
#include "amd64.h"
#include "coreboot.h"

#undef DBG
#define DBG iprint

Conf conf; /* XXX - must go - gag */

static uintptr sp; /* XXX - must go - user stack of init proc */

/* Next time you see a system with cores/sockets running at different clock rates, on x86,
 * let me know. AFAIK, it no longer happens. So the BSP hz is good for the AP hz.
 */
i64 hz;

Sys *const sys = UINT2PTR(KSYS);

/*
 * Option arguments from the command line.
 * oargv[0] is the boot file.
 * Optionsinit() is called from multiboot() to
 * set it all up.
 */
char *cputype = "amd64";
static i64 oargc;
static char *oargv[128];
static char oargb[4096];
static int oargblen;

static int maxcores = 1024; /* max # of cores given as an argument */
static int numtcs = 32;	    /* initial # of TCs */

char dbgflg[256];
static int vflag;

int nosmp = 0;
static int showpost = 0;

// Harvey is out of date on many platforms, and frequently the only
// indication is a brick.
// Modern systems have extensive logging of outb to port 0x80, as well
// as serial over lan support. Further, the emulated serial easily runs
// as fast as the x86 can push it over LPC.
// This post function is intended only for use in main(), when we are
// booting and things might be going badly wrong.
// It is enabled by showpost (see above) and is intended only to be
// turned on when porting, since it will produce output we don't ordinarily
// want to see.
// Don't fall into the trap of using macros for this capability; the Plan 9 rule is that
// code always compiles, and macros usually break that rule. The cost in space
// and time of this bit of extra code is so small as to not matter.
void
post(char *msg, u8 terminal)
{
	if(!showpost)
		return;
	for(int i = 0; i < strlen(msg); i++){
		outb(0x3f8, msg[i]);
		outb(0x80, msg[i]);
	}
	outb(0x80, terminal);
}
void *
sigscan(u8 *address, int length, char *signature)
{
	u8 *e, *p;
	int siglength;

	e = address + length;
	siglength = strlen(signature);
	for(p = address; p + siglength < e; p += 16){
		if(memcmp(p, signature, siglength))
			continue;
		return p;
	}

	return nil;
}

/*
 *	this may need improvement, but right now it's just for
 *	pretty printing below, so it doesn't need to be accurate
 */
static int
ktextaddr(uintptr pc)
{
	return (pc & KTZERO) == KTZERO;
}

void
stacksnippet(void)
{
	Stackframe *stkfr;
	print("cpu%d stack:", machno());
	for(stkfr = stackframe(); stkfr != nil; stkfr = stkfr->next)
		print(" %c:%p", ktextaddr(stkfr->pc) ? 'k' : '?', stkfr->pc);
	print("\n");
}

void
machp_bad(void)
{
	static uintptr trace[256];
	uintptr badpc;
	int i;

	badpc = (uintptr)getcallerpc();
	for(i = 0; i < nelem(trace); i++){
		if(trace[i] == badpc)
			return;
		if(trace[i] == 0)
			break;
	}
	if(i == nelem(trace)){
		print("machp_bad: out of trace space\n");
		return;
	}
	trace[i] = badpc;
	kmprint("machp access spllo,");
	stacksnippet();
}

void
optionsinit(char *s)
{
	oargblen = strlcpy(oargb, s, sizeof(oargb));
	if(oargblen >= sizeof(oargb))
		panic("optionsinit: kernel command line too long");
	oargc = tokenize(oargb, oargv, nelem(oargv) - 1);
	oargv[oargc] = nil;
}

static void
options(int argc, char *argv[])
{
	char *p;
	char *env[2];
	int n, o;
	char envcopy[256];
	extern int printallsyscalls;

	/*
	 * Process flags.
	 * Flags [A-Za-z] may be optionally followed by
	 * an integer level between 1 and 127 inclusive
	 * (no space between flag and level).
	 * '--' ends flag processing.
	 */
	while(--argc > 0){
		char *next = *++argv;
		if(next[0] == '-' && next[1] != '-'){
			while((o = *++argv[0]) != '\0'){
				if(!(o >= 'A' && o <= 'Z') && !(o >= 'a' && o <= 'z'))
					continue;
				n = strtol(argv[0] + 1, &p, 0);
				if(p == argv[0] + 1 || n < 1 || n > 127)
					n = 1;
				argv[0] = p - 1;
				dbgflg[o] = n;
			}
		} else {
			strncpy(envcopy, next, sizeof envcopy);
			gettokens(envcopy, env, 2, "=");
			if(strcmp(env[0], "maxcores") == 0){
				maxcores = strtol(env[1], 0, 0);
			}
			if(strcmp(env[0], "numtcs") == 0){
				numtcs = strtol(env[1], 0, 0);
			}
			if(strcmp(env[0], "printallsyscalls") == 0){
				printallsyscalls = strtol(env[1], 0, 0);
			}
		}
	}
	vflag = dbgflg['v'];
	nosmp = dbgflg['n'];
}

void
loadenv(int argc, char *argv[])
{
	char *env[2];

	/*
	 * Process command line env options
	 */
	while(--argc > 0){
		char *next = *++argv;
		if(next[0] != '-'){
			print("loadenv: processing '%s'\n", next);
			if(strchr(next, '=') == nil){
				print("Ignoring parameter (%s) with no value.\n", next);
				continue;
			}
			if(gettokens(next, env, 2, "=") == 2)
				ksetenv(env[0], env[1], 0);
		}
	}
}

extern int num_cpus;

void
squidboy(Mach *mach)
{
	// FIX QEMU. extern int64_t hz;
	i64 hz;

	sys->machptr[mach->machno] = mach;
	/*
	 * Need something for initial delays
	 * until a timebase is worked out.
	 */
	mach->cpuhz = 2000000000ll;
	mach->cpumhz = 2000;
	mach->perf.period = 1;

	mach->NIX.nixtype = NIXAC;
	// no NIXAC for now.
	mach->NIX.nixtype = NIXTC;

	// NOTE: you can't do ANYTHING here before vsvminit.
	// PRINT WILL PANIC. So wait.
	vsvminit(MACHSTKSZ, mach->NIX.nixtype, mach);

	DBG("Hello squidboy APIC %d (cpu%d)\n", mach->apicno, machno());

	/*
	 * Beware the Curse of The Non-Interruptable Were-Temporary.
	 */
	hz = archhz();
	/* Intel cpu's in archamd64.c must be reviewed */
	if(hz == 0)
		hz = 2000000000ll;
	mach->cpuhz = hz;
	mach->cyclefreq = hz;
	mach->cpumhz = hz / 1000000ll;

	mmuinit();
	if(!apiconline())
		ndnr();
	fpuinit();

	acmodeset(mach->NIX.nixtype);
	mach->splpc = 0;
	mach->online = 1;

	/*
	 * CAUTION: no time sync done, etc.
	 * Stupid print to avoid up = nil or
	 * last cpu couldn't start in nixquids.
	 */
	DBG("Wait for the thunderbirds!\n");
	while(!active.thunderbirdsarego)
		;
	wrmsr(0x10, sys->epoch);
	mach->rdtsc = rdtsc();

	print("cpu%d color %d role %s tsc %lld\n",
	      mach->machno, corecolor(mach->machno), rolename[mach->NIX.nixtype], mach->rdtsc);
	if(mach->machno + 1 > num_cpus){
		num_cpus = mach->machno + 1;
	}
	switch(mach->NIX.nixtype){
	case NIXAC:
		acmmuswitch();
		acinit();
		adec(&active.nbooting);
		ainc(&active.nonline); /* this was commented out */
		acsched();
		panic("squidboy");
		break;
	case NIXTC:
		/*
		 * We only need the idt and syscall entry point actually.
		 * At boot time the boot processor might set our role after
		 * we have decided to become an AC.
		 */
		vsvminit(MACHSTKSZ, NIXTC, mach);

		/*
		 * Enable the timer interrupt.
		 */
		apictimerenab();
		apicpri(0);

		timersinit();
		adec(&active.nbooting);
		ainc(&active.nonline);

		/* Ready? steady? going to timer */
		//ndnr();

		schedinit();
		break;
	}
	panic("squidboy returns (type %d)", mach->NIX.nixtype);
}

static void
testiccs(void)
{
	int i;
	Mach *mach;
	extern void testicc(int);

	/* setup arguments for all */
	for(i = 0; i < MACHMAX; i++)
		if((mach = sys->machptr[i]) != nil && mach->online && mach->NIX.nixtype == NIXAC)
			testicc(i);
	print("bootcore: all cores done\n", i);
}

/*
 * Rendezvous with other cores. Set roles for those that came
 * up online, and wait until they are initialized.
 * Sync TSC with them.
 * We assume other processors that could boot had time to
 * set online to 1 by now.
 */
static void
nixsquids(void)
{
	Mach *mach;
	int i;
	u64 now, start;

	/* Not AC for now :-) */
	numtcs = MACHMAX;
	for(i = 1; i < MACHMAX; i++)
		if((mach = sys->machptr[i]) != nil && mach->online){
			/*
			 * Inter-core calls. A ensure *mp->iccall and mp->icargs
			 * go into different cache lines.
			 */
			mach->NIX.icc = mallocalign(sizeof *machp()->NIX.icc, ICCLNSZ, 0, 0);
			mach->NIX.icc->fn = nil;
			if(i < numtcs){
				sys->nmach++;
				mach->NIX.nixtype = NIXTC;
				sys->nc[NIXTC]++;
			} else
				sys->nc[NIXAC]++;
			ainc(&active.nbooting);
		}
	sys->epoch = rdtsc();
	mfence();
	wrmsr(0x10, sys->epoch);
	machp()->rdtsc = rdtsc();
	active.thunderbirdsarego = 1;
	start = fastticks2us(fastticks(nil));
	do {
		now = fastticks2us(fastticks(nil));
	} while(active.nbooting > 0 && now - start < 1000000);
	if(active.nbooting > 0)
		print("cpu0: %d cores couldn't start\n", active.nbooting);
	active.nbooting = 0;
}

void
DONE(void)
{
	print("DONE\n");
	//prflush();
	delay(10000);
	ndnr();
}

void
HERE(void)
{
	print("here\n");
	//prflush();
	delay(5000);
}

/* The old plan 9 standby ... wave ... */

/* Keep to debug trap.c */
void
wave(int c)
{
	outb(0x3f8, c);
}

void
hi(char *s)
{
	if(!s)
		s = "<NULL>";
	while(*s)
		wave(*s++);
}

void
hihex(u64 x)
{
	const char *hex = "0123456789abcdef";
	for(int i = 60; i >= 0; i -= 4)
		wave(hex[(x >> i) & 0xF]);
}

/*
 * for gdb:
 * call this anywhere in your code.
 *   die("yourturn with gdb\n");
 *   gdb 9k
 *   target remote localhost:1234
 *   display/i $pc
 *   set staydead = 0
 *   stepi, and debug.
 * note, you can always resume after a die. Just set staydead = 0
 */

int staydead = 1;
void
die(char *s)
{
	wave('d');
	wave('i');
	wave('e');
	wave(':');
	hi(s);
	while(staydead)
		;
	staydead = 1;
}

void
badcall(u64 where, u64 what)
{
	hi("Bad call from function ");
	hihex(where);
	hi(" to ");
	hihex(what);
	hi("\n");
	for(;;)
		;
}

void
errstr(char *s, int i)
{
	panic("errstr");
}

static int x = 0x123456;

void
main(Mach *mach, u32 mbmagic, u32 mbaddress)
{
	int postterminal = 1;
	USED(postterminal);

	sys->machptr[mach->machno] = mach;

	mach->self = PTR2UINT(mach);
	mach->machno = 0;
	mach->online = 1;
	mach->NIX.nixtype = NIXTC;
	mach->stack = PTR2UINT(sys->machstk);
	mach->vsvm = sys->vsvmpage;
	mach->externup = nil;

	/*
	 * Need something for initial delays
	 * until a timebase is worked out.
	 */
	mach->cpuhz = 2000000000ll;
	mach->cpumhz = 2000;

	// Initialize VSM so that we can use interrupts and so forth.
	vsvminit(MACHSTKSZ, NIXTC, mach);
	if(machp() != mach)
		panic("After vsvminit, m and machp() are different");

	// The kernel maps the first 4GiB before entry to main().  If the
	// image is too big, we will fail to boot properly.
	if((uintptr)end - KZERO > 4ULL * GiB)
		panic("main: kernel too big: image ends after 4GiB");

	/*
	 * Check that our data is on the right boundaries.
	 * This works because the immediate value is in code.
	 */
	if(x != 0x123456)
		panic("Data is not set up correctly\n");

	sys->cyclefreq = mach->cpuhz;
	sys->nmach = 1;
	active.nonline = 1;
	active.exiting = 0;
	active.nbooting = 0;

	/*
	 * ilock via i8250enable via i8250console
	 * needs machp()->machno, sys->machptr[] set, and
	 * also 'up' set to nil.
	 */
	cgapost(sizeof(uintptr) * 8);

	mallocinit();
	pamapinit();
	multiboot(mbmagic, mbaddress, 0);
	options(oargc, oargv);
	pamapmerge();

	fmtinit();

	cgainit();
	i8250console("0");

	consputs = cgaconsputs;

	print("\nHarvey\n");
	multiboot(mbmagic, mbaddress, 1);
	pamapdump();

	mach->perf.period = 1;
	if((hz = archhz()) != 0ll){
		mach->cpuhz = hz;
		mach->cyclefreq = hz;
		sys->cyclefreq = hz;
		mach->cpumhz = hz / 1000000ll;
	}
	//iprint("archhz returns 0x%lld\n", hz);
	//iprint("NOTE: if cpuidhz runs too fast, we get die early with a NULL pointer\n");
	//iprint("So, until that's fixed, we bring up AP cores slowly. Sorry!\n");

	/*
	 * Mmuinit before meminit because it flushes the TLB.
	 */
	mmuinit();

	ioinit();
	keybinit();
	meminit();
	confinit();
	archinit();

	/*
	 * Acpiinit will cause the first malloc
	 * call to happen.
	 * If the system dies here it's probably due
	 * to malloc not being initialised
	 * correctly, or the data segment is misaligned
	 * (it's amazing how far you can get with
	 * things like that completely broken).
	 */
	acpiinit();

	umeminit();

	/*
	 * This is necessary with GRUB and QEMU.
	 * Without it an interrupt can occur at a weird vector,
	 * because the vector base is likely different, causing
	 * havoc. Do it before any APIC initialisation.
	 */
	i8259init(32);

	procinit0();
	print("before mpacpi, maxcores %d\n", maxcores);
	mpacpi(maxcores);
	trapinit();
	printinit();
	apiconline();
	ioapiconline();
	/* Forcing to single core if desired */
	if(!nosmp){
		sipi();
	} else {
		print("SMP Disabled by command line\n");
	}
	timersinit();
	fpuinit();
	psinit(conf.nproc);
	initimage();
	links();

	keybenable();
	mouseenable();

	devtabreset();
	pageinit();
	swapinit();
	userinit();
	/* Forcing to single core if desired */
	if(!nosmp){
		nixsquids();
		testiccs();
	}

	alloc_cpu_buffers();

	acpistart();
	print("CPU Freq. %dMHz\n", mach->cpumhz);

	print("schedinit...\n");

	schedinit();
}

void
init0(void)
{
	Proc *up = externup();
	char buf[2 * KNAMELEN];

	up->nerrlab = 0;

	/*
	 * if(consuart == nil)
	 * i8250console("0");
	 */
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
		snprint(buf, sizeof(buf), "%s", "AMD64");
		loadenv(oargc, oargv);
		ksetenv("terminal", buf, 0);
		ksetenv("cputype", cputype, 0);
		ksetenv("pgsz", "2097152", 0);
		// no longer. 	confsetenv();
		poperror();
	}
	kproc("alarm", alarmkproc, 0);
	touser(sp);
}

void
bootargs(uintptr base)
{
	int i;
	u32 ssize;
	char **av, *p;

	/*
	 * Push the boot args onto the stack.
	 * Make sure the validaddr check in syscall won't fail
	 * because there are fewer than the maximum number of
	 * args by subtracting sizeof(up->arg).
	 */
	i = oargblen + 1;
	p = UINT2PTR(STACKALIGN(base + BIGPGSZ - sizeof(((Proc *)0)->arg) - i));
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
	av = (char **)(p - (oargc + 2) * sizeof(char *));
	ssize = base + BIGPGSZ - PTR2UINT(av);
	*av++ = (char *)oargc;
	for(i = 0; i < oargc; i++)
		*av++ = (oargv[i] - oargb) + (p - base) + (USTKTOP - BIGPGSZ);
	*av = nil;

	sp = USTKTOP - ssize;
}

void
userinit(void)
{
	Proc *up = externup();
	Proc *p;
	Segment *s;
	KMap *k;
	Page *pg;
	int sno;

	p = newproc();
	p->pgrp = newpgrp();
	p->egrp = smalloc(sizeof(Egrp));
	p->egrp->r.ref = 1;
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
	p->sched.sp = PTR2UINT(p->kstack + KSTACK - sizeof(up->arg) - sizeof(uintptr));
	p->sched.sp = STACKALIGN(p->sched.sp);

	/*
	 * User Stack
	 *
	 * Technically, newpage can't be called here because it
	 * should only be called when in a user context as it may
	 * try to sleep if there are no pages available, but that
	 * shouldn't be the case here.
	 */
	sno = 0;
	s = newseg(SG_STACK | SG_READ | SG_WRITE, USTKTOP - USTKSIZE, USTKSIZE / BIGPGSZ);
	p->seg[sno++] = s;
	pg = newpage(1, 0, USTKTOP - BIGPGSZ, BIGPGSZ, -1);
	segpage(s, pg);
	k = kmap(pg);
	bootargs(VA(k));
	kunmap(k);

	/*
	 * Text
	 */
	s = newseg(SG_TEXT | SG_READ | SG_EXEC, UTZERO, 1);
	s->flushme++;
	p->seg[sno++] = s;
	pg = newpage(1, 0, UTZERO, BIGPGSZ, -1);
	memset(pg->cachectl, PG_TXTFLUSH, sizeof(pg->cachectl));
	segpage(s, pg);
	k = kmap(s->map[0]->pages[0]);
	/* UTZERO is only needed until we make init not have 2M block of zeros at the front. */
	memmove(UINT2PTR(VA(k) + init_code_start - UTZERO), init_code_out, sizeof(init_code_out));
	kunmap(k);

	/*
	 * Data
	 */
	s = newseg(SG_DATA | SG_READ | SG_WRITE, UTZERO + BIGPGSZ, 1);
	s->flushme++;
	p->seg[sno++] = s;
	pg = newpage(1, 0, UTZERO + BIGPGSZ, BIGPGSZ, -1);
	memset(pg->cachectl, PG_TXTFLUSH, sizeof(pg->cachectl));
	segpage(s, pg);
	k = kmap(s->map[0]->pages[0]);
	/* This depends on init having a text segment < 2M. */
	memmove(UINT2PTR(VA(k) + init_data_start - (UTZERO + BIGPGSZ)), init_data_out, sizeof(init_data_out));
	kunmap(k);

	// XXX: Something is faulting when init moves to
	// another core.
	//procwired(p, 0);

	ready(p);
}

void
confinit(void)
{
	int i;

	conf.npage = 0;
	for(i = 0; i < nelem(conf.mem); i++)
		conf.npage += conf.mem[i].npage;
	conf.nproc = 1000;
	conf.nimage = 200;
}

static void
shutdown(int ispanic)
{
	int ms, once;

	lock(&active.l);
	if(ispanic)
		stacksnippet();
	if(ispanic)
		active.ispanic = ispanic;
	else if(machp()->machno == 0 && machp()->online == 0)
		active.ispanic = 0;
	once = machp()->online;
	machp()->online = 0;
	adec(&active.nonline);
	active.exiting = 1;
	unlock(&active.l);

	if(once)
		iprint("cpu%d: exiting\n", machp()->machno);

	spllo();
	for(ms = 5 * 1000; ms > 0; ms -= TK2MS(2)){
		delay(TK2MS(2));
		if(active.nonline == 0 && consactive() == 0)
			break;
	}

	if(active.ispanic && machp()->machno == 0){
		if(cpuserver)
			delay(30000);
		else
			for(;;)
				halt();
	} else
		delay(1000);
}

void
reboot(void *v, void *w, i32 i)
{
	panic("reboot\n");
}

void
exit(int ispanic)
{
	shutdown(ispanic);
	archreset();
}
