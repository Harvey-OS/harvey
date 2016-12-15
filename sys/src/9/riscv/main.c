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
#include "io.h"
#include "encoding.h"
#include "ureg.h"
#include <tos.h>


int cpuserver = 1;

extern void (*consuartputs)(char*, int);
void query_mem(const char *config_string, uintptr_t *base, size_t *size);
void query_rtc(const char *config_string, uintptr_t *mtime);
void query_uint(const char *config_string, char *name, uintptr_t *val);

void testPrint(uint8_t c);

void msg(char *s)
{
	while (*s)
		testPrint(*s++);
}
void die(char *s)
{
	msg(s);
	while (1);
}

void
ndnr(void)
{
	die("ndnr");
}

static void puts(char * s, int n)
{
	while (n--)
		testPrint(*s++);
}

/* mach info for hart 0. */
/* in many plan 9 implementations this stuff is all reserved in early assembly.
 * we don't have to do that. */
uint64_t m0stack[4096];
Mach m0;

Sys asys, *sys=&asys;
Conf conf;
uintptr_t kseg0 = KZERO;
char *cputype = "riscv";
int64_t hz;
uintptr_t rtc;

/* I forget where this comes from and I don't care just now. */
uint32_t kerndate;
int maxcores = 1;
int nosmp = 1;
uint64_t mtimepa, mtimecmppa;
uint64_t *mtime, *mtimecmp;
/*
 * kseg2 is the base of the virtual address space.
 * it is not a constant as in amd64; in riscv there are many possible
 * values, even on the same SOC. It is determined by firmware.
 */
void *kseg2;

char *configstring; /* from coreboot, first arg to main */

static uintptr_t sp;		/* XXX - must go - user stack of init proc */

/* general purpose hart startup. We call this via startmach.
 * When we enter here, the machp() function is usable.
 */

void hart(void)
{
	//Mach *mach = machp();
	die("not yet");
}

uint64_t
rdtsc(void)
{
	uint64_t cycles;
//	msg("rdtsc\n");
	cycles = read_csr(/*s*/cycle);
//print("cycles in rdtsc is 0x%llx\n", cycles);
//	msg("done rdts\n");
	return cycles;
}

void
loadenv(int argc, char* argv[])
{
	char *env[2];

	/*
	 * Process command line env options
	 */
	while(--argc > 0){
		char* next = *++argv;
		if(next[0] !='-'){
			if (gettokens(next, env, 2, "=")  == 2){;
				ksetenv(env[0], env[1], 0);
			}else{
				print("Ignoring parameter with no value: %s\n", env[0]);
			}
		}
	}
}

void
init0(void)
{
	Proc *up = externup();
	char buf[2*KNAMELEN];
	Ureg u;

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
print("init0: up is %p\n", up);
	up->slash = namec("#/", Atodir, 0, 0);
print("1\n");
	pathclose(up->slash->path);
print("1\n");
	up->slash->path = newpath("/");
print("1\n");
	up->dot = cclone(up->slash);
print("1\n");

	devtabinit();
print("1\n");

	if(!waserror()){
		//snprint(buf, sizeof(buf), "%s %s", "AMD64", conffile);
		//loadenv(oargc, oargv);
		ksetenv("terminal", buf, 0);
		ksetenv("cputype", cputype, 0);
		ksetenv("pgsz", "2097152", 0);
		// no longer. 	confsetenv();
		poperror();
	}
	kproc("alarm", alarmkproc, 0);
	//nixprepage(-1);
	print("TOUSER: kstack is %p\n", up->kstack);
	//debugtouser((void *)UTZERO);
	memset(&u, 0, sizeof(u));
	u.ip = (uintptr_t)init_main;
	u.sp = sp;
	u.a2 = USTKTOP-sizeof(Tos);
	touser(&u);
}

/*
 * Option arguments from the command line.
 * oargv[0] is the boot file.
 * TODO: do it.
 */
static int64_t oargc;
static char* oargv[20];
static char oargb[1024];
static int oargblen;

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
	 * args by subtracting sizeof(up->arg).
	 */
	i = oargblen+1;
	p = UINT2PTR(STACKALIGN(base + BIGPGSZ - sizeof(((Proc*)0)->arg) - i));
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
	ssize = base + BIGPGSZ - PTR2UINT(av);
	print("Stack size in boot args is %p\n", ssize);
	*av++ = (char*)oargc;
	for(i = 0; i < oargc; i++)
		*av++ = (oargv[i] - oargb) + (p - base) + (USTKTOP - BIGPGSZ);
	*av = nil;

	sp = USTKTOP - ssize;
	print("New sp in bootargs is %p\n", sp);
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
	p->sched.sp = PTR2UINT(p->kstack+KSTACK-sizeof(up->arg)-sizeof(uintptr_t));
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
	print("newseg(0x%x, %p, 0x%llx)\n", SG_STACK|SG_READ|SG_WRITE, (void *)USTKTOP-USTKSIZE, USTKSIZE/ BIGPGSZ);
	s = newseg(SG_STACK|SG_READ|SG_WRITE, USTKTOP-USTKSIZE, USTKSIZE/ BIGPGSZ);
	p->seg[sno++] = s;
	pg = newpage(1, 0, USTKTOP-BIGPGSZ, BIGPGSZ, -1);
	segpage(s, pg);
	k = kmap(pg);
	bootargs(VA(k));
	kunmap(k);

	/*
	 * Text
	 */
	s = newseg(SG_TEXT|SG_READ|SG_EXEC, UTZERO, 1);
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
	s = newseg(SG_DATA|SG_READ|SG_WRITE, UTZERO + BIGPGSZ, 1);
	s->flushme++;
	p->seg[sno++] = s;
	pg = newpage(1, 0, UTZERO + BIGPGSZ, BIGPGSZ, -1);
	memset(pg->cachectl, PG_TXTFLUSH, sizeof(pg->cachectl));
	segpage(s, pg);
	k = kmap(s->map[0]->pages[0]);
	/* This depends on init having a text segment < 2M. */
	memmove(UINT2PTR(VA(k) + init_data_start - (UTZERO + BIGPGSZ)), init_data_out, sizeof(init_data_out));

	kunmap(k);
	ready(p);
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

/* check checks simple atomics and anything else that is critical to correct operation.
 * You can make the prints optional on errors cases, not have it print all tests,
 * but you should never remove check or the call to it. It found some nasty problems. */
static void
check(void)
{
	uint64_t f2ns;
	// cas test
	uint32_t t = 0;
	int fail = 0;
	int _42 = 42;
	int a = _42, b;
	int x = cas32(&t, 0, 1);

	print("cas32 done x %d (want 1) t %d (want 1)\n", x, t);
	if ((t != 1) || (x != 1))
		fail++;

	x = cas32(&t, 0, 1);
	print("cas32 done x %d (want 0) t %d (want 1)\n", x, t);
	if ((t != 1) || (x != 0))
		fail++;

	print("t is now %d before final cas32\n", t);
	x = cas32(&t, 1, 2);
	print("cas32 done x %d (want 1) t %d (want 2)\n", x, t);
	if ((t != 2) || (x != 1))
		fail++;

	t = 0;
	x = tas32(&t);
	print("tas done x %d (want 0) t %d (want 1)\n", x, t);
	if ((t != 1) || (x != 0))
		fail++;

	x = tas32(&t);
	print("tas done x %d (want 1) t %d (want 1)\n", x, t);
	if ((t != 1) || (x != 1))
		fail++;

	t = 0;
	x = tas32(&t);
	print("tas done x %d (want ) t %d (want 1)\n", x, t);
	if ((t != 1) || (x != 0))
		fail++;

	b = ainc(&a);
	print("after ainc a is %d (want 43) b is %d (want 43)\n", a, b);
	if ((b != _42 + 1) || (a != _42 + 1))
		fail++;

	b = ainc(&a);
	print("after ainc a is %d (want 44) b is %d (want 44)\n", a, b);
	if ((b != _42 + 2) || (a != _42 + 2))
		fail++;

	b = adec(&a);
	print("after ainc a is %d (want 43) b is %d (want 43)\n", a, b);
	if ((b != _42 + 1) || (a != _42 + 1))
		fail++;

	if (fail) {
		print("%d failures in check();\n", fail);
		panic("FIX ME");
	}

	f2ns = fastticks2ns(10);
	if ((f2ns < 1) || (f2ns > 10)) {
		print("fastticks2ns(1) is nuts: %d\n", f2ns);
		panic("Should be in the range 1 to 10, realistically");
	}

	f2ns = ns2fastticks(1);
	if ((f2ns < 2) || (f2ns > 100)) {
		print("ns2fastticks(1) is nuts: %d\n", f2ns);
		panic("Should be in the range 2 to 100, realistically");
	}
		
}
	
void bsp(void *stack, uintptr_t _configstring)
{
	kseg2 = findKSeg2();
	msg("HIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII\n");
	configstring = KADDR(_configstring);
	msg(configstring);
	Mach *mach = machp();
	if (mach != &m0)
		die("MACH NOT MATCH");
	msg("memset mach\n");
	memset(mach, 0, sizeof(Mach));
	msg("done that\n");
	MACHP(0) = mach;

	msg(configstring);
	mach->self = (uintptr_t)mach;
	msg("SET SELF OK\n");
	mach->machno = 0;
	mach->online = 1;
	mach->NIX.nixtype = NIXTC;
	mach->stack = PTR2UINT(stack);
	*(uintptr_t*)mach->stack = STACKGUARD;
	msg(configstring);
	mach->externup = nil;
	active.nonline = 1;
	active.exiting = 0;
	active.nbooting = 0;

	consuartputs = puts;
	msg("call asminit\n");
	msg("==============================================\n");
	asminit();
	msg(",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,\n");
	asmmapinit(0x81000000, 0x3f000000, 1);

	msg(configstring);
	/*
	 * Need something for initial delays
	 * until a timebase is worked out.
	 */
	mach->cpuhz = 2000000000ll;
	mach->cpumhz = 2000;
	sys->cyclefreq = mach->cpuhz;
	
	sys->nmach = 1;

	msg(configstring);
	fmtinit();
	print("\nHarvey\n");
print("KADDR OF (uintptr_t) 0x40001000 is %p\n", KADDR((uintptr_t) 0x40001000));

	/* you're going to love this. Print does not print the whole
	 * string. msg does. Bug. */
	print("Config string:%p '%s'\n", configstring, configstring);
	msg("Config string via msg\n");
	msg(configstring);
	msg("\n");
	mach->perf.period = 1;
	if((hz = archhz()) != 0ll){
		mach->cpuhz = hz;
		mach->cyclefreq = hz;
		sys->cyclefreq = hz;
		mach->cpumhz = hz/1000000ll;
	}

	print("print a number like 5 %d\n", 5);
	/*
	 * Mmuinit before meminit because it
	 * flushes the TLB via machp()->pml4->pa.
	 */
	mmuinit();

	ioinit(); print("ioinit\n");
print("IOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIO\n");
	meminit();print("meminit\n");
print("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\n");
	confinit();print("confinit\n");
print("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\n");
	archinit();print("archinit\n");
print("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
	mallocinit();print("mallocinit\n");

	/* test malloc. It's easier to find out it's broken here,
	 * not deep in some call chain.
	 * See next note.
	 *
	 */
	if (1) {
		void *v = malloc(1234);
		msg("allocated\n ");
		free(v);
		msg("free ok\n");
	}

	query_rtc(configstring, &rtc);
	print("rtc: %p\n", rtc);

	query_uint(configstring, "rtc{addr", (uintptr_t*)&mtimepa);
	mtime = KADDR(mtimepa);
	query_uint(configstring, "core{0{0{timecmp", (uintptr_t*)&mtimecmppa);
	mtimecmp = KADDR(mtimecmppa);

	print("mtime is %p and mtimecmp is %p\n", mtime, mtimecmp);
	umeminit();

	procinit0();
	print("before mpacpi, maxcores %d\n", maxcores);
	trapinit();
	print("trapinit done\n");
	/* Forcing to single core if desired */
	if(!nosmp) {
		// smp startup
	}
//working.
	// not needed. teardownidmap(mach);
	timersinit();print("	timersinit();\n");
	// ? fpuinit();
	psinit(conf.nproc);print("	psinit(conf.nproc);\n");
	initimage();print("	initimage();\n");
	links();

	devtabreset();print("	devtabreset();\n");
	pageinit();print("	pageinit();\n");
	swapinit();print("	swapinit();\n");
	userinit();print("	userinit();\n");
	/* Forcing to single core if desired */
	if(!nosmp) {
		//nixsquids();
		//testiccs();
	}

	print("NO profiling until you set upa alloc_cpu_buffers()\n");
	//alloc_cpu_buffers();

	print("CPU Freq. %dMHz\n", mach->cpumhz);
	// set the trap vector
	void *supervisor_trap_entry(void);
	write_csr(/*stvec*/0x105, supervisor_trap_entry);
	// enable all interrupt sources.
	uint64_t ints = read_csr(sie);
	ints |= 0x666;
	write_csr(sie, ints);

	dumpmmuwalk(0xfffffffffffff000ULL);

	check();

	print("schedinit...\n");

	schedinit();
	die("Completed hart for bsp OK!\n");
}

/* stubs until we implement in assembly */
int corecolor(int _)
{
	return -1;
}

Proc *externup(void)
{
	if (! machp())
		return nil;
	return machp()->externup;
}

void errstr(char *s, int i) {
	panic("errstr");
}

void
oprof_alarm_handler(Ureg *u)
{
	//print((char *)__func__);
	//print("IGNORING\n");
}

void
hardhalt(void)
{
	panic((char *)__func__);
}

void
ureg2gdb(Ureg *u, uintptr_t *g)
{
	panic((char *)__func__);
}

int
userureg(Ureg*u)
{
	int64_t ip = (int64_t)u->ip;
	if (ip < 0) {
		//print("RETURNING 0 for userureg\n");
		return 0;
	}
	//print("Returning 1 for userureg; need a better test\n");
	return 1;
}

void    exit(int _)
{
	panic((char *)__func__);
}

void fpunoted(void)
{
	panic((char *)__func__);
}

void fpunotify(Ureg*_)
{
	print("fpunotify: doing nothing since FPU is disabled\n");
}

void fpusysrfork(Ureg*_)
{
	print((char *)__func__);
	print("IGNORING\n");
}

void sysrforkret(void)
{
	void *stack(void);
	void *sp = stack();
	print("sysrforkret: stack is %p\n", sp);
	dumpgpr((Ureg *)sp);
void _sysrforkret();
	_sysrforkret();
}

void
reboot(void*_, void*__, int32_t ___)
{
	panic("reboot");
}

void fpusysprocsetup(Proc *_)
{
	print((char *)__func__);
	print("THIS IS GONNA SCREW YOU IF YOU DO NOT FIX IT\n");
}

void     fpusysrforkchild(Proc*_, Proc*__)
{
	print((char *)__func__);
	print("THIS IS GONNA SCREW YOU IF YOU DO NOT FIX IT\n");
}

int
fpudevprocio(Proc*p, void*v, int32_t _, uintptr_t __, int ___)
{
	panic((char *)__func__);
	return -1;
}

void cycles(uint64_t *p)
{
	return;
	*p = rdtsc();
}

int islo(void)
{
//	msg("isloc\n");
	uint64_t ms = read_csr(sstatus);
//	msg("read it\n");
	return ms & MSTATUS_SIE;
}

void
stacksnippet(void)
{
	//Stackframe *stkfr;
	kmprint(" stack:");
//	for(stkfr = stackframe(); stkfr != nil; stkfr = stkfr->next)
//		kmprint(" %c:%p", ktextaddr(stkfr->pc) ? 'k' : '?', ktextaddr(stkfr->pc) ? (stkfr->pc & 0xfffffff) : stkfr->pc);
	kmprint("\n");
}


/* crap. */
/* this should come from build but it's intimately tied in to VGA. Crap. */
Physseg physseg[8];
int nphysseg = 8;

/* bringup -- remove asap. */
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
void wave(int c)
{
	testPrint(c);
}

void hi(char *s)
{
	if (! s)
		s = "<NULL>";
	while (*s)
		wave(*s++);
}

