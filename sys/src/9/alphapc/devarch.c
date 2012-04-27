#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include	"axp.h"

typedef struct IOMap IOMap;
struct IOMap
{
	IOMap	*next;
	char	tag[13];
	ulong	start;
	ulong	end;
};

static struct
{
	Lock;
	IOMap	*m;
	IOMap	*free;
	IOMap	maps[32];		// some initial free maps

	QLock	ql;			// lock for reading map
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

typedef long Rdwrfn(Chan*, void*, long, vlong);

static Rdwrfn *readfn[Qmax];
static Rdwrfn *writefn[Qmax];

static Dirtab archdir[] = {
	".",	{ Qdir, 0, QTDIR },	0,	0555,
	"ioalloc",	{ Qioalloc, 0 },	0,	0444,
	"iob",		{ Qiob, 0 },		0,	0660,
	"iow",		{ Qiow, 0 },		0,	0660,
	"iol",		{ Qiol, 0 },		0,	0660,
};
Lock archwlock;	/* the lock is only for changing archdir */
int narchdir = Qbase;
int (*_pcmspecial)(char *, ISAConf *);
void (*_pcmspecialclose)(int);

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
	int i;

	for(i = 0; i < nelem(iomap.maps)-1; i++)
		iomap.maps[i].next = &iomap.maps[i+1];
	iomap.maps[i].next = nil;
	iomap.free = iomap.maps;

	// a dummy entry at 2^17
	ioalloc(0x20000, 1, 0, "dummy");
}

//
//	alloc some io port space and remember who it was
//	alloced to.  if port < 0, find a free region.
//
int
ioalloc(int port, int size, int align, char *tag)
{
	IOMap *m, **l;
	int i;

	lock(&iomap);
	if(port < 0){
		// find a free port above 0x400 and below 0x1000
		port = 0x400;
		for(l = &iomap.m; *l; l = &(*l)->next){
			m = *l;
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
		// see if the space clashes with previously allocated ports
		for(l = &iomap.m; *l; l = &(*l)->next){
			m = *l;
			if(m->end <= port)
				continue;
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
	return devopen(c, omode, archdir, nelem(archdir), devgen);
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
	char buf[Linelen+1], *p;
	int port;
	ushort *sp;
	ulong *lp;
	IOMap *m;
	Rdwrfn *fn;

	switch((ulong)c->qid.path){

	case Qdir:
		return devdirread(c, a, n, archdir, nelem(archdir), devgen);

	case Qiob:
		port = offset;
		checkport(offset, offset+n);
		for(p = a; port < offset+n; port++)
			*p++ = inb(port);
		return n;

	case Qiow:
		if((n & 0x01) || (offset & 0x01))
			error(Ebadarg);
		checkport(offset, offset+n+1);
		n /= 2;
		sp = a;
		for(port = offset; port < offset+n; port += 2)
			*sp++ = ins(port);
		return n*2;

	case Qiol:
		if((n & 0x03) || (offset & 0x03))
			error(Ebadarg);
		checkport(offset, offset+n+3);
		n /= 4;
		lp = a;
		for(port = offset; port < offset+n; port += 4)
			*lp++ = inl(port);
		return n*4;

	case Qioalloc:
		break;

	default:
		if(c->qid.path < narchdir && (fn = readfn[c->qid.path]))
			return fn(c, a, n, offset);
		error(Eperm);
		break;
	}

	offset = offset/Linelen;
	n = n/Linelen;
	p = a;
	lock(&iomap);
	for(m = iomap.m; n > 0 && m != nil; m = m->next){
		if(offset-- > 0)
			continue;
		if(strcmp(m->tag, "dummy") == 0)
			break;
		sprint(buf, "%8lux %8lux %-12.12s\n", m->start, m->end-1, m->tag);
		memmove(p, buf, Linelen);
		p += Linelen;
		n--;
	}
	unlock(&iomap);

	return p - (char*)a;
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
		if((n & 01) || (offset & 01))
			error(Ebadarg);
		checkport(offset, offset+n+1);
		n /= 2;
		sp = a;
		for(port = offset; port < offset+n; port += 2)
			outs(port, *sp++);
		return n*2;

	case Qiol:
		if((n & 0x03) || (offset & 0x03))
			error(Ebadarg);
		checkport(offset, offset+n+3);
		n /= 4;
		lp = a;
		for(port = offset; port < offset+n; port += 4)
			outl(port, *lp++);
		return n*4;

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

PCArch* arch;
extern PCArch* knownarch[];

PCArch archgeneric = {
	"generic",				/* id */
	0,					/* ident */

	0,					/* coreinit */
	0,					/* coredetach */
};

static char	*sysnames[] =
{
[1]		"Alpha Demo. Unit",
[2]		"DEC 4000; Cobra",
[3]		"DEC 7000; Ruby",
[4]		"DEC 3000/500; Flamingo family (TC)",
[6]		"DEC 2000/300; Jensen (EISA/ISA)",
[7]		"DEC 3000/300; Pelican (TC)",
[8]		"Avalon A12; Avalon Multicomputer",
[9]		"DEC 2100/A500; Sable",
[10]		"DEC APXVME/64; AXPvme (VME?)",
[11]		"DEC AXPPCI/33; NoName (PCI/ISA)",
[12]		"DEC 21000; TurboLaser (PCI/EISA)",
[13]		"DEC 2100/A50; Avanti (PCI/ISA)",
[14]		"DEC MUSTANG; Mustang",
[15]		"DEC KN20AA; kn20aa (PCI/EISA)",
[17]		"DEC 1000; Mikasa (PCI/ISA?)",
[19]		"EB66; EB66 (PCI/ISA?)",		// DEC?
[20]		"EB64P; EB64+ (PCI/ISA?)",		// DEC?
[21]		"Alphabook1; Alphabook",
[22]		"DEC 4100; Rawhide (PCI/EISA)",
[23]		"DEC EV45/PBP; Lego",
[24]		"DEC 2100A/A500; Lynx",
[26]		"DEC AlphaPC 164",	// only supported one: "EB164 (PCI/ISA)"
[27]		"DEC 1000A; Noritake",
[28]		"DEC AlphaVME/224; Cortex",
[30]		"DEC 550; Miata (PCI/ISA)",
[32]		"DEC EV56/PBP; Takara",
[33]		"DEC AlphaVME/320; Yukon (VME?)",
[34]		"DEC 6600; MonetGoldrush",
// 200 and up is Alpha Processor Inc. machines
// [201]	"API UP1000; Nautilus",
};

static char	*cpunames[] =
{
[1]		"EV3",
[2]		"EV4: 21064",
[3]		"Simulation",
[4]		"LCA4: 2106[68]",
[5]		"EV5: 21164",
[6]		"EV45: 21064A",
[7]		"21164A",		/* only supported one: EV56 */
[8]		"EV6: 21264",
[9]		"PCA256: 21164PC",
};

void
cpuidprint(void)
{
	int i, maj, min;
	Hwcpu *cpu;
	Hwdsr *dsr;
	char *s;

	print("\n");

	if (hwrpb->rev >= 6) {
		dsr = (Hwdsr*)((ulong)hwrpb + hwrpb->dsroff);

		s = (char*)dsr + dsr->sysnameoff + 8;
		print("%s\n", s);
	}
	else {
		s = "<unknown>";
		if (hwrpb->systype < nelem(sysnames))
			s = sysnames[hwrpb->systype];
		print("%s (%llux, %llux, %llux)\n", s, hwrpb->systype, hwrpb->sysvar, hwrpb->sysrev);
	}

	for (i = 0; i < hwrpb->ncpu; i++) {
		cpu = (Hwcpu*) ((ulong)hwrpb + hwrpb->cpuoff + i*hwrpb->cpulen);
		s = "<unknown>";
		maj = (ulong)cpu->cputype;
		min = (ulong)(cpu->cputype>>32);
		if (maj < nelem(cpunames))
			s = cpunames[maj];
		print("cpu%d: %s-%d (%d.%d, %llux, %llux)\n",
			i, s, min, maj, min, cpu->cpuvar, cpu->cpurev);
	}

	print("\n");
}

static long
cputyperead(Chan*, void *a, long n, vlong offset)
{
	char str[32], *cputype;
	ulong mhz, maj;
	Hwcpu *cpu;

	mhz = (m->cpuhz+999999)/1000000;
	cpu = (Hwcpu*) ((ulong)hwrpb + hwrpb->cpuoff);	/* NB CPU 0 */
	cputype = "unknown";
	maj = (ulong)cpu->cputype;
	if (maj < nelem(cpunames))
		cputype = cpunames[maj];

	snprint(str, sizeof(str), "%s %lud\n", cputype, mhz);
	return readstr(offset, a, n, str);
}

void
archinit(void)
{
	PCArch **p;

	arch = 0;
	for(p = knownarch; *p; p++){
		if((*p)->ident && (*p)->ident() == 0){
			arch = *p;
			break;
		}
	}
	if(arch == 0)
		arch = &archgeneric;

	addarchfile("cputype", 0444, cputyperead, nil);
}

int
pcmspecial(char *idstr, ISAConf *isa)
{
	return (_pcmspecial  != nil)? _pcmspecial(idstr, isa): -1;
}

void
pcmspecialclose(int a)
{
	if (_pcmspecialclose != nil)
		_pcmspecialclose(a);
}
