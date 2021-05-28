#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

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

/* convert m->cputype and pvr register to a string in buf and return buf */
char *
cputype2name(char *buf, int size)
{
	char *s, *es;
	ulong cputype = getpvr();

	s = buf;
	es = buf + size;
	/* ignoring 460, 8xx & 8xxx, e200*, e500* */
	switch (m->cputype) {
	case 1:
		seprint(s, es, "601");
		break;
	case 3:	case 6: case 7:
		seprint(s, es, "603");
		break;
	case 4: case 9: case 0xa:
		seprint(s, es, "604");
		break;
	case 8: case 0x7000: case 0x7002:
		seprint(s, es, "G3 7xx");
		break;
	case 0x000c: case 0x800c: case 0x8000: case 0x8001:
	case 0x8002: case 0x8003: case 0x8004:
		seprint(s, es, "G4 74xx");
		break;
	case 0x39: case 0x3c:
		seprint(s, es, "G5 970");
		break;
	case 0x20:
		seprint(s, es, "403");
		break;
	case 0x1291: case 0x4011: case 0x41f1: case 0x5091: case 0x5121:
		seprint(s, es, "405");
		break;
	case 0x2001:			/* 200 is Xilinx, 1 is ppc405 */
		cputype &= ~0xfff;
		s = seprint(s, es, "Xilinx ");
		switch (cputype) {
		case 0x20010000:
			seprint(s, es, "Virtex-II Pro 405");
			break;
		case 0x20011000:
			seprint(s, es, "Virtex 4 FX 405D5X2");
			break;
		default:
			seprint(s, es, "405");
			break;
		}
		break;
	case 0x7ff2:
		s = seprint(s, es, "Xilinx ");
		if ((cputype & ~0xf) == 0x7ff21910)
			seprint(s, es, "Virtex 5 FXT 440X5");
		else
			seprint(s, es, "440");
		break;
	default:
		/* oddballs */
		if ((cputype & 0xf0000ff7) == 0x400008d4 ||
		    (cputype & 0xf0000ffb) == 0x200008d0 ||
		    (cputype & 0xf0000ffb) == 0x200008d8 ||
		    (cputype & 0xfff00fff) == 0x53200891 ||
		    (cputype & 0xfff00fff) == 0x53400890 ||
		    (cputype & 0xfff00fff) == 0x53400891) {
			seprint(s, es, "440");
			break;
		}
		cputype &= 0xf0000fff;
		switch (cputype) {
		case 0x40000440: case 0x40000481: case 0x40000850:
		case 0x40000858: case 0x400008d3: case 0x400008db:
		case 0x50000850: case 0x50000851: case 0x50000892:
		case 0x50000894:
			seprint(s, es, "440");
			break;
		default:
			seprint(s, es, "%#ux", m->cputype);
			break;
		}
		break;
	}
	return buf;
}

static long
cputyperead(Chan*, void *a, long n, vlong offset)
{
	char name[64], str[128];

	cputype2name(name, sizeof name);
	snprint(str, sizeof str, "PowerPC %s %lud\n", name, m->cpuhz / 1000000);
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

uvlong
fastns(void)
{
	return gettbl();
}

static long
mutread(Chan*, void *a, long n, vlong offset)
{
	char str[256];

	snprint(str, sizeof str, "period (sec.s) %lud\n"
		"last start sec. %lud\n"
		"ticks of last mutation %lud\n"
		"mutations %lud\n"
		"ticks of average mutation %lud\n"
		"border %#lux\n",
		mutstats.period, mutstats.lasttm, mutstats.lastticks,
		mutstats.count, mutstats.totticks / mutstats.count,
		qtmborder());
	return readstr(offset, a, n, str);
}

enum {
	CMnow,
	CMperiod,
	CMstop,
};

Cmdtab mutmsg[] = {
	CMnow,	"now",		1,
	CMperiod, "period",	2,
	CMstop,	"stop",		1,
};

static long
mutwrite(Chan*, void *a, long n, vlong /* offset */)
{
	Cmdbuf *cb;
	Cmdtab *ct;

	cb = parsecmd(a, n);
	if(waserror()) {
		free(cb);
		nexterror();
	}
	ct = lookupcmd(cb, mutmsg, nelem(mutmsg));
	switch(ct->index) {
	case CMnow:
		if (mutatetrigger() < 0)
			error("mutate in progress");
		break;
	case CMperiod:
		mutstats.period = atoi(cb->f[1]);
		break;
	case CMstop:
		mutstats.period = 0;
		break;
	default:
		error(Ebadctl);
		break;
	}
	poperror();
	free(cb);
	return n;
}

static long
intrsread(Chan*, void *a, long n, vlong offset)
{
	char *p;

	if(n == 0)
		return 0;
	p = malloc(READSTR);
	intrfmtcounts(p, p + READSTR);
	n = readstr(offset, a, n, p);
	free(p);
	return n;
}

void
archinit(void)
{
	addarchfile("cputype", 0444, cputyperead, nil);
	addarchfile("timebase",0444, tbread, nil);
	addarchfile("nsec", 0444, nsread, nil);
	addarchfile("mutate", 0644, mutread, mutwrite);
	addarchfile("intrs", 0444, intrsread, nil);
}
