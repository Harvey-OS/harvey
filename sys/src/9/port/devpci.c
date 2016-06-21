/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

// Include the definitions from VIRTIO spec v1.0
// http://docs.oasis-open.org/virtio/virtio/v1.0/csprd02/listings/virtio_ring.h

#include	"virtio_ring.h"

enum {
	Qtopdir = 0,

	Qpcidir,
	Qpcictl,
	Qpciraw,
	Qpcicap,
};

#define TYPE(q)		((uint32_t)(q).path & 0x0F)
#define QID(c, t)	(((c)<<4)|(t))

static Dirtab topdir[] = {
	".",	{ Qtopdir, 0, QTDIR },	0,	0555,
	"pci",	{ Qpcidir, 0, QTDIR },	0,	0555,
};

extern Dev pcidevtab;

static int
pcidirgen(Chan *c, int t, int tbdf, Dir *dp)
{
	Pcidev *p;
	Proc *up = externup();
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
		devdir(c, q, up->genbuf, 128, eve, 0664, dp);
		return 1;

	// Display device capabilities as a directory, 
	// each capability has an entry as a file.

	case Qpcicap:
		p = pcimatchtbdf(tbdf);
		if((p == nil) || (p->capcnt == 0))
			return 0;
		snprint(up->genbuf, sizeof up->genbuf, "%d.%d.%dcap",
			BUSBNO(tbdf), BUSDNO(tbdf), BUSFNO(tbdf));
		q.type = QTDIR;
		devdir(c, q, up->genbuf, 0, eve, DMDIR|0555, dp);
		return 1;
	}
	return -1;
}

static int
pcigen(Chan *c, char *d, Dirtab* dir, int i, int s, Dir *dp)
{
	Proc *up = externup();
	int tbdf;
	Pcidev *p;
	Qid q;

	switch(TYPE(c->qid)){
	case Qtopdir:
		if(s == DEVDOTDOT){
			q = (Qid){QID(0, Qtopdir), 0, QTDIR};
			snprint(up->genbuf, sizeof up->genbuf, "#%C", pcidevtab.dc);
			devdir(c, q, up->genbuf, 0, eve, 0555, dp);
			return 1;
		}
		return devgen(c, nil, topdir, nelem(topdir), s, dp);
	case Qpcidir:
		if(s == DEVDOTDOT){
			q = (Qid){QID(0, Qtopdir), 0, QTDIR};
			snprint(up->genbuf, sizeof up->genbuf, "#%C", pcidevtab.dc);
			devdir(c, q, up->genbuf, 0, eve, 0555, dp);
			return 1;
		}
		p = pcimatch(nil, 0, 0);
		while(s >= 3 && p != nil) {
			p = pcimatch(p, 0, 0);
			s -= 3;
		}
		if(p == nil)
			return -1;
		return pcidirgen(c, s+Qpcictl, p->tbdf, dp);
	case Qpcicap:
	case Qpcictl:
	case Qpciraw:
		tbdf = MKBUS(BusPCI, 0, 0, 0)|BUSBDF((uint32_t)c->qid.path);
		p = pcimatchtbdf(tbdf);
		if(p == nil)
			return -1;
		
		// Generate a directory entry for each PCI device capability
		// gathered during the PCI bus scan.
		// Each capability is displayed as a file of length equal to the length
		// of the capability in the memory, and the file name is formatted as
		// capN.vV.lL.tT.bB.oO where N is capability index as collected during
		// the PCI bus scan, V is capability vendor code (cap_vndr), L is
		// capability length in the PCI config space (cap_len), T is capability
		// config type (cfg_type), B is BAR number, and O is offset within BAR.
		// The capabilities files cannot be read or written. They are displayed
		// for exploratory purposes only.
			
		if(TYPE(c->qid) == Qpcicap) {
			if(s >= p->capcnt)
				return -1;
			q = (Qid){BUSBDF(tbdf)|(Qpcicap + s + 1), 0, 0};
			Pcicap *pcp = p->capidx[s];
			snprint(up->genbuf, sizeof up->genbuf, "cap%d.v%d.l%d.t%d.b%d.o%d", 
				s, pcp->vndr, pcp->caplen, pcp->type, pcp->bar, pcp->offset);
			devdir(c, q, up->genbuf, pcp->length, eve, 0444, dp);
			return 1;
		} else {
			return pcidirgen(c, TYPE(c->qid), tbdf, dp);
		}
	default:
		break;
	}
	return -1;
}

static Chan*
pciattach(char *spec)
{
	return devattach(pcidevtab.dc, spec);
}

Walkqid*
pciwalk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, (Dirtab *)0, 0, pcigen);
}

static int32_t
pcistat(Chan* c, uint8_t* dp, int32_t n)
{
	return devstat(c, dp, n, (Dirtab *)0, 0L, pcigen);
}

static Chan*
pciopen(Chan *c, int omode)
{
	c = devopen(c, omode, (Dirtab*)0, 0, pcigen);
	switch(TYPE(c->qid)){
	default:
		break;
	}
	return c;
}

static void
pciclose(Chan* c)
{
}

static int32_t
pciread(Chan *c, void *va, int32_t n, int64_t offset)
{
	char buf[256], *ebuf, *w, *a;
	int i, tbdf, r;
	uint32_t x;
	Pcidev *p;

	a = va;
	switch(TYPE(c->qid)){
	case Qtopdir:
	case Qpcidir:
	case Qpcicap:
		return devdirread(c, a, n, (Dirtab *)0, 0L, pcigen);
	case Qpcictl:
		tbdf = MKBUS(BusPCI, 0, 0, 0)|BUSBDF((uint32_t)c->qid.path);
		p = pcimatchtbdf(tbdf);
		if(p == nil)
			error(Egreg);
		ebuf = buf+sizeof buf-1;	/* -1 for newline */
		w = seprint(buf, ebuf, "%.2x.%.2x.%.2x %.4x/%.4x %3d",
			p->ccrb, p->ccru, p->ccrp, p->vid, p->did, p->intl);
		for(i=0; i<nelem(p->mem); i++){
			if(p->mem[i].size == 0)
				continue;
			w = seprint(w, ebuf, " %d:%.8lux %d", i, p->mem[i].bar, p->mem[i].size);
		}
		*w++ = '\n';
		*w = '\0';
		return readstr(offset, a, n, buf);
	case Qpciraw:
		tbdf = MKBUS(BusPCI, 0, 0, 0)|BUSBDF((uint32_t)c->qid.path);
		p = pcimatchtbdf(tbdf);
		if(p == nil)
			error(Egreg);
		if(n+offset > 256)
			n = 256-offset;
		if(n < 0)
			return 0;
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
	}
	return n;
}

static int32_t
pciwrite(Chan *c, void *va, int32_t n, int64_t offset)
{
	char buf[256];
	uint8_t *a;
	int i, r, tbdf;
	uint32_t x;
	Pcidev *p;

	if(n >= sizeof(buf))
		n = sizeof(buf)-1;
	a = va;
	strncpy(buf, (char*)a, n);
	buf[n] = 0;

	switch(TYPE(c->qid)){
	case Qpciraw:
		tbdf = MKBUS(BusPCI, 0, 0, 0)|BUSBDF((uint32_t)c->qid.path);
		p = pcimatchtbdf(tbdf);
		if(p == nil)
			error(Egreg);
		if(offset > 256)
			return 0;
		if(n+offset > 256)
			n = 256-offset;
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
	}
	return n;
}

Dev pcidevtab = {
	.dc = '$',
	.name = "pci",

	.reset = devreset,
	.init = devinit,
	.shutdown = devshutdown,
	.attach = pciattach,
	.walk = pciwalk,
	.stat = pcistat,
	.open = pciopen,
	.create = devcreate,
	.close = pciclose,
	.read = pciread,
	.bread = devbread,
	.write = pciwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};
