#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../ip/ip.h"

#include <pool.h>
#include <tos.h>

#include "qtm.h"

#include "init.h"

#define	MAXCONF		32

ulong dverify = 0x01020304;
ulong bverify;
int securemem;
Conf conf;

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

static uintptr sp;		/* XXX - must go - user stack of init proc */

static char confname[MAXCONF][KNAMELEN];
static char *confval[MAXCONF];
static int nconf;

static void
mypanic(Pool *p, char *fmt, ...)
{
	USED(p, fmt);
	print("malloc panic\n");
	delay(1000);
	splhi();
	for (;;)
		;
}

void
machninit(int machno)
{
	if (machno >= MAXMACH)
		panic("machno %d >= MAXMACH %d", machno, MAXMACH);
	conf.nmach++;
	m = MACHP(machno);
	memset(m, 0, sizeof(Mach));
	m->machno = machno;
	m->cputype = getpvr()>>16;   /* OWN[0:11] and PCF[12:15] fields */
	m->delayloop = 20000;	/* initial estimate only; set by clockinit */
 	m->perf.period = 1;
	/* other values set by archreset */
}

void
cpuidprint(void)
{
	char name[64];

	cputype2name(name, sizeof name);
	print("cpu%d: %ldMHz PowerPC %s\n", m->machno, m->cpuhz/1000000, name);
}

void
mach0init(void)
{
	conf.nmach = 0;

	machninit(0);

	active.machs = 1;
	active.exiting = 0;
}

static void
setconf(char *name, char *val)
{
	strncpy(confname[nconf], name, KNAMELEN);
	kstrdup(&confval[nconf], val);
	nconf++;
}

static void
plan9iniinit(void)
{
	/* virtex configuration */
	setconf("nvram", "/boot/nvram");
	setconf("nvroff", "0");
	setconf("nvrlen", "512");

	setconf("aoeif", "ether0");
	setconf("aoedev", "e!#æ/aoe/1.0");
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

static void
optionsinit(char* s)
{
	char *o;

	o = strecpy(oargb, oargb+sizeof(oargb), s)+1;
	if(getenv("bootargs", o, o - oargb) != nil)
		*(o-1) = ' ';

	oargblen = strlen(oargb);
	oargc = tokenize(oargb, oargv, nelem(oargv)-1);
	oargv[oargc] = nil;
}

void
main(void)
{
	int machno;

	/* entry to main pushed stuff onto the stack. */

//	memset(edata, 0, (ulong)end-(ulong)edata);

	machno = getpir();
	if (machno > 0)
		startcpu(machno);

//	dcrcompile();
	if (dverify != 0x01020304) {
		uartlputs("data segment not initialised\n");
		panic("data segment not initialised");
	}
	if (bverify != 0) {
		uartlputs("bss segment not zeroed\n");
		panic("bss segment not zeroed");
	}
	mach0init();
	archreset();
	quotefmtinstall();
	optionsinit("/boot/boot boot");
//	archconsole();

	meminit();
	confinit();
	mmuinit();
	xinit();			/* xinit would print if it could */
	trapinit();
	qtminit();
	ioinit();
	uncinit();
	printinit();
	uartliteconsole();

	mainmem->flags |= POOL_ANTAGONISM;
	mainmem->panic = mypanic;
	ethermedium.maxtu = 1512;   /* must be multiple of 4 for temac's dma */

//	print("\n\nPlan 9k H\n");	/* already printed by l.s */
	plan9iniinit();
	timersinit();
	clockinit();

	dma0init();			/* does not start kprocs; see init0 */
	fpuinit();
	procinit0();
	initseg();
	links();

	chandevreset();
	okprint = 1;			/* only now can we print */
	barriers();
	dcflush((uintptr)&okprint, sizeof okprint);

	cpuidprint();

	print("%d Hz clock", HZ);
	print("; memory size %,ud (%#ux)\n", (uint)memsz, (uint)memsz);

	pageinit();
	swapinit();
	userinit();
	active.thunderbirdsarego = 1;
	dcflush((uintptr)&active.thunderbirdsarego,
		sizeof active.thunderbirdsarego);
	schedinit();
	/* no return */
	panic("schedinit returned");
}

void
confinit(void)
{
	char *p;
	int i, userpcnt;
	ulong kpages;

	if(p = getconf("*kernelpercent"))
		userpcnt = 100 - strtol(p, 0, 0);
	else
		userpcnt = 85;

	conf.npage = 0;
	for(i=0; i<nelem(conf.mem); i++)
		conf.npage += conf.mem[i].npage;

	/* see machninit for nmach setting */
	conf.nproc = 100 + ((conf.npage*BY2PG)/MB)*5;
	if(conf.nproc > 2000)
		conf.nproc = 2000;
	conf.nimage = 200;
	conf.nswap = conf.nproc*80;
	conf.nswppo = 4096;
	conf.copymode = 0;			/* copy on write */

	if(userpcnt < 10)
		userpcnt = 60;
	kpages = conf.npage - (conf.npage*userpcnt)/100;

	conf.upages = conf.npage - kpages;
	conf.ialloc = (kpages/2)*BY2PG;

	kpages *= BY2PG;
	kpages -= conf.upages*sizeof(Page)
		+ conf.nproc*sizeof(Proc)
		+ conf.nimage*sizeof(Image)
		+ conf.nswap
		+ conf.nswppo*sizeof(Page);
	mainmem->maxsize = kpages;
}

void
init0(void)
{
	int i;
	char buf[2*KNAMELEN];

	assert(up != nil);
	up->nerrlab = 0;
	barriers();
	intrack(~0);
	clrmchk();
	barriers();
	spllo();

	/*
	 * These are o.k. because rootinit is null.
	 * Then early kproc's will have a root and dot.
	 */
	up->slash = namec("#/", Atodir, 0, 0);
	pathclose(up->slash->path);
	up->slash->path = newpath("/");
	up->dot = cclone(up->slash);

	dmainit();			/* starts dma kprocs */
	devtabinit();

	if(!waserror()){
		snprint(buf, sizeof(buf), "power %s", conffile);
		ksetenv("terminal", buf, 0);
		ksetenv("cputype", "power", 0);
		if(cpuserver)
			ksetenv("service", "cpu", 0);
		else
			ksetenv("service", "terminal", 0);

		/* virtex configuration */
		ksetenv("nvram", "/boot/nvram", 0);
		ksetenv("nvroff", "0", 0);
		ksetenv("nvrlen", "512", 0);

		ksetenv("nobootprompt", "tcp", 0);

		poperror();
	}
	for(i = 0; i < nconf; i++){
		if(confval[i] == nil)
			continue;
		if(confname[i][0] != '*'){
			if(!waserror()){
				ksetenv(confname[i], confval[i], 0);
				poperror();
			}
		}
		if(!waserror()){
			ksetenv(confname[i], confval[i], 1);
			poperror();
		}
	}

	kproc("alarm", alarmkproc, 0);
	if (securemem)
	 	kproc("mutate", mutateproc, 0);
	else
		print("no secure memory found\n");

	/*
	 * The initial value of the user stack must be such
	 * that the total used is larger than the maximum size
	 * of the argument list checked in syscall.
	 */
	sync();
	isync();
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
	p = UINT2PTR(STACKALIGN(base + BY2PG - sizeof(up->s.args) - i));
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
	 * caller of initcode.
	 */
	sp = USTKTOP - ssize - sizeof(void*);
}

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
	memmove(UINT2PTR(VA(k)), initcode, sizeof initcode);
	sync();
	kunmap(k);

	ready(p);
}

/*
 * power-saving wait for interrupt when nothing ready.
 * ../port/proc.c should really call this splhi itself
 * to avoid the race avoided here by the call to anyready
 */
void
idlehands(void)
{
	int s, oldbits;

	/* we only use one processor, no matter what */
//	if (conf.nmach > 1)
//		return;
	s = splhi();
	if(!anyready()) {
		oldbits = lightstate(Ledidle);
		putmsr(getmsr() | MSR_WE | MSR_EE | MSR_CE); /* MSR_DE too? */
		lightstate(oldbits);
	}
	splx(s);
}

/*
 *  set mach dependent process state for a new process
 */
void
procsetup(Proc* p)
{
	fpusysprocsetup(p);
}

/*
 *  Save the mach dependent part of the process state.
 */
void
procsave(Proc *p)
{
	fpuprocsave(p);
}

void
shutdown(int ispanic)
{
	int ms, once;

	lock(&active);
	if(ispanic)
		active.ispanic = ispanic;
	else if(m->machno == 0 && (active.machs & (1<<m->machno)) == 0)
		active.ispanic = 0;
	once = active.machs & (1<<m->machno);
	active.machs &= ~(1<<m->machno);
	active.exiting = 1;
	unlock(&active);

	if(once)
		print("cpu%d: exiting\n", m->machno);
	spllo();
	for(ms = 5*1000; ms > 0; ms -= TK2MS(2)){
		delay(TK2MS(2));
		if(active.machs == 0 && consactive() == 0)
			break;
	}

#ifdef notdef
	if(active.ispanic && m->machno == 0){
		if(cpuserver)
			delay(30000);
		else
			for(;;)
				halt();
	}
	else
#endif /* notdef */
		delay(1000);
}

void
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
	memset(BOOTLINE, 0, BOOTLINELEN);	/* zero 9load boot line */
	memmove(BOOTARGS, p, n);
	poperror();
	free(p);
}

void
archreboot(void)
{
	splhi();
	iprint("reboot requested; going into wait state\n");
	for(;;)
		putmsr(getmsr() | MSR_WE);
}

void
exit(int ispanic)
{
	shutdown(ispanic);
	archreboot();
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
	confval[i] = val;
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
dump(void *vaddr, int words)
{
	ulong *addr;

	addr = vaddr;
	while (words-- > 0)
		print("%.8lux%c", *addr++, words % 8 == 0? '\n': ' ');
}
