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

extern void (*consuartputs)(char*, int);

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

static int x = 0x123456;

/* mach struct for hart 0. */
/* in many plan 9 implementations this stuff is all reserved in early assembly.
 * we don't have to do that. */
static uint64_t m0stack[4096];
static Mach m0;
Sys asys, *sys=&asys;
Conf conf;
uintptr_t kseg0 = KZERO;
char *cputype = "riscv";

/* I forget where this comes from and I don't care just now. */
uint32_t kerndate;


/* general purpose hart startup. We call this via startmach.
 * When we enter here, the machp() function is usable.
 */

void hart(void)
{
	//Mach *mach = machp();
	die("not yet");
}

void bsp(void)
{
	Mach *mach = machp();
	if (mach != &m0)
		die("MACH NOT MATCH");
	msg("memset mach\n");
	memset(mach, 0, sizeof(Mach));
	msg("done that\n");

	mach->self = (uintptr_t)mach;
	msg("SET SELF OK\n");
	mach->machno = 0;
	mach->online = 1;
	mach->NIX.nixtype = NIXTC;
	mach->stack = PTR2UINT(m0stack);
	*(uintptr_t*)mach->stack = STACKGUARD;
	mach->externup = nil;
	active.nonline = 1;
	active.exiting = 0;
	active.nbooting = 0;

	/*
	 * Need something for initial delays
	 * until a timebase is worked out.
	 */
	mach->cpuhz = 2000000000ll;
	mach->cpumhz = 2000;
	sys->cyclefreq = mach->cpuhz;

	// this is in 386, so ... not yet. i8250console("0");
	// probably pull in the one from coreboot for riscv.

	consuartputs = puts;

	die("Completed hart for bsp OK!\n");
}

void
main(uint32_t mbmagic, uint32_t mbaddress)
{

	testPrint('0');
	if (x != 0x123456)
		die("Data is not set up correctly\n");
	//memset(edata, 0, end - edata);
	msg("got somewhere");
	startmach(bsp, &m0);
}

/* stubs until we implement in assembly */
int corecolor(int _)
{
	return -1;
}

Proc *externup(void)
{
	return machp()->externup;
}

void errstr(char *s, int i) {
	panic("errstr");
}

void
oprof_alarm_handler(Ureg *u)
{
	panic((char *)__func__);
}

void
hardhalt(void)
{
	panic((char *)__func__);
}

uintmem
physalloc(uint64_t _, int*__, void*___)
{
	panic((char *)__func__);
	return 0;
}

void
ureg2gdb(Ureg *u, uintptr_t *g)
{
	panic((char *)__func__);
}

int
userureg(Ureg*u)
{
	panic((char *)__func__);
	return -1;
}

uintptr_t
userpc(Ureg*u)
{
	panic((char *)__func__);
	return 0;
}


int tas32(void *_)
{
	panic("tas32");
	return -1;
}
int      cas32(void*_, uint32_t __, uint32_t ___)
{
	panic((char *)__func__);
	return -1;
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
	panic((char *)__func__);
}

void fpusysrfork(Ureg*_)
{
	panic((char *)__func__);
}

void kexit(Ureg*_)
{
	panic((char *)__func__);
}

char*
seprintphysstats(char*_, char*__)
{
	return "NOT YET";
}

void
reboot(void*_, void*__, int32_t ___)
{
	panic("reboot");
}

void fpusysprocsetup(Proc *_)
{
	panic((char *)__func__);
}

void sysrforkret(void)
{
	panic((char *)__func__);
}

void     fpusysrforkchild(Proc*_, Proc*__)
{
	panic((char *)__func__);
}

int
fpudevprocio(Proc*p, void*v, int32_t _, uintptr_t __, int ___)
{
	panic((char *)__func__);
	return -1;
}

void
setregisters(Ureg*u, char*f, char*t, int amt)
{
	panic((char *)__func__);
}

uint64_t rdtsc(void)
{
	panic((char *)__func__);
	return 0;
}

int islo(void)
{
	panic((char *)__func__);
	return 0;
}

void mfence(void)
{
	panic((char *)__func__);
}

uintptr_t
dbgpc(Proc*p)
{
	panic((char *)__func__);
	return 0;
}


void dumpstack(void)
{
	panic((char *)__func__);
}

void
dumpgpr(Ureg* ureg)
{
	panic((char *)__func__);
}

void
setkernur(Ureg*u, Proc*p)
{
	panic((char *)__func__);
}


void
physfree(uintmem data, uint64_t size)
{
	panic("physfree %p 0x%lx", data, size);
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

