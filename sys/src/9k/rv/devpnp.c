/*
 * access to PCI configuration space only; only PCs have Shrug'n'Pray.
 * magic numbers 'R' us
 *
 * not yet done:
 *	- extend PCI raw access to configuration space (writes, byte/short access?)
 *	- implement PCI access to memory/io space/BIOS ROM
 *	- use c->aux instead of performing lookup on each read/write?
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	<ctype.h>
#include	"io.h"
#include	"../port/error.h"

int pnpdebug;

#define	DPRINT	if(pnpdebug) print
#define	XPRINT	if(1) print

enum {
	Qtopdir = 0,

	Qpcidir,
	Qpcictl,
	Qpciraw,
};

#define TYPE(q)		((ulong)(q).path & MASK(4))
// #define CSN(q)	(((ulong)(q).path >> 4) & MASK(8))
#define QID(c, t)	((c) << 4 | (t))

static Dirtab topdir[] = {
	".",	{ Qtopdir, 0, QTDIR },	0,	0555,
	"pci",	{ Qpcidir, 0, QTDIR },	0,	0555,
};
Dev pnpdevtab;

static void
pnpreset(void)
{
}

static int
pcigen(Chan *c, int t, int tbdf, Dir *dp)
{
	Qid q;

	q = (Qid){BUSBDF(tbdf)|t, 0, 0};
	switch(t) {
	case Qpcictl:
		snprint(up->genbuf, sizeof up->genbuf, "%d.%d.%dctl",
			BUSBNO(tbdf), BUSDNO(tbdf), BUSFNO(tbdf));
		devdir(c, q, up->genbuf, 0, eve, 0444, dp);
		return 1;
	case Qpciraw:
		snprint(up->genbuf, sizeof up->genbuf, "%d.%d.%draw",
			BUSBNO(tbdf), BUSDNO(tbdf), BUSFNO(tbdf));
		devdir(c, q, up->genbuf, 128, eve, 0660, dp);
		return 1;
	}
	return -1;
}

static int
pnpgen(Chan *c, char *, Dirtab*, int, int s, Dir *dp)
{
	Qid q;
	Pcidev *p;
	int tbdf;

	switch(TYPE(c->qid)){
	case Qtopdir:
		if(s == DEVDOTDOT){
			q = (Qid){QID(0, Qtopdir), 0, QTDIR};
			snprint(up->genbuf, sizeof up->genbuf, "#%C", pnpdevtab.dc);
			devdir(c, q, up->genbuf, 0, eve, 0555, dp);
			return 1;
		}
		return devgen(c, nil, topdir, nelem(topdir), s, dp);
	case Qpcidir:
		if(s == DEVDOTDOT){
			q = (Qid){QID(0, Qtopdir), 0, QTDIR};
			snprint(up->genbuf, sizeof up->genbuf, "#%C", pnpdevtab.dc);
			devdir(c, q, up->genbuf, 0, eve, 0555, dp);
			return 1;
		}
		p = pcimatch(nil, 0, 0);
		while(s >= 2 && p != nil) {
			p = pcimatch(p, 0, 0);
			s -= 2;
		}
		if(p == nil)
			return -1;
		return pcigen(c, s+Qpcictl, p->tbdf, dp);
	case Qpcictl:
	case Qpciraw:
		tbdf = MKBUS(BusPCI, 0, 0, 0)|BUSBDF((ulong)c->qid.path);
		p = pcimatchtbdf(tbdf);
		if(p == nil)
			return -1;
		return pcigen(c, TYPE(c->qid), tbdf, dp);
	}
	return -1;
}

static Chan*
pnpattach(char *spec)
{
	return devattach(pnpdevtab.dc, spec);
}

Walkqid*
pnpwalk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, (Dirtab *)0, 0, pnpgen);
}

static long
pnpstat(Chan* c, uchar* dp, int long n)
{
	return devstat(c, dp, n, (Dirtab *)0, 0L, pnpgen);
}

static Chan*
pnpopen(Chan *c, int omode)
{
	return devopen(c, omode, (Dirtab*)0, 0, pnpgen);
}

static void
pnpclose(Chan*)
{
}

static long
pnpread(Chan *c, void *va, long n, vlong offset)
{
	ulong x;
	Pcidev *p;
	char buf[256], *ebuf, *w;
	char *a = va;
	int i, tbdf, r;

	switch(TYPE(c->qid)){
	case Qtopdir:
	case Qpcidir:
		return devdirread(c, a, n, (Dirtab *)0, 0L, pnpgen);
	case Qpcictl:
		tbdf = MKBUS(BusPCI, 0, 0, 0)|BUSBDF((ulong)c->qid.path);
		p = pcimatchtbdf(tbdf);
		if(p == nil)
			error(Egreg);
		ebuf = buf+sizeof buf-1;	/* -1 for newline */
		w = seprint(buf, ebuf, "%.2x.%.2x.%.2x %.4x/%.4x %3d",
			p->ccrb, p->ccru, p->ccrp, p->vid, p->did, p->intl);
		for(i=0; i<nelem(p->mem); i++){
			if(p->mem[i].size == 0)
				continue;
			w = seprint(w, ebuf, " %d:%.8lux %d", i, p->mem[i].bar,
				p->mem[i].size);
		}
		*w++ = '\n';
		*w = '\0';
		return readstr(offset, a, n, buf);
	case Qpciraw:
		tbdf = MKBUS(BusPCI, 0, 0, 0)|BUSBDF((ulong)c->qid.path);
		p = pcimatchtbdf(tbdf);
		if(p == nil)
			error(Egreg);
		if(offset > sizeof buf)
			return 0;
		if(n+offset > sizeof buf)
			n = sizeof buf-offset;
		r = offset;
		if(!(r & 3) && n == 4){
			x = pcicfgr32(p, r);
			PBIT32(a, x);
			return 4;
		}
		if(!(r & 1) && n == 2){
			x = pcicfgr16(p, r);
			PBIT16(a, x);
			return 2;
		}
		for(i = 0; i <  n; i++){
			x = pcicfgr8(p, r);
			PBIT8(a, x);
			a++;
			r++;
		}
		return i;
	default:
		error(Egreg);
		notreached();
	}
}

static long
pnpwrite(Chan *c, void *va, long n, vlong offset)
{
	Pcidev *p;
	ulong x;
	char buf[256];
	uchar *a;
	int i, r, tbdf;

	if(n >= sizeof(buf))
		n = sizeof(buf)-1;
	a = va;
	strncpy(buf, va, n);
	buf[n] = 0;

	switch(TYPE(c->qid)){
	case Qpciraw:
		tbdf = MKBUS(BusPCI, 0, 0, 0)|BUSBDF((ulong)c->qid.path);
		p = pcimatchtbdf(tbdf);
		if(p == nil)
			error(Egreg);
		if(offset > sizeof buf)
			return 0;
		if(n+offset > sizeof buf)
			n = sizeof buf-offset;
		r = offset;
		if(!(r & 3) && n == 4){
			x = GBIT32(a);
			pcicfgw32(p, r, x);
			return 4;
		}
		if(!(r & 1) && n == 2){
			x = GBIT16(a);
			pcicfgw16(p, r, x);
			return 2;
		}
		for(i = 0; i <  n; i++){
			x = GBIT8(a);
			pcicfgw8(p, r, x);
			a++;
			r++;
		}
		return i;
	default:
		error(Egreg);
		notreached();
	}
}


Dev pnpdevtab = {
	'$',
	"pnp",

	pnpreset,
	devinit,
	devshutdown,
	pnpattach,
	pnpwalk,
	pnpstat,
	pnpopen,
	devcreate,
	pnpclose,
	pnpread,
	devbread,
	pnpwrite,
	devbwrite,
	devremove,
	devwstat,
};
