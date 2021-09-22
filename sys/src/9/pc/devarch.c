/*
 * support for the IBM PC architecture, notably for uniprocessors.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "mp.h"
#include "../port/error.h"

typedef struct IOMap IOMap;
struct IOMap
{
	IOMap	*next;
	int	reserved;
	char	tag[13];
	ulong	start;
	ulong	end;
};

static struct
{
	Lock;
	IOMap	*m;
	IOMap	*free;
	IOMap	maps[32];	/* some initial free maps */

	QLock	ql;		/* lock for reading map */
} iomap;

enum {
	Qdir = 0,
	Qioalloc = 1,
	Qiob,
	Qiow,
	Qiol,
	Qbase,

	Qmax = 16,
};

enum {
	Ppcireset =	0xcf9,	/* pci reset i/o port */
};

enum {				/* cpuid standard function codes */
	Highstdfunc = 0,	/* also returns vendor string */
	Procsig,
	Proctlbcache,
	Procserial,
	/* check Highstdfunc to see if these are allowed */
	Procmon = 5,		/* monitor/mwait capabilities */
	Procid7 = 7,

	Hypercpuid  = 0x40000000,	/* hypervisor's cpu id */

	Highextfunc = 0x80000000,
	Unused1,
	Brand0,
	Brand1,
	Brand2,
};

typedef long Rdwrfn(Chan*, void*, long, vlong);

static Rdwrfn *readfn[Qmax];
static Rdwrfn *writefn[Qmax];

static Dirtab archdir[Qmax] = {
	".",		{ Qdir, 0, QTDIR },	0,	0555,
	"ioalloc",	{ Qioalloc, 0 },	0,	0444,
	"iob",		{ Qiob, 0 },		0,	0660,
	"iow",		{ Qiow, 0 },		0,	0660,
	"iol",		{ Qiol, 0 },		0,	0660,
};
Lock archwlock;	/* the lock is only for changing archdir */
int narchdir = Qbase;

static char *monitorme;
static long dummymonitor;
static long *monitorwd = &dummymonitor;		/* ensure safe deref. always */

static int doi8253set = 1;

static char	*hypername(void);

/* complain, unless it's a known bug in this hypervisor */
void
vmbotch(ulong vmbit, char *cause)
{
	if (conf.vm & vmbit)
		return;
	if (conf.vm & Othervm)
		print("are you running %s, or is %s?\n", hypername(), cause);
	else
		print("%s\n", cause);
}

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

void
ioinit(void)
{
	char *excluded;
	int i;

	for(i = 0; i < nelem(iomap.maps)-1; i++)
		iomap.maps[i].next = &iomap.maps[i+1];
	iomap.maps[i].next = nil;
	iomap.free = iomap.maps;

	/*
	 * This is necessary to make the IBM X20 boot.
	 * Have not tracked down the reason.
	 * i82557 is at 0x1000, the dummy entry is needed for swappable devs.
	 */
	ioalloc(0x0fff, 1, 0, "dummy");

	if ((excluded = getconf("ioexclude")) != nil) {
		char *s;

		s = excluded;
		while (s && *s != '\0' && *s != '\n') {
			char *ends;
			int io_s, io_e;

			io_s = (int)strtol(s, &ends, 0);
			if (ends == nil || ends == s || *ends != '-') {
				print("ioinit: cannot parse option string\n");
				break;
			}
			s = ++ends;

			io_e = (int)strtol(s, &ends, 0);
			if (ends && *ends == ',')
				*ends++ = '\0';
			s = ends;

			ioalloc(io_s, io_e - io_s + 1, 0, "pre-allocated");
		}
	}

}

/*
 * Reserve a range to be ioalloced later.
 * This is in particular useful for exchangable cards, such
 * as pcmcia and cardbus cards (which we no longer support).
 */
int
ioreserve(int, int size, int align, char *tag)
{
	IOMap *m, **l;
	int i, port;

	lock(&iomap);
	/* find a free port above 0x400 and below 0x1000 */
	port = 0x400;
	for(l = &iomap.m; *l; l = &(*l)->next){
		m = *l;
		if (m->start < 0x400) continue;
		i = m->start - port;
		if(i > size)
			break;
		if(align > 0)
			port = ((port+align-1)/align)*align;
		else
			port = m->end;
	}
	if(*l == nil){
		unlock(&iomap);
		return -1;
	}
	m = iomap.free;
	if(m == nil){
		print("ioalloc: out of maps");
		unlock(&iomap);
		return port;
	}
	iomap.free = m->next;
	m->next = *l;
	m->start = port;
	m->end = port + size;
	m->reserved = 1;
	strncpy(m->tag, tag, sizeof(m->tag));
	m->tag[sizeof(m->tag)-1] = 0;
	*l = m;

	archdir[0].qid.vers++;

	unlock(&iomap);
	return m->start;
}

/*
 *	alloc some io port space and remember who it was
 *	alloced to.  if port < 0, find a free region.
 */
int
ioalloc(int port, int size, int align, char *tag)
{
	IOMap *m, **l;
	int i;

	lock(&iomap);
	if(port < 0){
		/* find a free port above 0x400 and below 0x1000 */
		port = 0x400;
		for(l = &iomap.m; *l; l = &(*l)->next){
			m = *l;
			if (m->start < 0x400) continue;
			i = m->start - port;
			if(i > size)
				break;
			if(align > 0)
				port = ((port+align-1)/align)*align;
			else
				port = m->end;
		}
		if(*l == nil){
			unlock(&iomap);
			return -1;
		}
	} else {
		/* Only 64KB I/O space on the x86. */
		if((port+size) > 0x10000){
			unlock(&iomap);
			return -1;
		}
		/* see if the space clashes with previously allocated ports */
		for(l = &iomap.m; *l; l = &(*l)->next){
			m = *l;
			if(m->end <= port)
				continue;
			if(m->reserved && m->start == port && m->end == port + size) {
				m->reserved = 0;
				unlock(&iomap);
				return m->start;
			}
			if(m->start >= port+size)
				break;
			unlock(&iomap);
			return -1;
		}
	}
	m = iomap.free;
	if(m == nil){
		print("ioalloc: out of maps");
		unlock(&iomap);
		return port;
	}
	iomap.free = m->next;
	m->next = *l;
	m->start = port;
	m->end = port + size;
	strncpy(m->tag, tag, sizeof(m->tag));
	m->tag[sizeof(m->tag)-1] = 0;
	*l = m;

	archdir[0].qid.vers++;

	unlock(&iomap);
	return m->start;
}

void
iofree(int port)
{
	IOMap *m, **l;

	lock(&iomap);
	for(l = &iomap.m; *l; l = &(*l)->next){
		if((*l)->start == port){
			m = *l;
			*l = m->next;
			m->next = iomap.free;
			iomap.free = m;
			break;
		}
		if((*l)->start > port)
			break;
	}
	archdir[0].qid.vers++;
	unlock(&iomap);
}

int
iounused(int start, int end)
{
	IOMap *m;

	for(m = iomap.m; m; m = m->next){
		if(start >= m->start && start < m->end
		|| start <= m->start && end > m->start)
			return 0;
	}
	return 1;
}

static void
checkport(int start, int end)
{
	/* standard vga regs are OK */
	if(start >= 0x2b0 && end <= 0x2df+1)
		return;
	if(start >= 0x3c0 && end <= 0x3da+1)
		return;

	if(iounused(start, end))
		return;
	error(Eperm);
}

static Chan*
archattach(char* spec)
{
	return devattach('P', spec);
}

Walkqid*
archwalk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, archdir, narchdir, devgen);
}

static int
archstat(Chan* c, uchar* dp, int n)
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
	char *buf, *p, *ep;
	int port;
	ushort *sp;
	ulong *lp;
	IOMap *m;
	Rdwrfn *fn;

	switch((ulong)c->qid.path){

	case Qdir:
		return devdirread(c, a, n, archdir, narchdir, devgen);

	case Qiob:
		port = offset;
		checkport(offset, offset+n);
		for(p = a; port < offset+n; port++)
			*p++ = inb(port);
		return n;

	case Qiow:
		if(n & 1)
			error(Ebadarg);
		checkport(offset, offset+n);
		sp = a;
		for(port = offset; port < offset+n; port += 2)
			*sp++ = ins(port);
		return n;

	case Qiol:
		if(n & 3)
			error(Ebadarg);
		checkport(offset, offset+n);
		lp = a;
		for(port = offset; port < offset+n; port += 4)
			*lp++ = inl(port);
		return n;

	case Qioalloc:
		break;

	default:
		if(c->qid.path < narchdir && (fn = readfn[c->qid.path]))
			return fn(c, a, n, offset);
		error(Eperm);
		break;
	}

	if((buf = malloc(n)) == nil)
		error(Enomem);
	p = buf;
	ep = buf + n;
	n = n/Linelen;
	offset = offset/Linelen;

	lock(&iomap);
	for(m = iomap.m; n > 0 && m != nil; m = m->next){
		if(offset-- > 0)
			continue;
		seprint(p, ep, "%8lux %8lux %-12.12s\n", m->start,
			m->end-1, m->tag);
		p += Linelen;
		n--;
	}
	unlock(&iomap);

	n = p - buf;
	memmove(a, buf, n);
	free(buf);

	return n;
}

static long
archwrite(Chan *c, void *a, long n, vlong offset)
{
	char *p;
	int port;
	ushort *sp;
	ulong *lp;
	Rdwrfn *fn;

	switch((ulong)c->qid.path){

	case Qiob:
		p = a;
		checkport(offset, offset+n);
		for(port = offset; port < offset+n; port++)
			outb(port, *p++);
		return n;

	case Qiow:
		if(n & 1)
			error(Ebadarg);
		checkport(offset, offset+n);
		sp = a;
		for(port = offset; port < offset+n; port += 2)
			outs(port, *sp++);
		return n;

	case Qiol:
		if(n & 3)
			error(Ebadarg);
		checkport(offset, offset+n);
		lp = a;
		for(port = offset; port < offset+n; port += 4)
			outl(port, *lp++);
		return n;

	default:
		if(c->qid.path < narchdir && (fn = writefn[c->qid.path]))
			return fn(c, a, n, offset);
		error(Eperm);
		break;
	}
	return 0;
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

/*
 *  the following is a generic version of the
 *  architecture specific stuff
 */

static int
unimplemented(int)
{
	return 0;
}

static void
nop(void)
{
}

void
archreset(void)
{
	arch->introff();
	pcireset();			/* turn off bus masters & intrs */

	i8042reset();

	/*
	 * Often the BIOS hangs during restart if a conventional 8042
	 * warm-boot sequence is tried. The following is Intel specific and
	 * seems to perform a cold-boot, but at least it comes back.
	 * And sometimes there is no keyboard...
	 *
	 * The reset register (0xcf9) is usually in one of the bridge
	 * chips. The actual location and sequence could be extracted from
	 * ACPI but why bother, this is the end of the line anyway.
	 */
	*(ushort*)WARMBOOT = 0x1234;	/* BIOS warm-boot flag */
	outb(Ppcireset, 0x02);		/* ensure bit transition ... */
	microdelay(10);
	outb(Ppcireset, 0x06);		/* ... on this write; warm reset */
	delay(1);

	/* suggestion: set sp to 0, send nmi to self */
	*(int *)0 = 0;
	delay(1);

	iprint("can't reset: idling\n");
	idle();
}

/*
 * On a uniprocessor, you'd think that coherence could be nop,
 * but it can't.  We still need a barrier when using coherence() in
 * device drivers.
 *
 * On VMware, it's safe (and a huge win) to set this to nop.
 * Aux/vmware does this via the #P/archctl file.
 */
void (*coherence)(void) = nop;

/* also used by ../port/taslock.c */
void (*locknote)(void) = nop;
void (*idlehands)(void) = nop;
void (*lockwake)(void) = nop;

PCArch archgeneric = {
.id=		"generic",
.ident=		0,
.reset=		archreset,

.intrinit=	i8259init,
.intrenable=	i8259enable,
.intrvecno=	i8259vecno,
.intrdisable=	i8259disable,
.intron=	i8259on,
.introff=	i8259off,

.clockenable=	i8253enable,
.fastclock=	i8253read,
.timerset=	i8253timerset,
};

PCArch archmp = {
.id=		"_MP_",
.ident=		mpidentify,
.reset=		mpshutdown,

.intrinit=	mpinit,
.intrenable=	mpintrenable,
.intron=	lapicintron,
.introff=	lapicintroff,

.fastclock=	i8253read,
.timerset=	lapictimerset,
.resetothers=	mpresetothers,
};

PCArch* knownarch[] = {
	&archmp,
};

PCArch* arch = &archgeneric;

typedef struct X86type X86type;
struct X86type {
	int	family;
	int	model;
	int	aalcycles;
	char*	name;
};

/* cpuid ax is 0x0ffMTFmS, where 0xffF is family, 0xMm is model */
static X86type x86intel[] =
{
	{ 4,	0,	22,	"486DX", },	/* known chips. 1989 */
	{ 4,	1,	22,	"486DX50", },
	{ 4,	2,	22,	"486SX", },
	{ 4,	3,	22,	"486DX2", },
	{ 4,	4,	22,	"486SL", },
	{ 4,	5,	22,	"486SX2", },
	{ 4,	7,	22,	"DX2WB", },	/* P24D */
	{ 4,	8,	22,	"DX4", },	/* P24C */
	{ 4,	9,	22,	"DX4WB", },	/* P24CT */

	{ 5,	0,	23,	"P5", },	/* 1993 */
	{ 5,	1,	23,	"P5", },
	{ 5,	2,	23,	"P54C", },	/* 1994 */
	{ 5,	3,	23,	"P24T", },
	{ 5,	4,	23,	"P55C MMX", },
	{ 5,	7,	23,	"P54C VRT", },

	{ 6,	1,	16,	"PentiumPro", },	/* 1995; trial & error */
	{ 6,	3,	16,	"PentiumII", },		/* 1997 */
	{ 6,	5,	16,	"PentiumII/Xeon", },	/* 1998 */
	{ 6,	6,	16,	"Celeron", },
	{ 6,	7,	16,	"PentiumIII/Xeon", },	/* 1999 */
	{ 6,	8,	16,	"PentiumIII/Xeon", },
	{ 6,	0xB,	16,	"PentiumIII/Xeon", },
	/* 2004: Intel 64 arrived */
	{ 6,	0xF,	16,	"Core 2/Xeon", },	/* 2006 */
	{ 6,	0x16,	16,	"Celeron", },
	{ 6,	0x17,	16,	"Core 2/Xeon", },
	{ 6,	0x1A,	16,	"Core i7/Xeon", },
	{ 6,	0x1C,	16,	"Atom", },
	{ 6,	0x1D,	16,	"Xeon 7000 MP", },
	{ 6,	0x1E,	16,	"Core i5/i7/Xeon", },	/* 2009 */
	{ 6,	0x1F,	16,	"Core i7/Xeon", },
	{ 6,	0x22,	16,	"Core i7", },
	{ 6,	0x25,	16,	"Core i3/i5/i7/Xeon 3000", },
	{ 6,	0x26,	16,	"Atom E6xx", },
	{ 6,	0x2A,	16,	"Core i3/i5/i7/Xeon E3", },
	{ 6,	0x2C,	16,	"Core i7/Xeon 3000", },
	{ 6,	0x2D,	16,	"Core i7/Xeon E5", },
	{ 6,	0x2E,	16,	"Xeon 6000 MP", },
	{ 6,	0x2F,	16,	"Xeon E7 MP", },
	{ 6,	0x3A,	16,	"Core i3/i5/i7/Xeon E3", }, /* ~2012 */
	{ 6,	0x3C,	16,	"Core i7/Xeon", },
	{ 6,	0x3D,	16,	"Core i7", },
	{ 6,	0x3E,	16,	"Core i7", },
	{ 6,	0x3F,	16,	"Core i7", },
	{ 6,	0x9e,	16,	"Xeon E3-1275 v6", },	/* ~2017 */

	{ 0xF,	1,	16,	"PentiumIV", },		/* 2001 */
	{ 0xF,	2,	16,	"PentiumIV/Xeon", },	/* 2002 */
	/* 2004: Intel 64 arrived */
	{ 0xF,	6,	16,	"PentiumIV/Xeon", },

	{ 3,	-1,	32,	"386", },	/* family defaults */
	{ 4,	-1,	22,	"486", },
	{ 5,	-1,	23,	"P5", },
	{ 6,	-1,	16,	"P6", },
	{ 0xF,	-1,	16,	"P4", },

	{ -1,	-1,	16,	"unknown Intel", },	/* total default */
};

/*
 * The AMD processors all implement the CPUID instruction.
 * The later ones also return the processor name via functions
 * 0x80000002, 0x80000003 and 0x80000004 in registers AX, BX, CX
 * and DX:
 *	K5	"AMD-K5(tm) Processor"
 *	K6	"AMD-K6tm w/ multimedia extensions"
 *	K6 3D	"AMD-K6(tm) 3D processor"
 *	K6 3D+	?
 */
static X86type x86amd[] =
{
	{ 5,	0,	23,	"AMD-K5", },		/* guesswork */
	{ 5,	1,	23,	"AMD-K5", },		/* guesswork */
	{ 5,	2,	23,	"AMD-K5", },		/* guesswork */
	{ 5,	3,	23,	"AMD-K5", },		/* guesswork */
	{ 5,	4,	23,	"AMD Geode GX1", },	/* guesswork */
	{ 5,	5,	23,	"AMD Geode GX2", },	/* guesswork */
	{ 5,	6,	11,	"AMD-K6", },		/* trial and error */
	{ 5,	7,	11,	"AMD-K6", },		/* trial and error */
	{ 5,	8,	11,	"AMD-K6-2", },		/* trial and error */
	{ 5,	9,	11,	"AMD-K6-III", },	/* trial and error */
	{ 5,	0xa,	23,	"AMD Geode LX", },	/* guesswork */

	{ 6,	1,	11,	"AMD-Athlon", },	/* trial and error */
	{ 6,	2,	11,	"AMD-Athlon", },	/* trial and error */

	/* 2003: amd64 arrived */
	{ 0x10,	9,	11,	"AMD-K10 Opteron G34", }, /* guesswork */
	{ 0x16,	0x30,	11,	"AMD-Jaguar GX412TC", }, /* guesswork */

	{ 4,	-1,	22,	"Am486", },		/* guesswork */
	{ 5,	-1,	23,	"AMD-K5/K6", },		/* guesswork */
	{ 6,	-1,	11,	"AMD-Athlon", },	/* guesswork */
	/* 2003: amd64 arrived */
	{ 0xF,	-1,	11,	"AMD-K8", },		/* guesswork */
	{ 0x10,	-1,	11,	"AMD-K10", },		/* guesswork 2007 */
	{ 0x14,	-1,	11,	"AMD-Bobcat", },	/* guesswork */
	{ 0x15,	-1,	11,	"AMD-Bulldozer", },	/* guesswork */
	{ 0x16,	-1,	11,	"AMD-Jaguar", },	/* guesswork 2013 */
	{ 0x17,	-1,	11,	"AMD-Zen", },		/* guesswork 2017 */

	{ -1,	-1,	11,	"unknown AMD", },	/* total default */
};

/*
 * WinChip 240MHz
 */
static X86type x86winchip[] =
{
	{5,	4,	23,	"Winchip",},	/* guesswork */
	{6,	7,	23,	"Via C3 Samuel 2 or Ezra",},
	{6,	8,	23,	"Via C3 Ezra-T",},
	{6,	9,	23,	"Via C3 Eden-N",},
	{ -1,	-1,	23,	"unknown Via", },/* total default */
};

/*
 * SiS 55x
 */
static X86type x86sis[] =
{
	{5,	0,	23,	"SiS 55x",},	/* guesswork */
	{ -1,	-1,	23,	"unknown SiS", },/* total default */
};

static X86type *cputype;

static void	simplecycles(uvlong*);
void	(*cycles)(uvlong*) = simplecycles;
void	_cycles(uvlong*);	/* in l.s */

static void
simplecycles(uvlong*x)
{
	*x = m->ticks;
}

typedef struct Hyper {
	char	*name;
	char	*id;		/* Hypercpuid id string, if any */
	ushort	vid;		/* pci vendor id */
	ulong	bit;		/* for conf.vm */
} Hyper;
static Hyper hypers[] = {
	"vmware",	"VMwareVMware",	Vvmware,	Vmwarehyp,
	"virtualbox",	"VboxVboxVbox",	Voracle,	Virtualbox,
	"parallels",	"prl hyperv",	Vparallels,	Parallels,
	0, 0, 0, 0,
};
static char *hypename = "an unknown hypervisor";

static char *
hypername(void)
{
	return hypename;
}

static void
sethyper(Hyper *hype)
{
	conf.vm = hype->bit;
	hypename = hype->name;
	print("\tin a %s virtual machine\n", hypename);
}

/* if we don't know the hypervisor's maker, check pci ids */
static void
latecheckhype(void)
{
	Hyper *hype;

	if (m->machno == 0 && conf.vm == Othervm) {
		for (hype = hypers; hype->id; hype++)
			if (pcimatch(0, hype->vid, 0) != nil)
				break;
		if (hype->id)
			sethyper(hype);
		else
			print("\t%s\n", hypename);
		if (hypename == nil)
			iprint("unknown hypervisor\n");	// TODO
	}
}


/*
 *  put the processor in the halt state if we've no processes to run.  an
 *  interrupt will get us going again.  this reduces power consumed, thus heat
 *  generated.  called at spllo from runproc, but also from taslock routines
 *  (via lockidle), at splhi from ilock and either PL from lock.  enabled
 *  interrupts will resume hlt and mwait instructions; an mwait option permits
 *  resuming from even disabled interrupts on newer cpus.
 *
 *  halting in an smp system can result in a startup latency for processes that
 *  become ready (e.g., due to an interrupt).  the latency seemed to be slight
 *  and it reduced lock contention (thus system time and real time) on many-core
 *  systems with large values of NPROC.  with more experience, the performance
 *  loss greatly increases interrupt latency for at least ethernet (e.g., local
 *  gbe ping from 45µs to 6000µs), which slows the whole system, so we now use
 *  monitor & mwait if available.
 */

/*
 * it's safe to mwait at splhi on this cpu, we assert.
 * for more accurate cycle counts, mask intrs and only then count
 * the idle cycles.  this will undercount on older (pre-2014?) machines.
 */
static void
mwaitalways(void)
{
	int s;

	s = splhi();
	if (m->mword == *monitorwd) {
#ifdef MEASURE
		uvlong stcyc, endcyc;

		cycles(&stcyc);
#endif
		mwait(Mwaitintrbreakshi, 0);
		m->mword = *monitorwd;
#ifdef MEASURE
		cycles(&endcyc);
		m->mwaitcycles += endcyc - stcyc;
#endif
	}
	splx(s);
}

/*
 * we assert that this is an older cpu on which it is unsafe to mwait at splhi
 * as an interrupt can't break us out.
 */
static void
mwaitiflo(void)
{
	if (!islo())
		return;
	if (m->mword == *monitorwd) {
#ifdef MEASURE
		uvlong stcyc, endcyc;

		cycles(&stcyc);
#endif
		mwait(0, 0);
		m->mword = *monitorwd;
#ifdef MEASURE
		cycles(&endcyc);
		m->mwaitcycles += endcyc - stcyc;
#endif
	}
}

void
mwaitwatch(void)
{
	m->mword = *monitorwd;
	monitor(monitorwd, 0, 0);
}

/* harmless on systems without monitor & mwait */
static void
wakemwait(void)
{
	m->mword = ainc(monitorwd);
	coherence();
}

static void
allocmword(void)
{
	if (conf.cpuidcx & Monitor && monitorme == nil && conf.monmax != 0) {
		monitorme = xspanalloc(2*conf.monmax, conf.monmax, conf.monmax);
		if (monitorme == nil)
			panic("cpuidprint: no memory");
		monitorwd = (long *)(monitorme + conf.monmax);
	}
}

/*
 * figure out how to best wait for interrupt or unlock, while consuming
 * the least power (and generating the least heat), within reason.
 * we don't use specific C states or other extreme measures.
 */
static void
chooseidler(void)
{
	int s;

	s = splhi();
	if (m->machno != 0) {
		if ((conf.cpuidcx & Monitor) == 0 && conf.nmach > 1)
			idlehands = nop;  /* we burn more power on old MPs */
		splx(s);
		return;
	}

	if (conf.cpuidcx & Monitor) {
		lockwake = wakemwait;
		locknote = mwaitwatch;
		idlehands = conf.cpuid5cx & Mwaitintrhi? mwaitalways: mwaitiflo;
		wakemwait();
	} else					/* pre-2014 cpu? */
		idlehands = halt;
	if (0 && conf.vm)			/* TODO: tune */
		// coherence =
		idlehands = nop;		/* coherence nop ok on vmware */
	splx(s);
}

/* called later in main than cpuidentify(); okay to print, etc. now. */
void
cpuidprint(void)
{
	int cpu;

	/*
	 * there are now often too many cores to print a line for each,
	 * and they must be homogeneous anyway.
	 */
	cpu = m->machno;
	if (cpu == 0) {
		char *sp, *ep;
		char buf[128];

		ep = buf + sizeof buf - 1;
		sp = seprint(buf, ep, "cpu%d: %dMHz ", cpu, m->cpumhz);
		if(conf.cpuidid[0])
			seprint(sp, ep, "%*.*s ", Cpuidlen, Cpuidlen, conf.cpuidid);
		seprint(sp, ep, "%s (cpuid: AX 0x%4.4uX DX 0x%4.4uX);\n"
			"\tbrand %s\n",
			conf.cpuidtype, conf.cpuidax, m->cpuiddx, conf.brand);
		print("%s", buf);
		latecheckhype();
		allocmword();		/* all cpus use this monitorwd */
	} else {
		static int beenhere;

		if (!beenhere) {
			print("more cpus:");
			beenhere = 1;
		}
		print(" %d", cpu);
	}
	chooseidler();
}

void
trim(char *s, int len)
{
	char *p;

	for(p = s + len; --p > s && (*p == ' ' || *p == '\0'); )
		*p = '\0';
}

static void
getcpuids(void)
{
	int leaf;
	ulong cpuidhigh, cpuidexthigh;
	ulong regs[4];
	char *p;

	memset(regs, 0, sizeof regs);
	cpuid(Procsig, regs);
	m->cpuiddx = regs[3];
	if (m->machno != 0)
		return;
	conf.cpuidax = regs[0];
	conf.cpuidcx = regs[2];
	/*
	 * cpuid function 2 on pentium pro and later (P6), or function 4 on
	 * pentium 4 and later (P4), gives more details.  pentium & later
	 * were 32 bytes; it's been 64 since pentium 4 (2000) & atom.
	 * note sure what amd uses.
	 */
	conf.cachelinesz = 8 * ((regs[1] >> 8) & MASK(8));
	if (!(m->cpuiddx & Cpuapic))
		iprint("cpu0: no lapic; should retire this pre-1994 antique\n");

	memset(regs, 0, sizeof regs);
	cpuid(Highstdfunc, regs);
	cpuidhigh = regs[0];			/* ax */
	memmove(conf.cpuidid,   &regs[1], BY2WD); /* bx */
	memmove(conf.cpuidid+4, &regs[3], BY2WD); /* dx */
	memmove(conf.cpuidid+8, &regs[2], BY2WD); /* cx */
	conf.cpuidid[Cpuidlen] = '\0';

	conf.cpuid5cx = 0;
	conf.monmin = conf.monmax = 0;
	if (cpuidhigh >= Procmon) {
		memset(regs, 0, sizeof regs);
		cpuid(Procmon, regs);
		conf.monmin = (ushort)regs[0];	/* e.g., 64 */
		conf.monmax = (ushort)regs[1];	/* e.g., 64 */
		conf.cpuid5cx = regs[2];	/* cx */
		/* it's too early to allocate memory; see cpuidprint */
	}

	conf.cpuid7bx = 0;
	if (cpuidhigh >= Procid7) {
		memset(regs, 0, sizeof regs);
		cpuid(Procid7, regs);
		conf.cpuid7bx = regs[1];
	}

	memset(regs, 0, sizeof regs);
	cpuid(Highextfunc, regs);
	cpuidexthigh = regs[0];			/* ax */
	if (cpuidexthigh >= Brand2) {
		p = conf.brand;
		for (leaf = Brand0; leaf <= Brand2; leaf++) {
			memset(regs, 0, sizeof regs);
			cpuid(leaf, regs);
			memmove(p, regs, sizeof regs);
			p += sizeof regs;
		}
		*p = '\0';
		trim(conf.brand, p - conf.brand);
	}
}

static X86type *
findvendor(void)
{
	X86type *tab;

	if(strncmp(conf.cpuidid, "AuthenticAMD", Cpuidlen) == 0 ||
	   strncmp(conf.cpuidid, "Geode by NSC", Cpuidlen) == 0) {
		tab = x86amd;
		conf.x86type = Amd;
	} else if(strncmp(conf.cpuidid, "CentaurHauls", Cpuidlen) == 0) {
		tab = x86winchip;
		conf.x86type = Centaur;
	} else if(strncmp(conf.cpuidid, "SiS SiS SiS ", Cpuidlen) == 0) {
		tab = x86sis;
		conf.x86type = Sis;
	} else {
		tab = x86intel;
		conf.x86type = Intel;
	}
	return tab;
}

/*
 * If machine check exception, page size extensions, page global bit, or sse fp
 * are supported enable them in CR4 and clear any other set extensions.
 * If machine check was enabled clear out any lingering status.
 */
static void
checkcr4extensions(int family)
{
	char *p;
	int nomce;
	ulong cr4;
	vlong mca, mct;

	if(!(m->cpuiddx & Fxsr)){
		fpsave = fpx87save;
		fprestore = fpx87restore;
	}
	if((m->cpuiddx & (Pge|Mce|Pse|Fxsr)) == 0)
		return;		/* no cr4 extensions that we care about */

	cr4 = 0;
	if(m->cpuiddx & Pse)
		cr4 |= CR4pse;
	if(p = getconf("*nomce"))
		nomce = strtoul(p, 0, 0);
	else
		nomce = 0;
	if((m->cpuiddx & Mce) && !nomce){
		cr4 |= CR4mce;
		if(family == 5){ /* these msrs are nops on later cpus */
			rdmsr(Msrmcaddr, &mca);
			rdmsr(Msrmctype, &mct);
		}
	}

	/*
	 * Detect whether the chip supports the global bit
	 * in page directory and page table entries.  When set
	 * in a particular entry, it means ``don't bother removing
	 * this from the TLB when CR3 changes.''
	 *
	 * We flag all kernel pages with this bit.  Doing so lessens the
	 * overhead of switching processes on bare hardware,
	 * even more so on VMware.  See mmu.c:/^memglobal.
	 *
	 * For future reference, should we ever need to do a
	 * full TLB flush, it can be accomplished by clearing
	 * the PGE bit in CR4, writing to CR3, and then
	 * restoring the PGE bit.
	 */
	if(m->cpuiddx & Pge){
		cr4 |= CR4pge;
		m->havepge = 1;
	}

	if(m->cpuiddx & Fxsr){			/* have sse fp? */
		cr4 |= CR4Osfxsr;
		fpsave = fpssesave;
		fprestore = fpsserestore;
	}
	putcr4(cr4);
	if(m->cpuiddx & Mce)
		rdmsr(Msrmctype, &mct);
}

/* are we running on imaginary hardware? */
static void
checkhype(void)
{
	int hypecpuid;
	ulong regs[4];
	char hypid[Cpuidlen+1];
	Hyper *hype;
	static ulong zregs[4];

	if (m->machno != 0)
		return;
	conf.vm = conf.cpuidcx & Hypervisor? Othervm: 0;
	if (!conf.vm)
		return;
	hypecpuid = 1;
	memset(regs, 0, sizeof regs);
	cpuid(Hypercpuid, regs);

	/* if cpuid knew Hypercpuid, it zeroed regs to reject it on hw */
	if (memcmp(regs, zregs, sizeof regs) == 0)
		hypecpuid = 0;
	/* if cpuid didn't know Hypercpuid at all, it did nothing */
	zregs[0] = Hypercpuid;
	if (memcmp(regs, zregs, sizeof regs) == 0)
		hypecpuid = 0;
	zregs[0] = 0;
	if (hypecpuid) {
		/* NB: not the same order as normal cpuidid */
		memmove(hypid, &regs[1], Cpuidlen);	/* bx-dx */
		hypid[Cpuidlen] = '\0';
		for (hype = hypers; hype->id; hype++)
			if (strcmp(hypid, hype->id) == 0)
				break;
		if (hype->id)
			sethyper(hype);
	}
	/* else catch it by pci id in latecheckhype() */
}

/*
 *  figure out:
 *	- cpu type & record various cpuid leaves
 *	- whether or not we have a TSC (cycle counter)
 *	- whether or not it supports any of the following, and if so,
 *	  turn them on: page size extensions, machine check exceptions,
 *	  the page global flag
 */
int
cpuidentify(void)
{
	int family, model;
	X86type *t;

	getcpuids();
	family = X86FAMILY(conf.cpuidax);
	model = X86MODEL(conf.cpuidax);
	for(t=findvendor(); t->name; t++)
		if((t->family == family && t->model == model)
		|| (t->family == family && t->model == -1)
		|| (t->family == -1))
			break;
	cputype = t;
	conf.cpuidtype = cputype->name;

	/*
	 *  if there is one, set tsc to a known value
	 */
	if(m->cpuiddx & Tsc){
		conf.havetsc = 1;
		cycles = _cycles;
		if(m->cpuiddx & Cpumsr)
			wrmsr(Msrtsc, 0);
	}

	/*
	 *  use i8253 to guess our cpu speed
	 */
	guesscpuhz(t->aalcycles);

	checkcr4extensions(family);
	checkhype();
	return t->family;
}

static long
cputyperead(Chan*, void *a, long n, vlong offset)
{
	char str[32];
	ulong mhz;

	mhz = (m->cpuhz+999999)/1000000;

	snprint(str, sizeof(str), "%s %lud\n", cputype->name, mhz);
	return readstr(offset, a, n, str);
}

static long
archctlread(Chan*, void *a, long nn, vlong offset)
{
	int n;
	char *buf, *p, *ep;

	p = buf = malloc(READSTR);
	if(p == nil)
		error(Enomem);
	ep = p + READSTR;
	p = seprint(p, ep, "cpu %s %lud%s\n",
		cputype->name, (ulong)(m->cpuhz+999999)/1000000,
		m->havepge ? " pge" : "");
	p = seprint(p, ep, "pge %s\n", getcr4()&CR4pge ? "on" : "off");
	p = seprint(p, ep, "id %s\n", conf.cpuidid);
	p = seprint(p, ep, "brand %s\n", conf.brand);
	if(conf.vm)
		p = seprint(p, ep, "vm %s\n", hypename);
	p = seprint(p, ep, "cache-line-size %ld\n", conf.cachelinesz);
	if (monitorme != nil && conf.monmax != 0) {
		p = seprint(p, ep, "mwait-line-size %d", conf.monmin);
		if (conf.monmin != conf.monmax)
			p = seprint(p, ep, "-%d", conf.monmax);
		p = seprint(p, ep, "\n");
#ifdef MEASURE
		uvlong mwaitcycles;

		mwaitcycles = 0;
		for (n = 0; n < conf.nmach; n++)
			mwaitcycles += MACHP(n)->mwaitcycles;
		p = seprint(p, ep, "mwait %ld cpus %,llud cycles = %llud s.\n",
			conf.nmach, mwaitcycles, mwaitcycles / m->cpuhz);
#endif
		p = seprint(p, ep, "mwait %s extension to break on intrs always\n",
			conf.cpuid5cx & Mwaitintrhi? "has": "doesn't have");
	}
	p = seprint(p, ep, "coherence ");
	if(coherence == mb586)
		p = seprint(p, ep, "mb586\n");
	else if(coherence == mfence)
		p = seprint(p, ep, "mfence\n");
	else if(coherence == nop)
		p = seprint(p, ep, "nop\n");
	else
		p = seprint(p, ep, "%#p\n", coherence);
	p = seprint(p, ep, "cmpswap ");
	if(cmpswap == cmpswap486)
		p = seprint(p, ep, "cmpswap486\n");
	else
		p = seprint(p, ep, "%#p\n", cmpswap);

	p = seprint(p, ep, "i8253set %s\n", doi8253set ? "on" : "off");
	n = (p - buf) + mtrrprint(p, ep - p);
	buf[n] = '\0';

	n = readstr(offset, a, nn, buf);
	free(buf);
	return n;
}

enum
{
	CMpge,
	CMcoherence,
	CMi8253set,
	CMcache,
};

static Cmdtab archctlmsg[] =
{
	CMpge,		"pge",		2,
	CMcoherence,	"coherence",	2,
	CMi8253set,	"i8253set",	2,
	CMcache,	"cache",	4,
};

static char Eincoher[] = "coherence style too new for old cpu";

static long
archctlwrite(Chan*, void *a, long n, vlong)
{
	uvlong base, size;
	Cmdbuf *cb;
	Cmdtab *ct;
	char *ep;

	cb = parsecmd(a, n);
	if(waserror()){
		free(cb);
		nexterror();
	}
	ct = lookupcmd(cb, archctlmsg, nelem(archctlmsg));
	switch(ct->index){
	case CMpge:
		if(!m->havepge)
			error("processor does not support pge");
		if(strcmp(cb->f[1], "on") == 0)
			putcr4(getcr4() | CR4pge);
		else if(strcmp(cb->f[1], "off") == 0)
			putcr4(getcr4() & ~CR4pge);
		else
			cmderror(cb, "invalid pge ctl");
		break;
	case CMcoherence:
		if(strcmp(cb->f[1], "mb586") == 0){
			if(!cpuispost486())		/* never true */
				error(Eincoher);
			coherence = mb586;
		}else if(strcmp(cb->f[1], "mfence") == 0){
			if((m->cpuiddx & Sse2) == 0)	/* antique, pre-2000 */
				error(Eincoher);
			coherence = mfence;
		}else if(strcmp(cb->f[1], "nop") == 0){
			/* only safe on vmware */
			if(conf.nmach > 1)
				error("cannot disable coherence on a multiprocessor");
			coherence = nop;
		}else
			cmderror(cb, "invalid coherence ctl");
		break;
	case CMi8253set:
		if(strcmp(cb->f[1], "on") == 0)
			doi8253set = 1;
		else if(strcmp(cb->f[1], "off") == 0){
			doi8253set = 0;
			(*arch->timerset)(0);
		}else
			cmderror(cb, "invalid i2853set ctl");
		break;
	case CMcache:
		base = strtoull(cb->f[1], &ep, 0);
		if(*ep)
			error("cache: parse error: base not a number?");
		size = strtoull(cb->f[2], &ep, 0);
		if(*ep)
			error("cache: parse error: size not a number?");
		mtrr(base, size, cb->f[3]);
		break;
	}
	free(cb);
	poperror();
	return n;
}

/* called after cpuidentify */
void
archinit(void)
{
	PCArch *newarch;
	PCArch **p;

	newarch = nil;
	for(p = knownarch; *p; p++){
		if((*p)->ident && (*p)->ident() == 0){
			newarch = *p;
			break;
		}
	}
	if(newarch == nil)
		arch = &archgeneric;
	else{
		arch = newarch;
		if(arch->id == 0)
			arch->id = archgeneric.id;
		if(arch->reset == 0)
			arch->reset = archgeneric.reset;
		if(arch->intrinit == 0)
			arch->intrinit = archgeneric.intrinit;
		if(arch->intrenable == 0)
			arch->intrenable = archgeneric.intrenable;
	}

	/*
	 *  Decide whether to use copy-on-reference (386 and mp).
	 *  We get another chance to set it in mpinit() for a
	 *  multiprocessor.
	 */
	if(cpuisa386()) {		/* never true; family too old */
		conf.copymode = 1;
		// cmpswap = cmpswap386;
	}

	/*
	 * 32 bytes is a safe size for flushing a range of cache lines.
	 */
	if (conf.cachelinesz == 0)
		conf.cachelinesz = 32;

	if(m->cpuiddx & Sse2) {		/* antique, pre-2000 */
		coherence = mfence;
		iprint("you should retire this 20th century antique.\n");
	} else if(cpuispost486())
		coherence = mb586;

	addarchfile("cputype", 0444, cputyperead, nil);
	addarchfile("archctl", 0664, archctlread, archctlwrite);
}

void
archrevert(void)
{
	arch = &archgeneric;
	/* could zero _mp_ too, if feeling paranoid */
}

/*
 *  return value and speed of timer set in arch->clockenable
 */
uvlong
fastticks(uvlong *hz)
{
	return (*arch->fastclock)(hz);
}

ulong
µs(void)
{
	return fastticks2us((*arch->fastclock)(nil));
}

/*
 *  set next timer interrupt
 */
void
timerset(Tval x)
{
	if(doi8253set)
		(*arch->timerset)(x);
}

int
clz(Clzuint n)			/* count leading zeroes */
{
	if (n == 0)
		return BI2BY * sizeof n;
	return BI2BY * sizeof n - 1 - bsr(n);
}
