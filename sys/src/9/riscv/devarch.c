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

/* leave this for now; we might want to keep track of MMIO apart from memory. */
typedef struct IOMap IOMap;
struct IOMap
{
	IOMap	*next;
	int	reserved;
	char	tag[13];
	uintptr_t	start;
	uintptr_t	end;
};

static struct
{
	Lock l;
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
	/* NOTE: kludge until we have real permissions. */
	"iob",		{ Qiob, 0 },		0,	0660 | 6,
	"iow",		{ Qiow, 0 },		0,	0660 | 6,
	"iol",		{ Qiol, 0 },		0,	0660 | 6,
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
	int i;

	for(i = 0; i < nelem(iomap.maps)-1; i++)
		iomap.maps[i].next = &iomap.maps[i+1];
	iomap.maps[i].next = nil;
	iomap.free = iomap.maps;
}

// Reserve a range to be ioalloced later.
// This is in particular useful for exchangable cards, such
// as pcmcia and cardbus cards.
int
ioreserve(int n, int size, int align, char *tag)
{
	panic("ioreserve");
#if 0
	IOMap *map, **l;
	int i, port;

	lock(&iomap.l);
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
		unlock(&iomap.l);
		return -1;
	}
	map = iomap.free;
	if(map == nil){
		print("ioalloc: out of maps");
		unlock(&iomap.l);
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

	unlock(&iomap.l);
	return map->start;
#endif
	return 0;
}

//
//	alloc some io port space and remember who it was
//	alloced to.  if port < 0, find a free region.
//
int
ioalloc(int port, int size, int align, char *tag)
{
	panic("ioalloc");
#if 0
	IOMap *map, **l;
	int i;

	lock(&iomap.l);
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
			unlock(&iomap.l);
			return -1;
		}
	} else {
		// Only 64KB I/O space on the x86.
		if((port+size) > 0x10000){
			unlock(&iomap.l);
			return -1;
		}
		// see if the space clashes with previously allocated ports
		for(l = &iomap.map; *l; l = &(*l)->next){
			map = *l;
			if(map->end <= port)
				continue;
			if(map->reserved && map->start == port && map->end == port + size) {
				map->reserved = 0;
				unlock(&iomap.l);
				return map->start;
			}
			if(map->start >= port+size)
				break;
			unlock(&iomap.l);
			return -1;
		}
	}
	map = iomap.free;
	if(map == nil){
		print("ioalloc: out of maps");
		unlock(&iomap.l);
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

	unlock(&iomap.l);
	return map->start;
#endif
	return 0;
}

void
iofree(int port)
{
	panic("iofree");
#if 0
	IOMap *map, **l;

	lock(&iomap.l);
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
	unlock(&iomap.l);
#endif
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

#if 0
static void
checkport(int start, int end)
{
	if(iounused(start, end))
		return;
	error(Eperm);
}
#endif

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
archclose(Chan* c)
{
}

enum
{
	Linelen= 31,
};

static int32_t
archread(Chan *c, void *a, int32_t n, int64_t offset)
{
	char *buf, *p;
	//int port;
	//uint16_t *sp;
	//uint32_t *lp;
	IOMap *map;
	Rdwrfn *fn;

	switch((uint32_t)c->qid.path){

	case Qdir:
		return devdirread(c, a, n, archdir, narchdir, devgen);

#if 0
// not now, not ever?
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
	n = n/Linelen;
	offset = offset/Linelen;

	switch((uint32_t)c->qid.path){
	case Qioalloc:
		lock(&iomap.l);
		for(map = iomap.map; n > 0 && map != nil; map = map->next){
			if(offset-- > 0)
				continue;
			sprint(p, "%#8lx %#8lx %-12.12s\n", map->start, map->end-1, map->tag);
			p += Linelen;
			n--;
		}
		unlock(&iomap.l);
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
	//char *p;
	//int port;
	//uint16_t *sp;
	//uint32_t *lp;
	Rdwrfn *fn;

	switch((uint32_t)c->qid.path){
#if 0

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
		break;
	}
	return 0;
}

Dev archdevtab = {
	.dc = 'P',
	.name = "arch",

	.reset = devreset,
	.init = devinit,
	.shutdown = devshutdown,
	.attach = archattach,
	.walk = archwalk,
	.stat = archstat,
	.open = archopen,
	.create = devcreate,
	.close = archclose,
	.read = archread,
	.bread = devbread,
	.write = archwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};

/*
 */
void
nop(void)
{
}

void (*coherence)(void) = mfence;

static int32_t
cputyperead(Chan* c, void *a, int32_t n, int64_t off)
{
	return readstr(off, a, n, "riscv");
}

static int32_t
numcoresread(Chan* c, void *a, int32_t n, int64_t off)
{
        char buf[8];
        snprint(buf, 8, "%d\n", sys->nmach);
        return readstr(off, a, n, buf);
}

void
archinit(void)
{
	addarchfile("cputype", 0444, cputyperead, nil);
	addarchfile("numcores", 0444, numcoresread, nil);
}

void
archreset(void)
{
	panic("archreset");

}

/*
 *  return value and speed of timer
 */
uint64_t
fastticks(uint64_t* hz)
{
	if(hz != nil)
		*hz = machp()->cpuhz;
	return rdtsc();
}

uint32_t
ms(void)
{
	return fastticks2us(rdtsc());
}

/*
 *  set next timer interrupt
 */
void
timerset(uint64_t x)
{
	panic("apictimerset");
//	extern void apictimerset(uint64_t);

//	apictimerset(x);
}

void
delay(int millisecs)
{
	uint64_t r, t;

	if(millisecs <= 0)
		millisecs = 1;
	cycles(&r);
	for(t = r + (sys->cyclefreq*millisecs)/1000ull; r < t; cycles(&r))
		;
}

/*
 *  performance measurement ticks.  must be low overhead.
 *  doesn't have to count over a second.
 */
uint64_t
perfticks(void)
{
	uint64_t x;

//	if(m->havetsc)
		cycles(&x);
//	else
//		x = 0;
	return x;
}
