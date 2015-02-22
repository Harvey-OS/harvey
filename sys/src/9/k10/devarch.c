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
#include "../port/error.h"

#include "ureg.h"

typedef struct IOMap IOMap;
struct IOMap
{
	IOMap	*next;
	int	reserved;
	char	tag[13];
	uint32_t	start;
	uint32_t	end;
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
	Qiob,
	Qiow,
	Qiol,
	Qbase,
	Qmapram,

	Qmax = 16,
};

typedef int32_t Rdwrfn(Chan*, void*, int32_t, int64_t);

static Rdwrfn *readfn[Qmax];
static Rdwrfn *writefn[Qmax];

static Dirtab archdir[Qmax] = {
	".",		{ Qdir, 0, QTDIR },	0,	0555,
	"ioalloc",	{ Qioalloc, 0 },	0,	0444,
	"iob",		{ Qiob, 0 },		0,	0660,
	"iow",		{ Qiow, 0 },		0,	0660,
	"iol",		{ Qiol, 0 },		0,	0660,
	"mapram",	{ Qmapram, 0 },	0,	0444,
};
Lock archwlock;	/* the lock is only for changing archdir */
int narchdir = Qbase;

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
	 * Someone needs to explain why this was here...
	 */
	ioalloc(0x0fff, 1, 0, "dummy");	// i82557 is at 0x1000, the dummy
					// entry is needed for swappable devs.

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

// Reserve a range to be ioalloced later.
// This is in particular useful for exchangable cards, such
// as pcmcia and cardbus cards.
int
ioreserve(int, int size, int align, char *tag)
{
	IOMap *map, **l;
	int i, port;

	lock(&iomap);
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
	map->reserved = 1;
	strncpy(map->tag, tag, sizeof(map->tag));
	map->tag[sizeof(map->tag)-1] = 0;
	*l = map;

	archdir[0].qid.vers++;

	unlock(&iomap);
	return map->start;
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
			if(map->reserved && map->start == port && map->end == port + size) {
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

	for(map = iomap.map; map; map = map->next){
		if(start >= map->start && start < map->end
		|| start <= map->start && end > map->start)
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

static int32_t
archstat(Chan* c, uint8_t* dp, int32_t n)
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
archread(Chan *c, void *a, long n, int64_t offset)
{
	char *buf, *p;
	int port;
	ushort *sp;
	uint32_t *lp;
	IOMap *map;
	Rdwrfn *fn;

	switch((uint32_t)c->qid.path){

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
	n = n/Linelen;
	offset = offset/Linelen;

	switch((uint32_t)c->qid.path){
	case Qioalloc:
		lock(&iomap);
		for(map = iomap.map; n > 0 && map != nil; map = map->next){
			if(offset-- > 0)
				continue;
			sprint(p, "%#8lux %#8lux %-12.12s\n", map->start, map->end-1, map->tag);
			p += Linelen;
			n--;
		}
		unlock(&iomap);
		break;
	case Qmapram:
/* shit */
#ifdef NOTYET
		for(mp = rmapram.map; mp->size; mp++){
			/*
			 * Up to MemMinMiB is already set up.
			 */
			if(mp->addr < MemMinMiB*MiB){
				if(mp->addr+mp->size <= MemMinMiB*MiB)
					continue;
				pa = MemMinMiB*MiB;
				size = mp->size - MemMinMiB*MiB-mp->addr;
			}
			else{
				pa = mp->addr;
				size = mp->size;
			}
#endif
		error("Not yet");
	
		break;
	}

	n = p - buf;
	memmove(a, buf, n);
	free(buf);

	return n;
}

static int32_t
archwrite(Chan *c, void *a, int32_t n, int64_t offset)
{
	char *p;
	int port;
	uint16_t *sp;
	uint32_t *lp;
	Rdwrfn *fn;

	switch((uint32_t)c->qid.path){

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
 */
void
nop(void)
{
}

void (*coherence)(void) = mfence;

static int32_t
cputyperead(Chan*, void *a, int32_t n, int64_t off)
{
	char buf[512], *s, *e;
	int i, k;

	e = buf+sizeof buf;
	s = seprint(buf, e, "%s %ud\n", "AMD64", m->cpumhz);
	k = m->ncpuinfoe - m->ncpuinfos;
	if(k > 4)
		k = 4;
	for(i = 0; i < k; i++)
		s = seprint(s, e, "%#8.8ux %#8.8ux %#8.8ux %#8.8ux\n",
			m->cpuinfo[i][0], m->cpuinfo[i][1],
			m->cpuinfo[i][2], m->cpuinfo[i][3]);
	return readstr(off, a, n, buf);
}

void
archinit(void)
{
	addarchfile("cputype", 0444, cputyperead, nil);
}

void
archreset(void)
{
	int i;

	/*
	 * And sometimes there is no keyboard...
	 *
	 * The reset register (0xcf9) is usually in one of the bridge
	 * chips. The actual location and sequence could be extracted from
	 * ACPI but why bother, this is the end of the line anyway.
	print("Takes a licking and keeps on ticking...\n");
	 */
	i = inb(0xcf9);					/* ICHx reset control */
	i &= 0x06;
	outb(0xcf9, i|0x02);				/* SYS_RST */
	millidelay(1);
	outb(0xcf9, i|0x06);				/* RST_CPU transition */

	for(;;)
		;
}

/*
 *  return value and speed of timer
 */
uint64_t
fastticks(uint64_t* hz)
{
	if(hz != nil)
		*hz = m->cpuhz;
	return rdtsc();
}

uint32_t
µs(void)
{
	return fastticks2us(rdtsc());
}

/*
 *  set next timer interrupt
 */
void
timerset(uint64_t x)
{
	extern void apictimerset(uint64_t);

	apictimerset(x);
}

void
cycles(uint64_t* t)
{
	*t = rdtsc();
}

void
delay(int millisecs)
{
	uint64_t r, t;

	if(millisecs <= 0)
		millisecs = 1;
	r = rdtsc();
	for(t = r + m->cpumhz*1000ull*millisecs; r < t; r = rdtsc())
		;
}

/*  
 *  performance measurement ticks.  must be low overhead.
 *  doesn't have to count over a second.
 */
uint32_t
perfticks(void)
{
	uint64_t x;

//	if(m->havetsc)
		cycles(&x);
//	else
//		x = 0;
	return x;
}
