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

static long
cputyperead(Chan*, void *a, long n, vlong offset)
{
	char str[128];

	snprint(str, sizeof str, "MIPS 24k %lud\n", m->hz / Mhz);
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

char *cputype = "mips";

char	*faultsprint(char *, char *);
char	*fpemuprint(char *, char *);

static long
archctlread(Chan*, void *a, long nn, vlong offset)
{
	int n;
	char *buf, *p, *ep;

	p = buf = malloc(READSTR);
	if(p == nil)
		error(Enomem);
	ep = p + READSTR;
	p = seprint(p, ep, "cpu %s %lud\n", cputype,
		(ulong)(m->hz+999999)/1000000);
	p = seprint(p, ep, "stlb hash collisions");
	for (n = 0; n < conf.nmach; n++)
		p = seprint(p, ep, " %d", MACHP(n)->hashcoll);
	p = seprint(p, ep, "\n");
	p = seprint(p, ep, "NKTLB %d ktlb misses %ld utlb misses %ld\n",
		NKTLB, m->ktlbfault, m->utlbfault);
	p = fpemuprint(p, ep);
	faultsprint(p, ep);
	n = readstr(offset, a, nn, buf);
	free(buf);
	return n;
}

enum
{
	CMfpemudebug,
};

static Cmdtab archctlmsg[] =
{
#ifdef FPEMUDEBUG
	CMfpemudebug,	"fpemudebug",	2,
#else
	CMfpemudebug,	"dummy",	1,
#endif
};

static long
archctlwrite(Chan*, void *a, long n, vlong)
{
	Cmdbuf *cb;
	Cmdtab *ct;

	cb = parsecmd(a, n);
	if(waserror()){
		free(cb);
		nexterror();
	}
	ct = lookupcmd(cb, archctlmsg, nelem(archctlmsg));
	switch(ct->index){
	case CMfpemudebug:
		fpemudebug = atoi(cb->f[1]);
		break;
	}
	free(cb);
	poperror();
	return n;
}

void
archinit(void)
{
	addarchfile("cputype", 0444, cputyperead, nil);
	addarchfile("timebase",0444, tbread, nil);
	addarchfile("archctl", 0664, archctlread, archctlwrite);
//	addarchfile("nsec", 0444, nsread, nil);
}
