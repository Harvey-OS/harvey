/*
 * #P/cputype, shutdown and timing.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "riscv.h"

#include <ctype.h>

#define RDTIMES(clint, tsc, mtime) \
	coherence(); \
	mtime = RDCLTIME(clint);	/* ticks */ \
	tsc = rdtsc()			/* cycles */

enum {
	Qdir = 0,
	Qbase,

	Qmax = 16,

	Debug = 0,
	Newsbi = 0,	/* flag: print sbi extensions (on a new machine) */
};

typedef long Rdwrfn(Chan*, void*, long, vlong);

static Rdwrfn *readfn[Qmax];
static Rdwrfn *writefn[Qmax];

static Dirtab archdir[Qmax] = {
	".",		{ Qdir, 0, QTDIR },	0,	0555,
};
Lock archwlock;	/* the lock is only for changing archdir */
int narchdir = Qbase;

/* from kernel config */
extern uvlong cpuhz;

extern Dev archdevtab;

/*
 * Add a file to the #P listing.  Once added, you can't delete it.
 * You can't add a file with the same name as one already there,
 * and you get a pointer to the Dirtab entry so you can do things
 * like change the Qid version.  Changing the Qid path is disallowed.
 */
Dirtab*
addarchfile(char *name, int perm, Rdwrfn *rdfn, Rdwrfn *wrfn)
{
	int i;
	Dirtab d;
	Dirtab *dp;

	memset(&d, 0, sizeof d);
	strcpy(d.name, name);
	d.perm = perm;

	lock(&archwlock);
	if(narchdir >= Qmax){
		unlock(&archwlock);
		return nil;
	}

	for(i=0; i<narchdir; i++)
		if(strcmp(archdir[i].name, name) == 0){
			unlock(&archwlock);
			return nil;
		}

	d.qid.path = narchdir;
	archdir[narchdir] = d;
	readfn[narchdir] = rdfn;
	writefn[narchdir] = wrfn;
	dp = &archdir[narchdir++];
	unlock(&archwlock);
	return dp;
}

static Chan*
archattach(char* spec)
{
	return devattach(archdevtab.dc, spec);
}

Walkqid*
archwalk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, archdir, narchdir, devgen);
}

static long
archstat(Chan* c, uchar* dp, long n)
{
	return devstat(c, dp, n, archdir, narchdir, devgen);
}

static Chan*
archopen(Chan* c, int omode)
{
	return devopen(c, omode, archdir, narchdir, devgen);
}

static void
archclose(Chan*)
{
}

enum
{
	Linelen= 31,
};

static long
archread(Chan *c, void *a, long n, vlong offset)
{
	Rdwrfn *fn;

	switch((ulong)c->qid.path){

	case Qdir:
		return devdirread(c, a, n, archdir, narchdir, devgen);
	default:
		if(c->qid.path < narchdir && (fn = readfn[c->qid.path]))
			return fn(c, a, n, offset);
		error(Eperm);
		notreached();
	}
}

static long
archwrite(Chan *c, void *a, long n, vlong offset)
{
	Rdwrfn *fn;

	switch((ulong)c->qid.path){
	default:
		if(c->qid.path < narchdir && (fn = writefn[c->qid.path]))
			return fn(c, a, n, offset);
		error(Eperm);
		notreached();
	}
}

Dev archdevtab = {
	'P',
	"arch",

	devreset,
	devinit,
	devshutdown,
	archattach,
	archwalk,
	archstat,
	archopen,
	devcreate,
	archclose,
	archread,
	devbread,
	archwrite,
	devbwrite,
	devremove,
	devwstat,
};

void
nop(void)
{
}

static long
cputyperead(Chan*, void *a, long n, vlong off)
{
	char str[32];

	snprint(str, sizeof(str), "%s %ud\n", cputype, m->cpumhz);
	return readstr(off, a, n, str);
}

static ulong exts[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 0x10,
	0x735049, 0x52464e43, HSM, 0x53525354,
};

static void
sbiext(int ext)
{
	char c;

	if (sbiprobeext(ext) != 0) {
		print(" %#ux", ext);
		if (ext >= 256) {
			print(" (");
			for (; ext > 0; ext <<= 8) {
				c = ((uchar *)&ext)[3];
				if (c && isprint(c))
					print("%c", c);
			}
			print(")");
		}
	}
}

static void
sbiinit(void)
{
	int i;

	if (bootmachmode)
		/* reads back as 0x222 Superie on tinyemu */
		print("mideleg %#ux\n", mideleg); /* varies with implement'n */
	else if (!nosbi) {
		havesbihsm = sbiprobeext(HSM) != 0;
		if (!Newsbi)
			return;
		print("sbi extensions present:");
		for (i = 0; i < nelem(exts); i++)
			sbiext(exts[i]);
		print("\n");
	}
}

void
archinit(void)
{
	addarchfile("cputype", 0444, cputyperead, nil);
	sbiinit();
}

void
archreset(void)
{
	Htif *htif = (Htif *)htifpwrregs;	/* tinyemu only */

	cacheflush();
	spllo();
	if (htif) {
		iprint("htif resetting.\n");
		htif->tohost[0] = 1;
		htif->tohost[1] = 0;		/* cmd 1: power off */
	}
	delay(1);
	if (rvreset)
		rvreset();
	if (!nosbi && m->machno == 0) {
		iprint("sbi shutting down.\n");
		sbishutdown();			/* shuts down all cpus */
	}
	iprint("last resort: into wfi.\n");
	for (;;)
		halt();
}

static void
haltsys(void)
{
	int i;

	iprint("mpshutdown: %d active cpus\n", sys->nonline);
	delay(5);
	devtabshutdown();
	splhi();

	/* paranoia: wait for other cpus */
	for (i = 100; i > 0 && sys->nonline > 1; i--) {
		iprint(" %d", sys->nonline);
		delay(200);
	}

	/* we can now use the uniprocessor reset */
	archreset();
	notreached();
}

/*
 * secondary cpus should all idle (here or in shutdown, called from reboot).
 * for a true shutdown, reset and boot, cpu 0 should then reset itself or the
 * system as a whole.  if plan 9 is rebooting (loading a new kernel and
 * starting it), cpu 0 needs to keep running to do that work, so we return
 * in that case only.
 *
 * we assume shutdown has previously been called on this cpu.
 */
void
mpshutdown(void)
{
	void (*tramp)(Reboot *);

	if(m->machno == 0) {
		static QLock mpshutdownlock;

		qlock(&mpshutdownlock);
		/* we are the first proc on cpu0 to begin shutdown */
		if(!active.rebooting) {
			haltsys();
			notreached();
		}
	} else {
		/*
		 * we're secondary, we should go into sys->reboottramp to enter
		 * wfi.  if idling, sys->Reboot won't be set and that's okay.
		 */
		cacheflush();
		mmulowidmap();			/* disables user mapping too */
		m->virtdevaddrs = 0;
		/* await populated sys->Reboot */
		/* secstall is cleared by schedcpus, settrampargs from reboot */
		while (sys->secstall == RBFLAGSET)
			coherence();
		tramp = (void (*)(Reboot *))PADDR(sys->reboottramp);
		(*tramp)(&sys->Reboot);
		notreached();
	}
}

/*
 * there are two main timers: the system-wide clint clock (mtime)
 * and the per-core cpu clock (rdtsc).  The clint clock is typically
 * something like 1 or 10 MHz and the cpu clock 600 - 1200 MHz.  
 */

/*
 *  return value and frequency of timer (via hz)
 */
uvlong
fastticks(uvlong* hz)
{
	uvlong clticks;

	if(hz != nil)
		*hz = timebase;
	coherence();
	clticks = RDCLTIME(clintaddr());
	if (sysepoch == 0)
		return clticks;
	if (clticks >= sysepoch)
		clticks -= sysepoch;		/* clticks since boot */
	else
		iprint("fastticks: cpu%d: clticks %,lld < sysepoch %,lld\n",
			m->machno, clticks, sysepoch);
	return clticks;
}

ulong
µs(void)
{
	return fastticks2us(fastticks(nil));
}

void
setclinttmr(Clint *clnt, uvlong clticks)
{
	if (nosbi) {
		/* seems not to work under OpenSBI, though undocumented */
		WRCLTIMECMP(clnt, clticks);
		coherence();	/* Stip might not be extinguished immediately */
	} else
		sbisettimer(clticks);	/* how long does this take? */
}

static uvlong tscperclintglob;

/* are cycles for an interval consistent with clint ticks? */
static int
areclockswonky(uvlong difftsc, uvlong diffclint)
{
	vlong difftscperclint;

	if (diffclint == 0)
		diffclint = 1;
	difftscperclint = difftsc / diffclint;
	if (difftscperclint < tscperclintglob*2/3 ||
	    difftscperclint > tscperclintglob*3) {
		/* can't trust clocks to pop us out of wfi soon */
		idlepause = 1;
		if (Debug)
			print("clocks are wonky or emulated; using PAUSE to idle.\n");
		return 1;
	}
	return 0;
}

static void
timeop(Clint *clint, char *name, void (*op)(Clint *))
{
	int i;
	uvlong tsc, mtime, tsc2, mtime2, diffsumtsc, diffsumclint;

	RDTIMES(clint, tsc, mtime);
	for (i = 0; i < 1000; i++)
		op(clint);
	RDTIMES(clint, tsc2, mtime2);

	diffsumtsc = tsc2 - tsc;
	diffsumclint = mtime2 - mtime;

	if (Debug)
		print("op %s:\t%d took %,lld tsc cycles and %,lld clint ticks\n",
			name, i, diffsumtsc, diffsumclint);
	areclockswonky(diffsumtsc, diffsumclint);
}

static void
nullop(Clint *)
{
}

static void
rdtimeop(Clint *clint)
{
	vlong junk;

	junk = RDCLTIME(clint);
	USED(junk);
}

static void
wrtimeop(Clint *clint)
{
	WRCLTIME(clint, RDCLTIME(clint));
}

static void
wrtimecmpop(Clint *clint)
{
	WRCLTIMECMP(clint, VMASK(63));
}

vlong
vlabs(vlong vl)
{
	return vl >= 0? vl: -vl;
}

/*
 * compute timing parameters, turn off WFI use if clocks are inconsistent.
 */
void
calibrate(void)
{
	uvlong tsc, mtime, tsc2, mtime2, tscperclint;
	Clint *clint;

	if (sys->tscperclint)
		return;
	clint = clintaddr();
	RDTIMES(clint, tsc, mtime);
	delay(100);
	RDTIMES(clint, tsc2, mtime2);

	tscperclint = 1;
	if (mtime2 > mtime) {
		/*
		 * no need to round: a lower tscperclint will cause larger
		 * tick delays in timerset, which should be harmless.
		 */
		tscperclint = (tsc2 - tsc) / (mtime2 - mtime);
		if ((vlong)tscperclint < 1)
			tscperclint = 1;
	}
	tscperclintglob = tscperclint;
	mtime2 -= mtime;
	tsc2 -= tsc;
	if (Debug)
		print("%,llud clint ticks is %,llud rdtsc cycs, or %lld cycs/clint\n",
			mtime2, tsc2, tscperclint);
	areclockswonky(tsc2, mtime2);

	print("timebase given as %,llud Hz\n", timebase);

	/* compute constants for use by timerset & idle code */
	sys->clintsperhz = timebase / HZ;	/* clint ticks per HZ */
	sys->clintsperµs = timebase / 1000000;
	/*
	/* min. interval until intr; was /(100*HZ) but made too many intrs.
	 * Does minclints need to be less than timebase/HZ?  It allows
	 * shorter and more precise sleep intervals, e.g., for clock0link
	 * polling.  To keep the interrupt load and interactive response
	 * manageable, it needs to be somewhat > 0.
	 */
	sys->minclints = timebase / (10*HZ);
	sys->tscperclint = tscperclint;
	sys->tscperhz = tscperclint * sys->clintsperhz;
	sys->tscperminclints = tscperclint * sys->minclints;

	/* time various clock-related operations.  primarily debugging. */
	timeop(clint, "null", nullop);
	timeop(clint, "read time", rdtimeop);
	timeop(clint, "set time", wrtimeop);
	timeop(clint, "set timecmp", wrtimecmpop);
}

void
clockoff(Clint *clint)
{
	putsie(getsie() & ~Stie);
	/* ~0ull makes sense, but looks negative on some machines */
	setclinttmr(clint, VMASK(63));
	clrstip();
}

/*
 *  set next timer interrupt for time next, in cycles (fastticks).
 *  we won't go longer than 1/HZ s. without a clock interrupt.
 *  as a special case, next==0 requests a small interval (e.g., 1/HZ s.).
 */
void
timerset(uvlong next)
{
	Mpl pl;
	vlong cycs, clticks;
	Clint *clint;

	pl = splhi();
	if (sys->tscperclint == 0)
		panic("timerset: sys->tscperclint not yet set");
	clticks = sys->clintsperhz;
	if (next != 0) {
		cycs = next - fastticks(nil);
		/* enforce sane upper bound; head off division */
		if (cycs > 0 && cycs < sys->tscperhz)
			if (cycs <= sys->tscperminclints)
				/* don't interrupt immediately */
				clticks = sys->minclints;
			else
				clticks = (cycs + sys->tscperclint - 1) /
					sys->tscperclint;
	}

	clint = clintaddr();
	clockoff(clint);		/* don't trigger early */
	setclinttmr(clint, RDCLTIME(clint) + clticks);
	clockenable();
	splx(pl);
}
