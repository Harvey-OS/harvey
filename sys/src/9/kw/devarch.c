#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"

#include "../ip/ip.h"

enum {
	Qdir = 0,
	Qbase,

	Qmax = 16,
};

typedef long Rdwrfn(Chan*, void*, long, vlong);

static Rdwrfn *readfn[Qmax];
static Rdwrfn *writefn[Qmax];

static Dirtab archdir[Qmax] = {
	".",		{ Qdir, 0, QTDIR },	0,	0555,
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
		break;
	}

	return 0;
}

static long
archwrite(Chan *c, void *a, long n, vlong offset)
{
	Rdwrfn *fn;

	if(c->qid.path < narchdir && (fn = writefn[c->qid.path]))
		return fn(c, a, n, offset);
	error(Eperm);

	return 0;
}

void archinit(void);

Dev archdevtab = {
	'P',
	"arch",

	devreset,
	archinit,
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

/* convert AddrDevid register to a string in buf and return buf */
char *
cputype2name(char *buf, int size)
{
	ulong id, archid, rev;
	char *manu, *arch, *socrev;
	char unk[32], socnm[32], revname[32];
	Pciex *pci;

	m->cputype = *(ulong *)soc.devid;
#ifdef OLD
	switch(m->cputype & 3) {
	case 0:
		socnm = "88F6[12]80";
		break;
	case 1:
		socnm = "88F619[02]";
		break;
	case 2:
		socnm = "88F6281";
		break;
	default:
		socnm = "unknown";
		break;
	}
#endif
	/* strange way to get this information, but it's what u-boot does */
	pci = (Pciex *)soc.pci;
	snprint(socnm, sizeof socnm, "88F%ux", pci->devid);
	/* stash rev for benefit of later usb initialisation */
	m->socrev = rev = pci->revid & MASK(4);

	id = cpidget();
	if ((id >> 24) == 0x56 && pci->venid == 0x11ab)
		manu = "Marvell";
	else
		manu = "unknown";
	archid = (id >> 16) & MASK(4);
	switch (archid) {
	case 5:
		arch = "v5te";
		break;
	default:
		snprint(unk, sizeof unk, "unknown (%ld)", archid);
		arch = unk;
		break;
	}
	if (pci->devid != 0x6281)
		socrev = "unknown";
	else
		switch (rev) {
		case Socrevz0:
			socrev = "Z0";
			break;
		case Socreva0:
			socrev = "A0";
			break;
		case Socreva1:
			socrev = "A1";
			break;
		default:
			snprint(revname, sizeof revname, "unknown rev (%ld)",
				rev);
			socrev = revname;
			break;
		}
	seprint(buf, buf + size,
		"%s %s %s; arm926ej-s arch %s rev %ld.%ld part %lux",
		manu, socnm, socrev, arch, (id >> 20) & MASK(4),
		id & MASK(4), (id >> 4) & MASK(12));
	return buf;
}

static long
cputyperead(Chan*, void *a, long n, vlong offset)
{
	char name[64], str[128];

	cputype2name(name, sizeof name);
	snprint(str, sizeof str, "ARM %s %llud\n", name, m->cpuhz / 1000000);
	return readstr(offset, a, n, str);
}

static long
tbread(Chan*, void *a, long n, vlong offset)
{
	char str[16];
	uvlong tb;

	cycles(&tb);

	snprint(str, sizeof(str), "%16.16llux", tb);
	return readstr(offset, a, n, str);
}

static long
nsread(Chan*, void *a, long n, vlong offset)
{
	char str[16];
	uvlong tb;

	cycles(&tb);

	snprint(str, sizeof(str), "%16.16llux", (tb/700)* 1000);
	return readstr(offset, a, n, str);
}

void
archinit(void)
{
	addarchfile("cputype", 0444, cputyperead, nil);
	addarchfile("timebase",0444, tbread, nil);
//	addarchfile("nsec", 0444, nsread, nil);
}
