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

typedef struct Cpuflag {
	const char *name; /* short name (linux-like)*/
	u32 eax;	  /* input eax */
	u8 infoidx;  /* index of info result */
	u8 bitidx;	  /* feature bit in info result */
} Cpuflag;

// Below infoidxs equate to: 0=eax 1=ebx 2=ecx 3=edx
Cpuflag cpuflags[] = {
	/* name				eax 		info 	bit */
	{
		"fpu",
		0x00000001,
		3,
		0,
	},
	{
		"vme",
		0x00000001,
		3,
		1,
	},
	{
		"de",
		0x00000001,
		3,
		2,
	},
	{
		"pse",
		0x00000001,
		3,
		3,
	},
	{
		"tsc",
		0x00000001,
		3,
		4,
	},
	{
		"msr",
		0x00000001,
		3,
		5,
	},
	{
		"pae",
		0x00000001,
		3,
		6,
	},
	{
		"mce",
		0x00000001,
		3,
		7,
	},
	{
		"cx8",
		0x00000001,
		3,
		8,
	},
	{
		"apic",
		0x00000001,
		3,
		9,
	},
	{
		"sep",
		0x00000001,
		3,
		11,
	},
	{
		"mtrr",
		0x00000001,
		3,
		12,
	},
	{
		"pge",
		0x00000001,
		3,
		13,
	},
	{
		"mca",
		0x00000001,
		3,
		14,
	},
	{
		"cmov",
		0x00000001,
		3,
		15,
	},
	{
		"pat",
		0x00000001,
		3,
		16,
	},
	{
		"pse36",
		0x00000001,
		3,
		17,
	},
	{
		"pn",
		0x00000001,
		3,
		18,
	},
	{
		"clflush",
		0x00000001,
		3,
		19,
	},
	{
		"dts",
		0x00000001,
		3,
		21,
	},
	{
		"acpi",
		0x00000001,
		3,
		22,
	},
	{
		"mmx",
		0x00000001,
		3,
		23,
	},
	{
		"fxsr",
		0x00000001,
		3,
		24,
	},
	{
		"sse",
		0x00000001,
		3,
		25,
	},
	{
		"sse2",
		0x00000001,
		3,
		26,
	},
	{
		"ss",
		0x00000001,
		3,
		27,
	},
	{
		"ht",
		0x00000001,
		3,
		28,
	},
	{
		"tm",
		0x00000001,
		3,
		29,
	},
	{
		"ia64",
		0x00000001,
		3,
		30,
	},
	{
		"pbe",
		0x00000001,
		3,
		31,
	},
	{
		"pni",
		0x00000001,
		2,
		0,
	},
	{
		"pclmulqdq",
		0x00000001,
		2,
		1,
	},
	{
		"dtes64",
		0x00000001,
		2,
		2,
	},
	{
		"monitor",
		0x00000001,
		2,
		3,
	},
	{
		"ds_cpl",
		0x00000001,
		2,
		4,
	},
	{
		"vmx",
		0x00000001,
		2,
		5,
	},
	{
		"smx",
		0x00000001,
		2,
		6,
	},
	{
		"est",
		0x00000001,
		2,
		7,
	},
	{
		"tm2",
		0x00000001,
		2,
		8,
	},
	{
		"ssse3",
		0x00000001,
		2,
		9,
	},
	{
		"cid",
		0x00000001,
		2,
		10,
	},
	{
		"sdbg",
		0x00000001,
		2,
		11,
	},
	{
		"fma",
		0x00000001,
		2,
		12,
	},
	{
		"cx16",
		0x00000001,
		2,
		13,
	},
	{
		"xtpr",
		0x00000001,
		2,
		14,
	},
	{
		"pdcm",
		0x00000001,
		2,
		15,
	},
	{
		"pcid",
		0x00000001,
		2,
		17,
	},
	{
		"dca",
		0x00000001,
		2,
		18,
	},
	{
		"sse4_1",
		0x00000001,
		2,
		19,
	},
	{
		"sse4_2",
		0x00000001,
		2,
		20,
	},
	{
		"x2apic",
		0x00000001,
		2,
		21,
	},
	{
		"movbe",
		0x00000001,
		2,
		22,
	},
	{
		"popcnt",
		0x00000001,
		2,
		23,
	},
	{
		"tsc_deadline_timer",
		0x00000001,
		2,
		24,
	},
	{
		"aes",
		0x00000001,
		2,
		25,
	},
	{
		"xsave",
		0x00000001,
		2,
		26,
	},
	{
		"osxsave",
		0x00000001,
		2,
		27,
	},
	{
		"avx",
		0x00000001,
		2,
		28,
	},
	{
		"f16c",
		0x00000001,
		2,
		29,
	},
	{
		"rdrand",
		0x00000001,
		2,
		30,
	},
	{
		"hypervisor",
		0x00000001,
		2,
		31,
	},
	{
		"lahf_lm",
		0x80000001,
		2,
		0,
	},
	{
		"cmp_legacy",
		0x80000001,
		2,
		1,
	},
	{
		"svm",
		0x80000001,
		2,
		2,
	},
	{
		"extapic",
		0x80000001,
		2,
		3,
	},
	{
		"cr8_legacy",
		0x80000001,
		2,
		4,
	},
	{
		"abm",
		0x80000001,
		2,
		5,
	},
	{
		"sse4a",
		0x80000001,
		2,
		6,
	},
	{
		"misalignsse",
		0x80000001,
		2,
		7,
	},
	{
		"3dnowprefetch",
		0x80000001,
		2,
		8,
	},
	{
		"osvw",
		0x80000001,
		2,
		9,
	},
	{
		"ibs",
		0x80000001,
		2,
		10,
	},
	{
		"xop",
		0x80000001,
		2,
		11,
	},
	{
		"skinit",
		0x80000001,
		2,
		12,
	},
	{
		"wdt",
		0x80000001,
		2,
		13,
	},
	{
		"lwp",
		0x80000001,
		2,
		15,
	},
	{
		"fma4",
		0x80000001,
		2,
		16,
	},
	{
		"tce",
		0x80000001,
		2,
		17,
	},
	{
		"nodeid_msr",
		0x80000001,
		2,
		19,
	},
	{
		"tbm",
		0x80000001,
		2,
		21,
	},
	{
		"topoext",
		0x80000001,
		2,
		22,
	},
	{
		"perfctr_core",
		0x80000001,
		2,
		23,
	},
	{
		"perfctr_nb",
		0x80000001,
		2,
		24,
	},
	{
		"bpext",
		0x80000001,
		2,
		26,
	},
	{
		"ptsc",
		0x80000001,
		2,
		27,
	},
	{
		"perfctr_llc",
		0x80000001,
		2,
		28,
	},
	{
		"mwaitx",
		0x80000001,
		2,
		29,
	},
	{
		"fsgsbase",
		0x00000007,
		1,
		0,
	},
	{
		"tsc_adjust",
		0x00000007,
		1,
		1,
	},
	{
		"bmi1",
		0x00000007,
		1,
		3,
	},
	{
		"hle",
		0x00000007,
		1,
		4,
	},
	{
		"avx2",
		0x00000007,
		1,
		5,
	},
	{
		"smep",
		0x00000007,
		1,
		7,
	},
	{
		"bmi2",
		0x00000007,
		1,
		8,
	},
	{
		"erms",
		0x00000007,
		1,
		9,
	},
	{
		"invpcid",
		0x00000007,
		1,
		10,
	},
	{
		"rtm",
		0x00000007,
		1,
		11,
	},
	{
		"cqm",
		0x00000007,
		1,
		12,
	},
	{
		"mpx",
		0x00000007,
		1,
		14,
	},
	{
		"rdt_a",
		0x00000007,
		1,
		15,
	},
	{
		"avx512f",
		0x00000007,
		1,
		16,
	},
	{
		"avx512dq",
		0x00000007,
		1,
		17,
	},
	{
		"rdseed",
		0x00000007,
		1,
		18,
	},
	{
		"adx",
		0x00000007,
		1,
		19,
	},
	{
		"smap",
		0x00000007,
		1,
		20,
	},
	{
		"avx512ifma",
		0x00000007,
		1,
		21,
	},
	{
		"clflushopt",
		0x00000007,
		1,
		23,
	},
	{
		"clwb",
		0x00000007,
		1,
		24,
	},
	{
		"intel_pt",
		0x00000007,
		1,
		25,
	},
	{
		"avx512pf",
		0x00000007,
		1,
		26,
	},
	{
		"avx512er",
		0x00000007,
		1,
		27,
	},
	{
		"avx512cd",
		0x00000007,
		1,
		28,
	},
	{
		"sha_ni",
		0x00000007,
		1,
		29,
	},
	{
		"avx512bw",
		0x00000007,
		1,
		30,
	},
	{
		"avx512vl",
		0x00000007,
		1,
		31,
	},
	{
		"xsaveopt",
		0x0000000d,
		0,
		0,
	},
	{
		"xsavec",
		0x0000000d,
		0,
		1,
	},
	{
		"xgetbv1",
		0x0000000d,
		0,
		2,
	},
	{
		"xsaves",
		0x0000000d,
		0,
		3,
	},
	{
		"cqm_llc",
		0x0000000f,
		3,
		1,
	},
	{
		"cqm_occup_llc",
		0x0000000f,
		3,
		0,
	},
	{
		"cqm_mbm_total",
		0x0000000f,
		3,
		1,
	},
	{
		"cqm_mbm_local",
		0x0000000f,
		3,
		2,
	},
	{
		"dtherm",
		0x00000006,
		1,
		0,
	},
	{
		"ida",
		0x00000006,
		1,
		1,
	},
	{
		"arat",
		0x00000006,
		1,
		2,
	},
	{
		"pln",
		0x00000006,
		1,
		4,
	},
	{
		"pts",
		0x00000006,
		1,
		6,
	},
	{
		"hwp",
		0x00000006,
		1,
		7,
	},
	{
		"hwp_notify",
		0x00000006,
		1,
		8,
	},
	{
		"hwp_act_window",
		0x00000006,
		1,
		9,
	},
	{
		"hwp_epp",
		0x00000006,
		1,
		10,
	},
	{
		"hwp_pkg_req",
		0x00000006,
		1,
		11,
	},
	{
		"avx512vbmi",
		0x00000007,
		2,
		1,
	},
	{
		"umip",
		0x00000007,
		2,
		2,
	},
	{
		"pku",
		0x00000007,
		2,
		3,
	},
	{
		"ospke",
		0x00000007,
		2,
		4,
	},
	{
		"avx512_vbmi2",
		0x00000007,
		2,
		6,
	},
	{
		"gfni",
		0x00000007,
		2,
		8,
	},
	{
		"vaes",
		0x00000007,
		2,
		9,
	},
	{
		"vpclmulqdq",
		0x00000007,
		2,
		10,
	},
	{
		"avx512_vnni",
		0x00000007,
		2,
		11,
	},
	{
		"avx512_bitalg",
		0x00000007,
		2,
		12,
	},
	{
		"tme",
		0x00000007,
		2,
		13,
	},
	{
		"avx512_vpopcntdq",
		0x00000007,
		2,
		14,
	},
	{
		"la57",
		0x00000007,
		2,
		16,
	},
	{
		"rdpid",
		0x00000007,
		2,
		22,
	},
	{
		"cldemote",
		0x00000007,
		2,
		25,
	},
	{
		"movdiri",
		0x00000007,
		2,
		27,
	},
	{
		"movdir64b",
		0x00000007,
		2,
		28,
	},
	{
		"avx512_4vnniw",
		0x00000007,
		3,
		2,
	},
	{
		"avx512_4fmaps",
		0x00000007,
		3,
		3,
	},
	{
		"tsx_force_abort",
		0x00000007,
		3,
		13,
	},
	{
		"pconfig",
		0x00000007,
		3,
		18,
	},
	{
		"spec_ctrl",
		0x00000007,
		3,
		26,
	},
	{
		"intel_stibp",
		0x00000007,
		3,
		27,
	},
	{
		"flush_l1d",
		0x00000007,
		3,
		28,
	},
	{
		"arch_capabilities",
		0x00000007,
		3,
		29,
	},
	{
		"spec_ctrl_ssbd",
		0x00000007,
		3,
		31,
	},
};

typedef struct IOMap IOMap;
struct IOMap {
	IOMap *next;
	int reserved;
	char tag[13];
	u32 start;
	u32 end;
};

static struct
{
	Lock l;
	IOMap *map;
	IOMap *free;
	IOMap maps[32];	       // some initial free maps

	QLock ql;	 // lock for reading map
} iomap;

enum {
	Qdir = 0,
	Qioalloc = 1,
	Qiob,
	Qiow,
	Qiol,
	Qbase,
	Qmapram,

	Qmax = 32,
};

typedef i32 Rdwrfn(Chan *, void *, i32, i64);

static Rdwrfn *readfn[Qmax];
static Rdwrfn *writefn[Qmax];

static Dirtab archdir[Qmax] = {
	{".", {Qdir, 0, QTDIR}, 0, 0555},
	{"ioalloc", {Qioalloc, 0}, 0, 0444},
	/* NOTE: kludge until we have real permissions. */
	{"iob", {Qiob, 0}, 0, 0660 | 6},
	{"iow", {Qiow, 0}, 0, 0660 | 6},
	{"iol", {Qiol, 0}, 0, 0660 | 6},
	{"mapram", {Qmapram, 0}, 0, 0444},
};
Lock archwlock; /* the lock is only for changing archdir */
int narchdir = Qbase;

/*
 * Add a file to the #P listing.  Once added, you can't delete it.
 * You can't add a file with the same name as one already there,
 * and you get a pointer to the Dirtab entry so you can do things
 * like change the Qid version.  Changing the Qid path is disallowed.
 */
Dirtab *
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
		panic("addarchfile: %s: no more slots available: increase Qmax");
	}

	for(i = 0; i < narchdir; i++)
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

	for(i = 0; i < nelem(iomap.maps) - 1; i++)
		iomap.maps[i].next = &iomap.maps[i + 1];
	iomap.maps[i].next = nil;
	iomap.free = iomap.maps;

	/*
	 * Someone needs to explain why this was here...
	 */
	ioalloc(0x0fff, 1, 0, "dummy");	       // i82557 is at 0x1000, the dummy
					       // entry is needed for swappable devs.

	if(0) {	       // (excluded = getconf("ioexclude")) != nil){
		char *s;

		s = excluded;
		while(s && *s != '\0' && *s != '\n'){
			char *ends;
			int io_s, io_e;

			io_s = (int)strtol(s, &ends, 0);
			if(ends == nil || ends == s || *ends != '-'){
				print("ioinit: cannot parse option string\n");
				break;
			}
			s = ++ends;

			io_e = (int)strtol(s, &ends, 0);
			if(ends && *ends == ',')
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
ioreserve(int n, int size, int align, char *tag)
{
	IOMap *map, **l;
	int i, port;

	lock(&iomap.l);
	// find a free port above 0x400 and below 0x1000
	port = 0x400;
	for(l = &iomap.map; *l; l = &(*l)->next){
		map = *l;
		if(map->start < 0x400)
			continue;
		i = map->start - port;
		if(i > size)
			break;
		if(align > 0)
			port = ((port + align - 1) / align) * align;
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
	map->tag[sizeof(map->tag) - 1] = 0;
	*l = map;

	archdir[0].qid.vers++;

	unlock(&iomap.l);
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

	lock(&iomap.l);
	if(port < 0){
		// find a free port above 0x400 and below 0x1000
		port = 0x400;
		for(l = &iomap.map; *l; l = &(*l)->next){
			map = *l;
			if(map->start < 0x400)
				continue;
			i = map->start - port;
			if(i > size)
				break;
			if(align > 0)
				port = ((port + align - 1) / align) * align;
			else
				port = map->end;
		}
		if(*l == nil){
			unlock(&iomap.l);
			return -1;
		}
	} else {
		// Only 64KB I/O space on the x86.
		if((port + size) > 0x10000){
			unlock(&iomap.l);
			return -1;
		}
		// see if the space clashes with previously allocated ports
		for(l = &iomap.map; *l; l = &(*l)->next){
			map = *l;
			if(map->end <= port)
				continue;
			if(map->reserved && map->start == port && map->end == port + size){
				map->reserved = 0;
				unlock(&iomap.l);
				return map->start;
			}
			if(map->start >= port + size)
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
	map->tag[sizeof(map->tag) - 1] = 0;
	*l = map;

	archdir[0].qid.vers++;

	unlock(&iomap.l);
	return map->start;
}

void
iofree(int port)
{
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
}

int
iounused(int start, int end)
{
	IOMap *map;

	for(map = iomap.map; map != nil; map = map->next){
		if((start >= map->start && start < map->end) || (start <= map->start && end > map->start))
			return 0;
	}
	return 1;
}

static void
checkport(int start, int end)
{
	/* standard vga regs are OK */
	if(start >= 0x2b0 && end <= 0x2df + 1)
		return;
	if(start >= 0x3c0 && end <= 0x3da + 1)
		return;

	if(iounused(start, end))
		return;
	error(Eperm);
}

static Chan *
archattach(char *spec)
{
	return devattach('P', spec);
}

Walkqid *
archwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, archdir, narchdir, devgen);
}

static i32
archstat(Chan *c, u8 *dp, i32 n)
{
	return devstat(c, dp, n, archdir, narchdir, devgen);
}

static Chan *
archopen(Chan *c, int omode)
{
	return devopen(c, omode, archdir, narchdir, devgen);
}

static void
archclose(Chan *c)
{
}

enum {
	Linelen = 31,
};

static i32
archread(Chan *c, void *a, i32 n, i64 offset)
{
	char *buf, *p;
	int port;
	u16 *sp;
	u32 *lp;
	IOMap *map;
	Rdwrfn *fn;

	switch((u32)c->qid.path){

	case Qdir:
		return devdirread(c, a, n, archdir, narchdir, devgen);

	case Qiob:
		port = offset;
		checkport(offset, offset + n);
		for(p = a; port < offset + n; port++)
			*p++ = inb(port);
		return n;

	case Qiow:
		if(n & 1)
			error(Ebadarg);
		checkport(offset, offset + n);
		sp = a;
		for(port = offset; port < offset + n; port += 2)
			*sp++ = ins(port);
		return n;

	case Qiol:
		if(n & 3)
			error(Ebadarg);
		checkport(offset, offset + n);
		lp = a;
		for(port = offset; port < offset + n; port += 4)
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
	n = n / Linelen;
	offset = offset / Linelen;

	switch((u32)c->qid.path){
	case Qioalloc:
		lock(&iomap.l);
		for(map = iomap.map; n > 0 && map != nil; map = map->next){
			if(offset-- > 0)
				continue;
			sprint(p, "%#8lx %#8lx %-12.12s\n", map->start, map->end - 1, map->tag);
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
			if(mp->addr < MemMinMiB * MiB){
				if(mp->addr + mp->size <= MemMinMiB * MiB)
					continue;
				pa = MemMinMiB * MiB;
				size = mp->size - MemMinMiB * MiB - mp->addr;
			} else {
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

static i32
archwrite(Chan *c, void *a, i32 n, i64 offset)
{
	char *p;
	int port;
	u16 *sp;
	u32 *lp;
	Rdwrfn *fn;

	switch((u32)c->qid.path){

	case Qiob:
		p = a;
		checkport(offset, offset + n);
		for(port = offset; port < offset + n; port++)
			outb(port, *p++);
		return n;

	case Qiow:
		if(n & 1)
			error(Ebadarg);
		checkport(offset, offset + n);
		sp = a;
		for(port = offset; port < offset + n; port += 2)
			outs(port, *sp++);
		return n;

	case Qiol:
		if(n & 3)
			error(Ebadarg);
		checkport(offset, offset + n);
		lp = a;
		for(port = offset; port < offset + n; port += 4)
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

static i32
cputyperead(Chan *c, void *a, i32 n, i64 off)
{
	char buf[512], *s, *e;
	char *vendorid;
	u32 info0[4];

	s = buf;
	e = buf + sizeof buf;

	if(!cpuidinfo(0, 0, info0)){
		iprint("cputyperead: cpuidinfo(0, 0) failed, switching to default\n");
		vendorid = "Generic X86_64";
	} else
		vendorid = cpuidname(info0);

	s = seprint(s, e, "%s CPU @ %uMHz\ncpu cores: %d\n", vendorid, machp()->cpumhz, sys->nmach);

	return readstr(off, a, n, buf);
}

static void
get_cpuid_limits(int *num_basic, int *num_hypervisor, int *num_extended)
{
	u32 info[4];

	*num_basic = 0;
	*num_hypervisor = 0;
	*num_extended = 0;

	if(cpuid(0x00000000, 0, info)){
		*num_basic = info[0] + 1;
	}
	if(cpuid(0x40000000, 0, info)){
		*num_hypervisor = info[0] - 0x40000000 + 1;
	}
	if(cpuid(0x80000000, 0, info)){
		*num_extended = info[0] - 0x80000000 + 1;
	}
}

// Given an index into the valid cpuids, and the number of each range of values,
// return the appropriate EAX value.
static i32
itoeax(int i, u32 num_basic, u32 num_hyp, u32 num_ext)
{
	u32 first_hyp = num_basic;
	u32 first_ext = num_basic + num_hyp;
	if(i >= first_ext){
		return 0x80000000 + i - first_ext;
	} else if(i >= first_hyp){
		return 0x40000000 + i - first_hyp;
	} else {
		return i;
	}
}

// Output hex values of all valid cpuid values
static i32
cpuidrawread(Chan *c, void *a, i32 n, i64 off)
{
	char buf[4096];
	char *s = buf;
	char *e = buf + sizeof buf;
	u32 info[4];

	int num_basic = 0, num_hyp = 0, num_ext = 0;
	get_cpuid_limits(&num_basic, &num_hyp, &num_ext);

	for(int i = 0; i < num_basic + num_hyp + num_ext; i++){
		u32 eax = itoeax(i, num_basic, num_hyp, num_ext);
		if(!cpuid(eax, 0, info)){
			continue;
		}
		s = seprint(s, e, "%#8.8x %#8.8x %#8.8x %#8.8x %#8.8x %#8.8x\n",
			    eax, 0, info[0], info[1], info[2], info[3]);
	}

	return readstr(off, a, n, buf);
}

// Output cpu flag shortnames from cpuid values
static i32
cpuidflagsread(Chan *c, void *a, i32 n, i64 off)
{
	char buf[4096];
	char *s = buf;
	char *e = buf + sizeof buf;
	u32 info[4];

	int num_basic = 0, num_hyp = 0, num_ext = 0;
	get_cpuid_limits(&num_basic, &num_hyp, &num_ext);

	int num_flags = nelem(cpuflags);

	for(int i = 0; i < num_basic + num_hyp + num_ext; i++){
		u32 eax = itoeax(i, num_basic, num_hyp, num_ext);
		if(!cpuid(eax, 0, info)){
			continue;
		}

		// Extract any flag names if this particular eax contains flags
		for(int fi = 0; fi < num_flags; fi++){
			Cpuflag *flag = &cpuflags[fi];
			if(flag->eax != eax){
				continue;
			}

			if(info[flag->infoidx] & (1 << flag->bitidx)){
				s = seprint(s, e, "%s ", flag->name);
			}
		}
	}

	s = seprint(s, e, "\n");

	return readstr(off, a, n, buf);
}

void
archinit(void)
{
	addarchfile("cputype", 0444, cputyperead, nil);
	addarchfile("mtags", 0444, mtagsread, nil);
	addarchfile("cpuidraw", 0444, cpuidrawread, nil);
	addarchfile("cpuidflags", 0444, cpuidflagsread, nil);
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
	i = inb(0xcf9); /* ICHx reset control */
	i &= 0x06;
	outb(0xcf9, i | 0x02); /* SYS_RST */
	millidelay(1);
	outb(0xcf9, i | 0x06); /* RST_CPU transition */

	for(;;)
		;
}

/*
 *  return value and speed of timer
 */
u64
fastticks(u64 *hz)
{
	if(hz != nil)
		*hz = machp()->cpuhz;
	return rdtsc();
}

u32
ms(void)
{
	return fastticks2us(rdtsc());
}

/*
 *  set next timer interrupt
 */
void
timerset(u64 x)
{
	extern void apictimerset(u64);

	apictimerset(x);
}

void
cycles(u64 *t)
{
	*t = rdtsc();
}

void
delay(int millisecs)
{
	u64 r, t;

	if(millisecs <= 0)
		millisecs = 1;
	r = rdtsc();
	for(t = r + (sys->cyclefreq * millisecs) / 1000ull; r < t; r = rdtsc())
		;
}

/*
 *  performance measurement ticks.  must be low overhead.
 *  doesn't have to count over a second.
 */
u64
perfticks(void)
{
	u64 x;

	//	if(m->havetsc)
	cycles(&x);
	//	else
	//		x = 0;
	return x;
}
