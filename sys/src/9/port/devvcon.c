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
	Qvcctl,					// control file (one for all consoles)
};

static Dirtab topdir[] = {
	".",		{ Qtopdir, 0, QTDIR },	0,	DMDIR|0555,
	"virtcon",	{ Qvirtcon, 0, QTDIR },	0,	DMDIR|0555,
};

extern Dev vcondevtab;

// Private per-console data (shared between both queues)

enum
{
	Vcmraw = 0,				// raw buffer transfer mode
	Vcmdelim = 1,			// prefilled/delimited buffer receive mode
	Vcm9p = 2,				// 9p-aware buffer receive mode
};

typedef struct vcondata 
{
	uint8_t prefill;		// prefill the receiving buffer with this octet
	uint8_t delim;			// determine input length as index of this octet (or full buffer length)
	uint16_t vcmode;		// virtual console mode (enum above)
	int32_t maxbuf;			// maximum buffer length
} VconData;

// Array of defined virtconsoles and their number

static uint32_t nvcon;

static Vqctl **vcons;

// Handle control file writes. The ctl file accepts command lines in the format
// name<space>param<space>value. Name is the console name (vconsN), param is one of mode|prefill|delim|maxbuf,
// value for mode is one of raw|9p|delim, values for maxbuf, prefill and delim are strings representing
// numeric values. A buffer should contain whole command line. Tokens should be delimited with exactly one space.
// If a command line cannot be parsed, error will be raised.

#define NEXTSPACE for(idxp = idx; idx < n; idx++) if(buf[idx] <= ' ') break
#define IFENDBUF(msg) if(idx == n) {error(msg); return -1;}
#define TOKEN(tk) tkl = idx - idxp; uint8_t tk[tkl + 1]; memmove(tk, buf + idxp, tkl); tk[tkl] = 0; idx++
#define IFPARAM(p) if(!strcmp((char *)param, p))
#define IFVALUE(v) if(!strcmp((char *)value, v))
#define IFINVAL if(!valval) { error("numeric conversion error"); return -1;}

static int
ctl(void *va, int32_t n) 
{
	Vqctl *vctl = nil;
	uint8_t *buf = va;
	int idx = 0, idxp, tkl;
	NEXTSPACE;
	IFENDBUF("devvcon: cannot extract console name");
	TOKEN(name);
	NEXTSPACE;
	IFENDBUF("devvcon: value are not provided");
	TOKEN(param);
	NEXTSPACE;
	TOKEN(value);
	for(int i = 0; i < nvcon; i++) {
		if(!strcmp((char *)name, (char *)vcons[i]->devname)) {
			vctl = vcons[i];
			break;
		}
	}
	if(vctl == nil) {
		error(Enonexist);
		return -1;
	}
	VconData *vc = vctl->vqs[0]->pqdata;
	if(vc == nil) {
		error("devvcon: private data was not allocated");
		return -1;
	}
	char *cptr;
	long nvalue = strtol((char *)value, &cptr, 0);
	int valval = (*cptr == 0);
	IFPARAM("mode") {
		IFVALUE("9p") {
			vc->vcmode = Vcm9p;
		} else IFVALUE("delim") {
			vc->vcmode = Vcmdelim;
		} else IFVALUE("raw") {
			vc->vcmode = Vcmraw;
		} else {
			error("devvcon: unrecognized mode");
			return -1;
		}
	} else IFPARAM("delim") {
		IFINVAL;
		vc->delim = nvalue & 0xFF;
	} else IFPARAM("prefill") {
		IFINVAL;
		vc->prefill = nvalue & 0xFF;
	} else IFPARAM("maxbuf") {
		IFINVAL;
		vc->maxbuf = nvalue;
	} else {
		error("devvcon: unrecognized parameter");
		return -1;
	}

	return n;
}

// Read-write common code

static int32_t
getmaxbuf(VconData *vc)
{
	if(vc == 0)
		return 32768;
	else
		return vc->maxbuf;
}

static int
rwcommon(Vqctl *d, void *va, int32_t n, int qidx)
{
	uint16_t descr[1];
	Virtq *vq = d->vqs[qidx];
	VconData *vc = vq->pqdata;
	if(n > getmaxbuf(vc)) {
		error(Etoobig);
		return -1;
	}
	int nd = getdescr(vq, 1, descr);
	if(nd < 1)
	{
		error("virtcon: queue low");
		return -1;
	}
	uint8_t *buf = malloc(n);
	if(buf == nil) {
		error("devvcon: no memory to allocate the exchange buffer");
		return -1;
	}
	if(qidx) {
		memmove(buf, va, n);
	} else {
		if(vc && vc->vcmode == Vcmdelim)
			memset(buf, vc->prefill, n);
	}
	q2descr(vq, descr[0])->addr = PADDR(buf);
	q2descr(vq, descr[0])->len = n;	
	if(!qidx) {
		q2descr(vq, descr[0])->flags = VRING_DESC_F_WRITE;
	}
	int rc = queuedescr(vq, 1, descr);
	int32_t nlen = n;
	if(!qidx) {
		if(vc != nil)
		{
			switch(vc->vcmode) {
			case Vcmdelim:
				for(nlen = 0; nlen < n; nlen++) {
					if(buf[nlen] == vc->delim)
						break;
				}
				break;
			case Vcm9p:
				nlen = GBIT32(buf);
				if(nlen > n)
					nlen = n;
				break;
			default:
				nlen = n;
			}
		} else {
			nlen = n;
		}
		memmove(va, buf, nlen);
	}
	reldescr(vq, 1, descr);
	free(buf);
	return (rc >= 0)?nlen:rc;
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
		if(s > nvcon)
			return -1;
		if(s == nvcon) {
			snprint(up->genbuf, sizeof up->genbuf, "ctl", s);
			q = (Qid) {QID(0, Qvcctl), 0, 0};
			devdir(c, q, up->genbuf, 0, eve, 0222, dp);
			return 1;
		}
		snprint(up->genbuf, sizeof up->genbuf, vcons[s]->devname);
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
	case Qvcctl:
		error(Eperm);
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
	case Qvcctl:
		return ctl(va, n);
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
	uint32_t nvdev;

	print("virtio-serial-pci initializing\n");
	nvdev = getvdevnum();
	if(nvdev <= 0)
		return;
	vcons = mallocz(nvdev * sizeof(Vqctl *), 1);
	if(vcons == nil) {
		print("no memory to allocate virtual consoles\n");
		return;
	}
	nvcon = 0;
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
		VconData *vc = mallocz(sizeof(VconData), 1);
		if(vc == nil) {
			print("failed to allocate per-queue data for console %d, assuming raw mode only\n", i);
			continue;
		}
		vc->maxbuf = 32768;
		for(int j = 0; j < vcons[i]->nqs ; j++) {
			vcons[i]->vqs[j]->pqdata = vc;
		}
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
