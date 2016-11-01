/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

// devvcon.c ('#C'): a virtual console (virtio-serial-pci) driver.

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"virtio_ring.h"

#include	"virtio_config.h"
#include	"virtio_console.h"
#include	"virtio_pci.h"

#include	"virtio_lib.h"

enum
{
	Qtopdir = 0,			// top directory
	Qvirtcon,				// virtcon directory under the top where all consoles live
	Qvcpipe,				// console pipe for reading/writing
};

static Dirtab topdir[] = {
	".",		{ Qtopdir, 0, QTDIR },	0,	DMDIR|0555,
	"virtcon",	{ Qvirtcon, 0, QTDIR },	0,	DMDIR|0555,
};

extern Dev vcondevtab;

// Array of defined virtconsoles and their number

static uint32_t nvcon;

static Vqctl **vcons;

// Read-write common code

static int
rwcommon(Vqctl *d, void *va, int32_t n, int qidx)
{
	uint16_t descr[1];
	Virtq *vq = d->vqs[qidx];
	int nd = getdescr(vq, 1, descr);
	if(nd < 1)
	{
		error("virtcon: queue low");
		return -1;
	}
	uint8_t buf[n];
	if(qidx) {
		memmove(buf, va, n);
	}
	q2descr(vq, descr[0])->addr = PADDR(buf);
	q2descr(vq, descr[0])->len = n;	
	if(!qidx) {
		q2descr(vq, descr[0])->flags = VRING_DESC_F_WRITE;
	}
	int rc = queuedescr(vq, 1, descr);
	if(!qidx) {
		memmove(va, buf, n);
	}
	reldescr(vq, 1, descr);
	return (rc >= 0)?n:rc;
}

static int
vcongen(Chan *c, char *d, Dirtab* dir, int i, int s, Dir *dp)
{
	Proc *up = externup();
	Qid q;
	int t = TYPE(c->qid);
	int vdidx = DEV(c->qid);
	if(vdidx >= nvcon)
		error(Ebadarg);
	switch(t){
	case Qtopdir:
		if(s == DEVDOTDOT){
			q = (Qid){QID(0, Qtopdir), 0, QTDIR};
			snprint(up->genbuf, sizeof up->genbuf, "#%C", vcondevtab.dc);
			devdir(c, q, up->genbuf, 0, eve, DMDIR|0555, dp);
			return 1;
		}
		return devgen(c, nil, topdir, nelem(topdir), s, dp);
	case Qvirtcon:
		if(s == DEVDOTDOT){
			q = (Qid){QID(0, Qtopdir), 0, QTDIR};
			snprint(up->genbuf, sizeof up->genbuf, "#%C", vcondevtab.dc);
			devdir(c, q, up->genbuf, 0, eve, DMDIR|0555, dp);
			return 1;
		}
		if(s >= nvcon)
			return -1;
		snprint(up->genbuf, sizeof up->genbuf, "vcons%d", s);
		q = (Qid) {QID(s, Qvcpipe), 0, 0};
		devdir(c, q, up->genbuf, 0, eve, 0666, dp);
		return 1;
	}
	return -1;
}

static Chan*
vconattach(char *spec)
{
	return devattach(vcondevtab.dc, spec);
}


Walkqid*
vconwalk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, (Dirtab *)0, 0, vcongen);
}

static int32_t
vconstat(Chan* c, uint8_t* dp, int32_t n)
{
	return devstat(c, dp, n, (Dirtab *)0, 0L, vcongen);
}

static Chan*
vconopen(Chan *c, int omode)
{
	uint t = TYPE(c->qid);
	uint vdidx = DEV(c->qid);
	if(vdidx >= nvcon)
		error(Ebadarg);
	c = devopen(c, omode, (Dirtab*)0, 0, vcongen);
	switch(t) {
	default:
		break;
	}
	return c;
}

static int32_t
vconread(Chan *c, void *va, int32_t n, int64_t offset)
{
	int vdidx = DEV(c->qid);
	if(vdidx >= nvcon)
		error(Ebadarg);
	switch(TYPE(c->qid)) {
	case Qtopdir:
	case Qvirtcon:
		return devdirread(c, va, n, (Dirtab *)0, 0L, vcongen);
	case Qvcpipe:
		return rwcommon(vcons[vdidx], va, n, 0);
	}
	return -1;
}

static int32_t
vconwrite(Chan *c, void *va, int32_t n, int64_t offset)
{
	int vdidx = DEV(c->qid);
	if(vdidx >= nvcon)
		error(Ebadarg);
	switch(TYPE(c->qid)) {
	case Qtopdir:
	case Qvirtcon:
		error(Eperm);
		return -1;
	case Qvcpipe:
		return rwcommon(vcons[vdidx], va, n, 1);
	}
	return -1;
}

static void
vconclose(Chan* c)
{
}

static uint32_t 
wantfeat(uint32_t f) {
	return VIRTIO_CONSOLE_F_SIZE;	// We want only console size, but not multiport for simplicity
}
	
static void
vconinit(void)
{
	print("virtio-serial-pci initializing\n");
	uint32_t nvdev = getvdevnum();
	vcons = mallocz(nvdev * sizeof(Vqctl *), 1);
	if(vcons == nil) {
		print("no memory to allocate virtual consoles\n");
		return;
	}
	nvcon = getvdevsbypciid(PCI_DEVICE_ID_VIRTIO_CONSOLE, vcons, nvdev);
	print("virtio consoles found: %d\n", nvcon);
	for(int i = 0; i < nvcon; i++) {
		print("initializing virtual console %d\n", i);
		uint32_t feat = vdevfeat(vcons[i], wantfeat);
		print("features: 0x%08x\n", feat);
		struct virtio_console_config vcfg;
		int rc = readvdevcfg(vcons[i], &vcfg, sizeof(vcfg), 0);
		print("config area size %d\n", rc);
		print("cols=%d rows=%d ports=%d\n", vcfg.cols, vcfg.rows, vcfg.max_nr_ports);
		finalinitvdev(vcons[i]);
	}
}

Dev vcondevtab = {
	.dc = 'C',
	.name = "vcon",

	.reset = devreset,
	.init = vconinit,
	.shutdown = devshutdown,
	.attach = vconattach,
	.walk = vconwalk,
	.stat = vconstat,
	.open = vconopen,
	.create = devcreate,
	.close = vconclose,
	.read = vconread,
	.bread = devbread,
	.write = vconwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};
