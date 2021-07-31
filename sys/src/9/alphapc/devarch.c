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
	Qdir,
	Qcputype,
	Qioalloc,
	Qiob,
	Qiow,
	Qiol,
	Qirqalloc,
};

static Dirtab ioallocdir[] = {
	"cputype",	{ Qcputype, 0 },	0,	0444,
	"ioalloc",	{ Qioalloc, 0 },	0,	0444,
	"iob",		{ Qiob, 0 },		0,	0660,
	"iow",		{ Qiow, 0 },		0,	0660,
	"iol",		{ Qiol, 0 },		0,	0660,
	"irqalloc",	{ Qirqalloc, 0},	0,	0444,
};

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

static long cputyperead(char*, int, ulong);

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

	ioallocdir[0].qid.vers++;

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
	ioallocdir[0].qid.vers++;
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

int
archwalk(Chan* c, char* name)
{
	return devwalk(c, name, ioallocdir, nelem(ioallocdir), devgen);
}

static void
archstat(Chan* c, char* dp)
{
	devstat(c, dp, ioallocdir, nelem(ioallocdir), devgen);
}

static Chan*
archopen(Chan* c, int omode)
{
	return devopen(c, omode, ioallocdir, nelem(ioallocdir), devgen);
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
	char *p;
	IOMap *m;
	char buf[Linelen+1];
	int port;
	ushort *sp;
	ulong *lp;

	switch(c->qid.path & ~CHDIR){

	case Qdir:
		return devdirread(c, a, n, ioallocdir, nelem(ioallocdir), devgen);

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

	case Qirqalloc:
		return irqallocread(a, n, offset);

	case Qcputype:
		return cputyperead(a, n, offset);

	default:
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
	int port;
	ushort *sp;
	ulong *lp;
	char *p;

	switch(c->qid.path & ~CHDIR){

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
	archattach,
	devclone,
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
// [26]		"EB164",
[26]		"AlphaPC 164",
};

static char	*cpunames[] =
{
[7]		"21164A",
};

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
}

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
		print("DEC %s (%llux, %llux, %llux)\n", s, hwrpb->systype, hwrpb->sysvar, hwrpb->sysrev);
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
cputyperead(char *a, int n, ulong offset)
{
	char str[32], *cputype;
	ulong mhz, maj;
	Hwcpu *cpu;

	mhz = (m->cpuhz+999999)/1000000;
	cpu = (Hwcpu*) ((ulong)hwrpb + hwrpb->cpuoff);
	cputype = "unknown";
	maj = (ulong)cpu->cputype;
	if (maj < nelem(cpunames))
		cputype = cpunames[maj];

	snprint(str, sizeof(str), "%s %lud\n", cputype, mhz);
	return readstr(offset, a, n, str);
}
