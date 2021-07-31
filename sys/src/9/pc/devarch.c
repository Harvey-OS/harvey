#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"


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

	// a dummy entry at 2^16
	ioalloc(0x10000, 1, 0, "dummy");
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

void (*coherence)(void) = nop;
void cycletimerinit(void);
uvlong cycletimer(uvlong*);

PCArch* arch;
extern PCArch* knownarch[];

PCArch archgeneric = {
	"generic",				/* id */
	0,					/* ident */
	i8042reset,				/* reset */
	unimplemented,				/* serialpower */
	unimplemented,				/* modempower */

	i8259init,				/* intrinit */
	i8259enable,				/* intrenable */

	i8253enable,				/* clockenable */

	i8253read,				/* read the standard timer */
};

typedef struct {
	int	family;
	int	model;
	int	aalcycles;
	char*	name;
} X86type;

static X86type x86intel[] =
{
	{ 4,	0,	22,	"486DX", },	/* known chips */
	{ 4,	1,	22,	"486DX50", },
	{ 4,	2,	22,	"486SX", },
	{ 4,	3,	22,	"486DX2", },
	{ 4,	4,	22,	"486SL", },
	{ 4,	5,	22,	"486SX2", },
	{ 4,	7,	22,	"DX2WB", },	/* P24D */
	{ 4,	8,	22,	"DX4", },	/* P24C */
	{ 4,	9,	22,	"DX4WB", },	/* P24CT */
	{ 5,	0,	23,	"P5", },
	{ 5,	1,	23,	"P5", },
	{ 5,	2,	23,	"P54C", },
	{ 5,	3,	23,	"P24T", },
	{ 5,	4,	23,	"P55C MMX", },
	{ 5,	7,	23,	"P54C VRT", },
	{ 6,	1,	16,	"PentiumPro", },/* trial and error */
	{ 6,	3,	16,	"PentiumII", },
	{ 6,	5,	16,	"PentiumII/Xeon", },
	{ 6,	6,	16,	"Celeron", },
	{ 6,	7,	16,	"PentiumIII/Xeon", },
	{ 6,	8,	16,	"PentiumIII/Xeon", },

	{ 3,	-1,	32,	"386", },	/* family defaults */
	{ 4,	-1,	22,	"486", },
	{ 5,	-1,	23,	"P5", },
	{ 6,	-1,	16,	"P6", },

	{ -1,	-1,	23,	"unknown", },	/* total default */
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
	{ 5,	0,	23,	"AMD-K5", },	/* guesswork */
	{ 5,	1,	23,	"AMD-K5", },	/* guesswork */
	{ 5,	2,	23,	"AMD-K5", },	/* guesswork */
	{ 5,	3,	23,	"AMD-K5", },	/* guesswork */
	{ 5,	6,	11,	"AMD-K6", },	/* trial and error */
	{ 5,	7,	11,	"AMD-K6", },	/* trial and error */
	{ 5,	8,	11,	"AMD-K6-2", },	/* trial and error */
	{ 5,	9,	11,	"AMD-K6-III", },/* trial and error */

	{ 6,	1,	11,	"AMD-Athlon", },/* trial and error */
	{ 6,	2,	11,	"AMD-Athlon", },/* trial and error */

	{ 4,	-1,	22,	"Am486", },	/* guesswork */
	{ 5,	-1,	23,	"AMD-K5/K6", },	/* guesswork */
	{ 6,	-1,	11,	"AMD-Athlon", },/* guesswork */

	{ -1,	-1,	23,	"unknown", },	/* total default */
};

static uvlong fasthz;
static X86type *cputype;

void
cpuidprint(void)
{
	int i;
	char buf[128];

	i = sprint(buf, "cpu%d: %dMHz ", m->machno, m->cpumhz);
	if(m->cpuidid[0])
		i += sprint(buf+i, "%s ", m->cpuidid);
	sprint(buf+i, "%s (cpuid: AX 0x%4.4uX DX 0x%4.4uX)\n",
		m->cpuidtype, m->cpuidax, m->cpuiddx);
	print(buf);
}

int
cpuidentify(void)
{
	int family, model;
	X86type *t;
	ulong cr4;
	vlong mct;

	cpuid(m->cpuidid, &m->cpuidax, &m->cpuiddx);
	if(strncmp(m->cpuidid, "AuthenticAMD", 12) == 0)
		t = x86amd;
	else
		t = x86intel;
	family = X86FAMILY(m->cpuidax);
	model = X86MODEL(m->cpuidax);
	while(t->name){
		if((t->family == family && t->model == model)
		|| (t->family == family && t->model == -1)
		|| (t->family == -1))
			break;
		t++;
	}
	m->cpuidtype = t->name;
	i8253init(t->aalcycles, t->family >= 5);

	/*
	 * If machine check exception or page size extensions are supported
	 * enable them in CR4 and clear any other set extensions.
	 * If machine check was enabled clear out any lingering status.
	 */
	if(m->cpuiddx & 0x88){
		cr4 = 0;
		if(m->cpuiddx & 0x08)
			cr4 |= 0x10;		/* page size extensions */
		if(m->cpuiddx & 0x80)
			cr4 |= 0x40;		/* machine check enable */
		putcr4(cr4);
		if(m->cpuiddx & 0x80)
			rdmsr(0x01, &mct);
	}

	cputype = t;
	return t->family;
}

static long
cputyperead(char *a, int n, ulong offset)
{
	char str[32];
	ulong mhz;

	mhz = (m->cpuhz+999999)/1000000;

	snprint(str, sizeof(str), "%s %lud\n", cputype->name, mhz);
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
	else{
		if(arch->id == 0)
			arch->id = archgeneric.id;
		if(arch->reset == 0)
			arch->reset = archgeneric.reset;
		if(arch->serialpower == 0)
			arch->serialpower = archgeneric.serialpower;
		if(arch->modempower == 0)
			arch->modempower = archgeneric.modempower;
	
		if(arch->intrinit == 0)
			arch->intrinit = archgeneric.intrinit;
		if(arch->intrenable == 0)
			arch->intrenable = archgeneric.intrenable;
	}

	/* pick the better timer */
	if(X86FAMILY(m->cpuidax) >= 5){
		cycletimerinit();
		arch->fastclock = cycletimer;
	}

	/*
	 * Decide whether to use copy-on-reference (386 and mp).
	 */
	if(X86FAMILY(m->cpuidax) == 3 || conf.nmach > 1)
		conf.copymode = 1;

	if(X86FAMILY(m->cpuidax) >= 5)
		coherence = wbflush;
}

void
cycletimerinit(void)
{
	wrmsr(0x10, 0);
	fasthz = m->cpuhz;
}

/*
 *  return the most precise clock we have
 */
uvlong
cycletimer(uvlong *hz)
{
	uvlong tsc;

	rdmsr(0x10, (vlong*)&tsc);
	m->fastclock = tsc;
	if(hz != nil)
		*hz = fasthz;
	return tsc;
}

vlong
fastticks(uvlong *hz)
{
	return (*arch->fastclock)(hz);
}
