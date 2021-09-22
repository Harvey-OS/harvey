/*
 * #P/cputype, and maybe access to 8080 I/O ports.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
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
	IOMap	*map;
	IOMap	*free;
	IOMap	maps[32];		// some initial free maps

	QLock	ql;			// lock for reading map
} iomap;

enum {
	Qdir = 0,
	Qioalloc = 1,
#ifdef IOPORTS					/* only used by vga */
	Qiob,
	Qiow,
	Qiol,
#endif
	Qbase,

	Qmax = 16,
};

typedef long Rdwrfn(Chan*, void*, long, vlong);

static Rdwrfn *readfn[Qmax];
static Rdwrfn *writefn[Qmax];

static Dirtab archdir[Qmax] = {
	".",		{ Qdir, 0, QTDIR },	0,	0555,
	"ioalloc",	{ Qioalloc, 0 },	0,	0444,
#ifdef IOPORTS
	"iob",		{ Qiob, 0 },		0,	0660,
	"iow",		{ Qiow, 0 },		0,	0660,
	"iol",		{ Qiol, 0 },		0,	0660,
#endif
};
Lock archwlock;	/* the lock is only for changing archdir */
int narchdir = Qbase;

extern Dev archdevtab;

/* complain, unless it's a known bug in this hypervisor */
void
vmbotch(ulong vmbit, char *cause)
{
	print("am i running in a VM, or is %s?\n", cause);
	USED(vmbit);
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

/*
 * Welcome to 1978.  Somehow Intel hadn't seen the PDP-11 yet.
 * In addition to a separate address space for devices (I/O ports),
 * they required special IN and OUT instructions.
 */
void
ioinit(void)
{
	int i, io_s, io_e;
	char *ends, *s;

	for(i = 0; i < nelem(iomap.maps)-1; i++)
		iomap.maps[i].next = &iomap.maps[i+1];
	iomap.maps[i].next = nil;
	iomap.free = iomap.maps;

	for (s = getconf("ioexclude"); s && *s != '\0' && *s != '\n'; s = ends){
		io_s = (int)strtol(s, &ends, 0);
		if (ends == nil || ends == s || *ends != '-') {
			print("ioinit: cannot parse ioexclude string\n");
			break;
		}
		s = ++ends;

		io_e = (int)strtol(s, &ends, 0);
		if (ends && *ends == ',')
			*ends++ = '\0';

		ioalloc(io_s, io_e - io_s + 1, 0, "pre-allocated");
	}
}

//
//	alloc some io port space and remember who it was
//	alloced to.  if port < 0, find a free region.
//
int
ioalloc(int port, int size, int align, char *tag)
{
	IOMap *map, **l;
	int i;

	lock(&iomap);
	if(port < 0){
		// find a free port above 0x400 and below 0x1000
		port = 0x400;
		for(l = &iomap.map; *l; l = &(*l)->next){
			map = *l;
			if (map->start < 0x400)
				continue;
			i = map->start - port;
			if(i > size)
				break;
			if(align > 0)
				port = ((port+align-1)/align)*align;
			else
				port = map->end;
		}
		if(*l == nil){
			unlock(&iomap);
			return -1;
		}
	} else {
		// Only 64KB I/O space on the x86.
		if((port+size) > 0x10000){
			unlock(&iomap);
			return -1;
		}
		// see if the space clashes with previously allocated ports
		for(l = &iomap.map; *l; l = &(*l)->next){
			map = *l;
			if(map->end <= port)
				continue;
			if(map->reserved && map->start == port &&
			    map->end == port + size) {
				map->reserved = 0;
				unlock(&iomap);
				return map->start;
			}
			if(map->start >= port+size)
				break;
			unlock(&iomap);
			return -1;
		}
	}
	map = iomap.free;
	if(map == nil){
		print("ioalloc: out of maps");
		unlock(&iomap);
		return port;
	}
	iomap.free = map->next;
	map->next = *l;
	map->start = port;
	map->end = port + size;
	strncpy(map->tag, tag, sizeof(map->tag));
	map->tag[sizeof(map->tag)-1] = 0;
	*l = map;

	archdir[0].qid.vers++;

	unlock(&iomap);
	return map->start;
}

void
iofree(int port)
{
	IOMap *map, **l;

	lock(&iomap);
	for(l = &iomap.map; *l; l = &(*l)->next){
		if((*l)->start == port){
			map = *l;
			*l = map->next;
			map->next = iomap.free;
			iomap.free = map;
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
	IOMap *map;

	for(map = iomap.map; map; map = map->next)
		if(start >= map->start && start < map->end
		|| start <= map->start && end > map->start)
			return 0;
	return 1;
}

static void
checkport(int start, int end)
{
	/* standard vga regs are OK */
	if(start >= 0x2b0 && end <= 0x2df+1 ||
	   start >= 0x3c0 && end <= 0x3da+1)
		return;
	if(!iounused(start, end))
		error(Eperm);
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
	char *buf, *p, *e;
#ifdef IOPORTS
	int port;
	ushort *sp;
	ulong *lp;
#endif
	IOMap *map;
	Rdwrfn *fn;

	switch((ulong)c->qid.path){

	case Qdir:
		return devdirread(c, a, n, archdir, narchdir, devgen);
#ifdef IOPORTS
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
#endif
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
	e = buf + n;
	n /= Linelen;
	offset /= Linelen;

	lock(&iomap);
	for(map = iomap.map; n > 0 && map != nil; map = map->next){
		if(offset-- > 0)
			continue;
		seprint(p, e, "%#8lux %#8lux %-12.12s\n",
			map->start, map->end-1, map->tag);
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
#ifdef IOPORTS
	char *p;
	int port;
	ushort *sp;
	ulong *lp;
#endif
	Rdwrfn *fn;

	switch((ulong)c->qid.path){
#ifdef IOPORTS
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
#endif
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

void (*coherence)(void) = mfence;

static long
cputyperead(Chan*, void *a, long n, vlong off)
{
	char str[32];

	snprint(str, sizeof(str), "%s %ud\n", "AMD64", m->cpumhz);
	return readstr(off, a, n, str);
}

void
archinit(void)
{
	addarchfile("cputype", 0444, cputyperead, nil);
}

void
archreset(void)
{
	ushort *s;

	s = KADDR(0x472);
	*s = 0x1234;		/* BIOS warm-boot flag examined at boot */
	wbinvd();

	/*
	 * And sometimes there is no keyboard...
	 *
	 * The reset register (0xcf9) is usually in one of the bridge
	 * chips. The actual location and sequence could be extracted from
	 * ACPI but why bother, this is the end of the line anyway.
	 */
	spllo();
	iprint("resetting.\n\n");
	delay(500);
	pcicf9reset();
	delay(500);

	iprint("no luck, idling.\n");
	for(;;)
		idlehands();
}

/*
 * secondary cpus should all idle (here or in shutdown, called from reboot).
 * for a true shutdown, reset and boot, cpu 0 should then reset itself or the
 * system as a whole.  if plan 9 is rebooting (loading a new kernel and
 * starting it), cpu 0 needs to keep running to do that work.
 */
void
mpshutdown(void)
{
	static QLock mpshutdownlock;

	if(m->machno == 0 && canqlock(&mpshutdownlock)) {  /* we are cpu0 */
		if(active.rebooting)
			return;
		iprint("mpshutdown: %d active cpus\n", sys->nonline);
		delay(500);
		splhi();
		apicresetothers();
		delay(1);			/* let secondary cpus reset */

		/* we can now use the uniprocessor reset */
		archreset();
	}
	splhi();
	wbinvd();
	for (;;)
		idlehands();
}

/*
 *  return value and speed of timer
 */
uvlong
fastticks(uvlong* hz)
{
	if(hz != nil)
		*hz = m->cpuhz;
	return rdtsc();
}

ulong
µs(void)
{
	return fastticks2us(rdtsc());
}

/*
 *  set next timer interrupt
 */
void
timerset(uvlong x)
{
	extern void apictimerset(uvlong);

	apictimerset(x);
}
