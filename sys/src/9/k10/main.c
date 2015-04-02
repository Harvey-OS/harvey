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

#warning "what is kerndate?"
uint32_t kerndate = 0;

Conf conf;			/* XXX - must go - gag */

extern void crapoptions(void);	/* XXX - must go */
extern void confsetenv(void);	/* XXX - must go */

static uintptr_t sp;		/* XXX - must go - user stack of init proc */

uintptr_t kseg0 = KZERO;
Sys* sys = nil;
usize sizeofSys = sizeof(Sys);

/*
 * Option arguments from the command line.
 * oargv[0] is the boot file.
 * Optionsinit() is called from multiboot() to
 * set it all up.
 */
static int64_t oargc;
static char* oargv[20];
static char oargb[128];
static int oargblen;

static int maxcores = 1024;	/* max # of cores given as an argument */
static int numtcs = 32;		/* initial # of TCs */

char dbgflg[256];
static int vflag = 1;

void
optionsinit(char* s)
{
	oargblen = strecpy(oargb, oargb+sizeof(oargb), s) - oargb;
	oargc = tokenize(oargb, oargv, nelem(oargv)-1);
	oargv[oargc] = nil;
}

static void
options(int argc, char* argv[])
{
	char *p;
	int n, o;

	/*
	 * Process flags.
	 * Flags [A-Za-z] may be optionally followed by
	 * an integer level between 1 and 127 inclusive
	 * (no space between flag and level).
	 * '--' ends flag processing.
	 */
	while(--argc > 0 && (*++argv)[0] == '-' && (*argv)[1] != '-'){
		while(o = *++argv[0]){
			if(!(o >= 'A' && o <= 'Z') && !(o >= 'a' && o <= 'z'))
				continue;
			n = strtol(argv[0]+1, &p, 0);
			if(p == argv[0]+1 || n < 1 || n > 127)
				n = 1;
			argv[0] = p-1;
			dbgflg[o] = n;
		}
	}
	vflag = dbgflg['v'];
	if(argc > 0){
		maxcores = strtol(argv[0], 0, 0);
		argc--;
		argv++;
	}
	if(argc > 0){
		numtcs = strtol(argv[0], 0, 0);
		//argc--;
		//argv++;
	}
}

void
squidboy(int apicno)
{
	int64_t hz;

	sys->machptr[m->machno] = m;
	/*
	 * Need something for initial delays
	 * until a timebase is worked out.
	 */
	m->cpuhz = 2000000000ll;
	m->cpumhz = 2000;
	m->perf.period = 1;

	m->nixtype = NIXAC;

	DBG("Hello Squidboy %d %d\n", apicno, m->machno);

	vsvminit(MACHSTKSZ, m->nixtype);

	/*
	 * Beware the Curse of The Non-Interruptable Were-Temporary.
	 */
	hz = archhz();
	if(hz == 0)
		ndnr();
	m->cpuhz = hz;
	m->cyclefreq = hz;
	m->cpumhz = hz/1000000ll;
	mmuinit();
	if(!apiconline())
		ndnr();
	fpuinit();

	acmodeset(m->nixtype);
	m->splpc = 0;
	m->online = 1;

	/*
	 * CAUTION: no time sync done, etc.
	 */
	DBG("Wait for the thunderbirds!\n");
	while(!active.thunderbirdsarego)
		;
	wrmsr(0x10, sys->epoch);
	m->rdtsc = rdtsc();

	print("cpu%d color %d role %s tsc %lld\n",
		m->machno, corecolor(m->machno), rolename[m->nixtype], m->rdtsc);
	switch(m->nixtype){
	case NIXAC:
		acmmuswitch();
		acinit();
		adec(&active.nbooting);
		ainc(&active.nonline);	/* this was commented out */
		acsched();
		panic("squidboy");
		break;
	case NIXTC:
		/*
		 * We only need the idt and syscall entry point actually.
		 * At boot time the boot processor might set our role after
		 * we have decided to become an AC.
		 */
		vsvminit(MACHSTKSZ, NIXTC);

		/*
		 * Enable the timer interrupt.
		 */
		apictimerenab();
		apicpri(0);

		timersinit();
		adec(&active.nbooting);
		ainc(&active.nonline);

		schedinit();
		break;
	}
	panic("squidboy returns (type %d)", m->nixtype);
}

static void
testiccs(void)
{
	int i;
	Mach *mp;
	extern void testicc(int);

	/* setup arguments for all */
	for(i = 0; i < MACHMAX; i++)
		if((mp = sys->machptr[i]) != nil && mp->online && mp->nixtype == NIXAC)
			testicc(i);
	print("bootcore: all cores done\n");
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
	Mach *mp;
	int i;
	uint64_t now, start;

	for(i = 1; i < MACHMAX; i++)
		if((mp = sys->machptr[i]) != nil && mp->online){
			/*
			 * Inter-core calls. A ensure *mp->iccall and mp->icargs
			 * go into different cache lines.
			 */
			mp->icc = mallocalign(sizeof *m->icc, ICCLNSZ, 0, 0);
			mp->icc->fn = nil;
			if(i < numtcs){
				sys->nmach++;
				mp->nixtype = NIXTC;
				sys->nc[NIXTC]++;
			}else
				sys->nc[NIXAC]++;
			ainc(&active.nbooting);
		}
	sys->epoch = rdtsc();
	mfence();
	wrmsr(0x10, sys->epoch);
	m->rdtsc = rdtsc();
	active.thunderbirdsarego = 1;
	start = fastticks2us(fastticks(nil));
	do{
		now = fastticks2us(fastticks(nil));
	}while(active.nbooting > 0 && now - start < 1000000)
		;
	if(active.nbooting > 0)
		print("cpu0: %d cores couldn't start\n", active.nbooting);
	active.nbooting = 0;
}

void
DONE(void)
{
	print("DONE\n");
	prflush();
	delay(10000);
	ndnr();
}

void
HERE(void)
{
	print("here\n");
	prflush();
	delay(5000);
}

// The old plan 9 standby ... wave ... 
void wave(int c)
{
	outb(0x3f8, c);
}

void hi(char *s) 
{
	while (*s)
		wave(*s++);
}
	// for gdb: 
	// call this anywhere in your code. 
	//die("yourturn with gdb\n");
	// gdb 9k
	// target remote localhost:1234
	// display/i $pc
	// set staydead = 0
	// stepi, and debug. 
	// note, you can always resume after a die. Just set staydead = 0
int staydead = 1;
void die(char *s)
{
	wave('d');
	wave('i');
	wave('e');
	wave(':');
	hi(s);
	while(staydead);
	staydead = 1;
}
void bmemset(void *p)
{
	__asm__ __volatile__("1: jmp 1b");
}

void put8(uint8_t c)
{
	char x[] = "0123456789abcdef";
	wave(x[c>>4]);
	wave(x[c&0xf]);
}

void put16(uint16_t s)
{
	put8(s>>8);
	put8(s);
}

void put32(uint32_t u)
{
	put16(u>>16);
	put16(u);
}

void put64(uint64_t v)
{
	put32(v>>32);
	put32(v);
}

void badcall(uint64_t where, uint64_t what)
{
	hi("Bad call from function "); put64(where); hi(" to "); put64(what); hi("\n");
	while (1)
		;
}

void errstr(char *s, int i) {
	panic("errstr");
}

static int x = 0x123456;

void
main(uint32_t mbmagic, uint32_t mbaddress)
{
  // when we get here, m is set to core0 mach.
	sys->machptr[m->machno] = m;
	wrmsr(GSbase, PTR2UINT(&sys->machptr[m->machno]));
	hi("m "); put64((uint64_t)m); hi(" machp "); put64((uint64_t)machp()); hi("\n");
	if (machp() != m)
		die("================= m and machp() are different");
	hi("mbmagic "); put32(mbmagic); hi("\n");
	hi("mbaddress "); put32(mbaddress); hi("\n");
	assert(sizeof(Mach) <= PGSZ);
	int64_t hz;

	// Check that our data is on the right boundaries.
	// This works because the immediate value is in code.
	if (x != 0x123456) 
		die("data is not set up correctly\n");
	wave('H');
	memset(edata, 0, end - edata);

	m = (void *) (KZERO + 1048576 + 11*4096);
	sys = (void *) (KZERO + 1048576);
	wave('a');
	/*
	 * ilock via i8250enable via i8250console
	 * needs m->machno, sys->machptr[] set, and
	 * also 'up' set to nil.
	 */
	cgapost(sizeof(uintptr_t)*8);
	wave('r');
	memset(m, 0, sizeof(Mach));
	wave('v');
	m->machno = 0;
	wave('1');
	m->online = 1;
	wave('2');
	m->nixtype = NIXTC;
	wave('3');
	sys->machptr[m->machno] = &sys->mach;
	wave('4');
	m->stack = PTR2UINT(sys->machstk);
	wave('5');
	m->vsvm = sys->vsvmpage;
	wave('6');
	m->externup = (void *)0;
	active.nonline = 1;
	active.exiting = 0;
	active.nbooting = 0;
	wave('e');
	asminit();
	wave('y');
	multiboot(mbmagic, mbaddress, 0);
	wave(';');
	options(oargc, oargv);
	wave('s');
	// later.
	//crapoptions();
	wave('a');
	/*
	 * Need something for initial delays
	 * until a timebase is worked out.
	 */
	m->cpuhz = 2000000000ll;
	m->cpumhz = 2000;

	cgainit();
	wave('y');
	i8250console("0");
	
	wave('1');
	consputs = cgaconsputs;

	wave('2');
	// It all ends here.
	vsvminit(MACHSTKSZ, NIXTC);
	hi("we're back from vsvminit\n");
	if (machp() != m)
		die("================= after vsvminit, m and machp() are different");
	wave('s');

	fmtinit();
	
	print("\nNIX\n");
	print("NIX m = %p / sys = %p \n", m, sys);
	hi("m is "); put64((uint64_t) m); hi("\n");
	hi("sys is "); put64((uint64_t) sys); hi("\n");
	sys->nmach = 1;			

	if(1){
		multiboot(mbmagic, mbaddress, vflag);
	}

	hi("m: "); put64((uint64_t)m); hi("\n");
	m->perf.period = 1;
	hi("archhz\n");
	if((hz = archhz()) != 0ll){
		m->cpuhz = hz;
		m->cyclefreq = hz;
		m->cpumhz = hz/1000000ll;
	}

hi("gdb\n");
print("hi %d", 1);
print("hi %d", 2);
print("hi %d", 0x1234);
	/*
	 * Mmuinit before meminit because it
	 * flushes the TLB via m->pml4->pa.
	 */
hi("call mmuinit\n");
{	mmuinit(); hi("	mmuinit();\n");}

hi("wait for gdb\n");
{	ioinit(); hi("	ioinit();\n");}
{	kbdinit(); hi("	kbdinit();\n");}
{	meminit(); hi("	meminit();\n");}
{	confinit(); hi("	confinit();\n");}
{	archinit(); hi("	archinit();\n");}
{	mallocinit(); hi("	mallocinit();\n");}

	/* test malloc. It's easier to find out it's broken here, 
	 * not deep in some call chain.
	 * See next note. 
	 */
	void *v = malloc(1234);
	hi("v "); put64((uint64_t)v); hi("\n");
	free(v);
	hi("free ok\n");
	/*
	 * Acpiinit will cause the first malloc
	 * call to happen.
	 * If the system dies here it's probably due
	 * to malloc not being initialised
	 * correctly, or the data segment is misaligned
	 * (it's amazing how far you can get with
	 * things like that completely broken).
	 */
if (0){	acpiinit(); hi("	acpiinit();\n");}
	
{	umeminit(); hi("	umeminit();\n");}
{	trapinit(); hi("	trapinit();\n");}
{	printinit(); hi("	printinit();\n");}

	/*
	 * This is necessary with GRUB and QEMU.
	 * Without it an interrupt can occur at a weird vector,
	 * because the vector base is likely different, causing
	 * havoc. Do it before any APIC initialisation.
	 */
{	i8259init(32); hi("	i8259init(32);\n");}


{	procinit0(); hi("	procinit0();\n");}
{	mpsinit(maxcores); hi("	mpsinit(maxcores);\n");}
{	apiconline(); hi("	apiconline();\n");}
if (0) sipi(); 

{	timersinit(); hi("	timersinit();\n");}
{	kbdenable(); hi("	kbdenable();\n");}
{	fpuinit(); hi("	fpuinit();\n");}
{	psinit(conf.nproc); hi("	psinit(conf.nproc);\n");}
{	initimage(); hi("	initimage();\n");}
{	links(); hi("	links();\n");}
{	devtabreset(); hi("	devtabreset();\n");}
{	pageinit(); hi("	pageinit();\n");}
{	swapinit(); hi("	swapinit();\n");}
{	userinit(); hi("	userinit();\n");}
	if (0) { // NO NIX YET
		{	nixsquids(); hi("	nixsquids();\n");}
		{testiccs(); hi("testiccs();\n");}	
	}
	print("schedinit...\n");
	schedinit();
}

void
init0(void)
{
	char buf[2*KNAMELEN];

	m->externup->nerrlab = 0;

//	if(consuart == nil)
//		i8250console("0");
	spllo();

	/*
	 * These are o.k. because rootinit is null.
	 * Then early kproc's will have a root and dot.
	 */
	m->externup->slash = namec("#/", Atodir, 0, 0);
	pathclose(m->externup->slash->path);
	m->externup->slash->path = newpath("/");
	m->externup->dot = cclone(m->externup->slash);

	devtabinit();

	if(!waserror()){
		snprint(buf, sizeof(buf), "%s %s", "AMD64", conffile);
		ksetenv("terminal", buf, 0);
		ksetenv("cputype", "amd64", 0);
		if(cpuserver)
			ksetenv("service", "cpu", 0);
		else
			ksetenv("service", "terminal", 0);
		ksetenv("pgsz", "2097152", 0);
		confsetenv();
		poperror();
	}
	kproc("alarm", alarmkproc, 0);
	touser(sp);
}

void
bootargs(uintptr_t base)
{
	int i;
	uint32_t ssize;
	char **av, *p;

	/*
	 * Push the boot args onto the stack.
	 * Make sure the validaddr check in syscall won't fail
	 * because there are fewer than the maximum number of
	 * args by subtracting sizeof(m->externup->arg).
	 */
	i = oargblen+1;
	p = UINT2PTR(STACKALIGN(base + /*BIG*/PGSZ - sizeof(m->externup->arg) - i));
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
	ssize = base + /*BIG*/PGSZ - PTR2UINT(av);
	*av++ = (char*)oargc;
	for(i = 0; i < oargc; i++)
		*av++ = (oargv[i] - oargb) + (p - base) + (USTKTOP - /*BIG*/PGSZ);
	*av = nil;

	sp = USTKTOP - ssize;
}

void
userinit(void)
{
	Proc *p;
	Segment *s;
	KMap *k;
	Page *pg;
hi("1\n");
	p = newproc();
	p->pgrp = newpgrp();
	p->egrp = smalloc(sizeof(Egrp));
	p->egrp->ref = 1;
	p->fgrp = dupfgrp(nil);
	p->rgrp = newrgrp();
	p->procmode = 0640;

hi("2\n");
	kstrdup(&eve, "");
	kstrdup(&p->text, "*init*");
	kstrdup(&p->user, eve);

hi("3\n");
	/*
	 * Kernel Stack
	 *
	 * N.B. make sure there's enough space for syscall to check
	 *	for valid args and
	 *	space for gotolabel's return PC
	 * AMD64 stack must be quad-aligned.
	 */
	p->sched.pc = PTR2UINT(init0);
	p->sched.sp = PTR2UINT(p->kstack+KSTACK-sizeof(m->externup->arg)-sizeof(uintptr_t));
hi("SP "); put64(p->sched.sp); hi("\n");
	p->sched.sp = STACKALIGN(p->sched.sp);

hi("4\n");
	/*
	 * User Stack
	 *
	 * Technically, newpage can't be called here because it
	 * should only be called when in a user context as it may
	 * try to sleep if there are no pages available, but that
	 * shouldn't be the case here.
	 */
	// Big pages are not working yet.
#warning "Fix back to use big pages"
	s = newseg(SG_STACK, USTKTOP-USTKSIZE, USTKSIZE/ /*BIG*/PGSZ);
	p->seg[SSEG] = s;

hi("5\n");
	pg = newpage(1, 0, USTKTOP-/*BIG*/PGSZ, /*BIG*/PGSZ, -1);
	segpage(s, pg);
	k = kmap(pg);
	bootargs(VA(k));
	kunmap(k);

hi("6\n");
	/*
	 * Text
	 */
	s = newseg(SG_TEXT, UTZERO, 1);
	s->flushme++;
	p->seg[TSEG] = s;
	pg = newpage(1, 0, UTZERO, /*BIG*/PGSZ, -1);
hi("7\n");
	memset(pg->cachectl, PG_TXTFLUSH, sizeof(pg->cachectl));
	segpage(s, pg);
hi("8\n");
	k = kmap(s->map[0]->pages[0]);
	//memmove(UINT2PTR(VA(k)), initcode, sizeof(initcode));
	memmove(UINT2PTR(VA(k)), init_out, sizeof(init_out));
	kunmap(k);

hi("9\n");
	ready(p);
hi("done \n");
}

void
confinit(void)
{
	int i;

	conf.npage = 0;
	for(i=0; i<nelem(conf.mem); i++)
		conf.npage += conf.mem[i].npage;
	conf.nproc = 1000;
	conf.nimage = 200;
}

static void
shutdown(int ispanic)
{
	int ms, once;

	lock(&active);
	if(ispanic)
		active.ispanic = ispanic;
	else if(m->machno == 0 && m->online == 0)
		active.ispanic = 0;
	once = m->online;
	m->online = 0;
	adec(&active.nonline);
	active.exiting = 1;
	unlock(&active);

	if(once)
		iprint("cpu%d: exiting\n", m->machno);

	spllo();
	for(ms = 5*1000; ms > 0; ms -= TK2MS(2)){
		delay(TK2MS(2));
		if(active.nonline == 0 && consactive() == 0)
			break;
	}

	if(active.ispanic && m->machno == 0){
		if(cpuserver)
			delay(30000);
		else
			for(;;)
				halt();
	}
	else
		delay(1000);
}

void
reboot(void* v, void* w, int32_t i)
{
	panic("reboot\n");
}

void
exit(int ispanic)
{
	shutdown(ispanic);
	archreset();
}
