/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include	"virtio_def.h"

// Include the definitions from VIRTIO spec v1.0
// http://docs.oasis-open.org/virtio/virtio/v1.0/csprd02/listings/virtio_ring.h

#include	"virtio_ring.h"

#include	"virtio_config.h"
#include	"virtio_pci.h"

#define TYPE(q)		((uint32_t)(q).path & 0x0F)
#define QID(c, t)	(((c)<<4)|(t))

#define BY2PG PGSZ

enum {
	Qtopdir = 0,			// top directory
	Qvirtqs,				// virtqs directory under the top
	Qdevtop,				// toplevel for each device, based on the PCI device number
	Qdevcfg,				// device config area read as a file
};


static Dirtab topdir[] = {
	".",		{ Qtopdir, 0, QTDIR },	0,	DMDIR|0555,
	"virtqs",	{ Qvirtqs, 0, QTDIR },	0,	DMDIR|0555,
};

extern Dev v9pdevtab;

// Map device identifiers to descriptive strings to display as virtio device names.

typedef struct
{
	uint16_t did;
	char *desc;
} didmap;

static didmap dmtab[] = {
	PCI_DEVICE_ID_VIRTIO_NET, "net",
	PCI_DEVICE_ID_VIRTIO_BLOCK, "block",
	PCI_DEVICE_ID_VIRTIO_BALLOON, "balloon",
	PCI_DEVICE_ID_VIRTIO_CONSOLE, "console",
	PCI_DEVICE_ID_VIRTIO_SCSI, "scsi",
	PCI_DEVICE_ID_VIRTIO_RNG, "rng",
	PCI_DEVICE_ID_VIRTIO_9P, "9p",
};

// Specific device control structures. They are preallocated during the driver
// initialization and remain mainly constant during the kernel uptime.

static int nv9p;			// number of the detected virtio9p devices

typedef struct V9pctl		// per-device control structure
{
	Pcidev *pci;			// PCI device descriptor
	uint32_t port;			// base I/O port for the legacy port-based interface
	uint32_t feat;			// host features
	uint32_t nqs;			// virt queues count
	Virtq **vqs;			// virt queues descriptors
	uint32_t dcfglen;		// device config area length
	uint32_t dcfgoff;		// device config area offset (20 or 24)
} V9pctl;

static V9pctl **cv9p;		// array of device control structure pointers, length = nv9p

static int
v9pgen(Chan *c, char *d, Dirtab* dir, int i, int s, Dir *dp)
{
	Proc *up = externup();
	Qid q;
	int t = TYPE(c->qid);
	
//print("\nv9pgen: type %d, s %d\n", t, s);
	switch(t){
	case Qtopdir:
		if(s == DEVDOTDOT){
			q = (Qid){QID(0, Qtopdir), 0, QTDIR};
			snprint(up->genbuf, sizeof up->genbuf, "#%C", v9pdevtab.dc);
			devdir(c, q, up->genbuf, 0, eve, DMDIR|0555, dp);
			return 1;
		}
		return devgen(c, nil, topdir, nelem(topdir), s, dp);
	case Qvirtqs:
		if(s == DEVDOTDOT){
			q = (Qid){QID(0, Qtopdir), 0, QTDIR};
			snprint(up->genbuf, sizeof up->genbuf, "#%C", v9pdevtab.dc);
			devdir(c, q, up->genbuf, 0, eve, DMDIR|0555, dp);
			return 1;
		}
		if(s >= nv9p)
			return -1;
		char *dmap = nil;
		for(int i = 0; i < nelem(dmtab) ; i++) {
			if(cv9p[s]->pci->did == dmtab[i].did) {
				dmap = dmtab[i].desc;
				break;
			}
		}
		snprint(up->genbuf, sizeof up->genbuf, "%s-%d", dmap?dmap:"virtio", s);
		q = (Qid) {QID(s, Qdevtop), 0, QTDIR};
		devdir(c, q, up->genbuf, 0, eve, DMDIR|0555, dp);
		return 1;
	default:
		return -1;
	}
	return -1;
}


static Chan*
v9pattach(char *spec)
{
print("v9pattach %s\n", spec);
	return devattach(v9pdevtab.dc, spec);
}


Walkqid*
v9pwalk(Chan* c, Chan *nc, char** name, int nname)
{
//print("v9pwalk %d -> %d\n", c?TYPE(c->qid):-1, nc?TYPE(nc->qid):-1);
	return devwalk(c, nc, name, nname, (Dirtab *)0, 0, v9pgen);
}

static int32_t
v9pstat(Chan* c, uint8_t* dp, int32_t n)
{
//print("v9pstat %d\n", TYPE(c->qid));
	return devstat(c, dp, n, (Dirtab *)0, 0L, v9pgen);
}


static Chan*
v9popen(Chan *c, int omode)
{
//print("v9open %d\n", TYPE(c->qid));
	c = devopen(c, omode, (Dirtab*)0, 0, v9pgen);
	switch(TYPE(c->qid)){
	default:
		break;
	}
	return c;
}

static void
v9pclose(Chan* c)
{
//print("v9pclose %d\n", TYPE(c->qid));
}

static int32_t
v9pread(Chan *c, void *va, int32_t n, int64_t offset)
{
//print("v9pread %d %d %d\n", TYPE(c->qid), n, offset);
	switch(TYPE(c->qid)) {
	case Qtopdir:
	case Qvirtqs:
		return devdirread(c, va, n, (Dirtab *)0, 0L, v9pgen);
	default:
		return -1;
	}
}

static int32_t
v9pwrite(Chan *c, void *va, int32_t n, int64_t offset)
{
print("v9pwrite %d %d %d\n", TYPE(c->qid), n, offset);
	return -1;
}

// Scan virtqueues for the given device. If the vqs argument is not nil then
// nvq is expected to contain the length of the array vqs points to. In this case
// populate the Virtq structures for each virtqueue found. Otherwise just return
// the number of virtqueues detected. The port argument contains the base port
// for the device being scanned.
// Portions of code are borrowed from the 9front virtio drivers.
// TODO: this function should be shared among all virtio drivers. Move it outside of this driver.

static int
findvqs(uint32_t port, int nvq, Virtq **vqs)
{
	int cnt = 0;
	while(1) {
		outs(port + VIRTIO_PCI_QUEUE_SEL, cnt);
		int qs = ins(port + VIRTIO_PCI_QUEUE_NUM);
		print("\nv9p queue %d size %d\n", cnt, qs);
		if(cnt >= 64 || qs == 0 || (qs & (qs-1)) != 0)
			break;
		if(vqs != nil) {
			// Allocate vq's descriptor space, used and available spaces, all page-aligned.
			vqs[cnt] = malloc(sizeof(Virtq));
			Virtq *q = vqs[cnt];
			q->num = qs;
			q->desc = mallocalign(PGROUND((q->num * sizeof(struct virtq_desc))), BY2PG, 0, 0);
			q->free = -1;
			q->nfree = qs;
			// So in 9front: each descriptor's free field points to the next descriptor
			for(int i = 0; i < qs; i++) {
				q->desc[i].next = q->free;
				q->free = i;
			}
			q->avail = mallocalign(PGROUND((sizeof(struct virtq_avail) + qs * sizeof(le16))), BY2PG, 0, 0);
			q->used = mallocalign(PGROUND((sizeof(struct virtq_used) + qs * sizeof(le16))), BY2PG, 0, 0);
		}
		cnt++;
	}
	return cnt;
}

// Scan the PCI devices list for possible virtio9p devices. If the vcs argument
// is not nil then populate the array of control structures, otherwise just return
// the number of devices found. This function is intended to be called twice,
// once with vcs = nil just to count the devices, and the second time to populate
// the control structures, expecting vcs to point to an array of pointers to device
// descriptors of sufficient length. Portions of code are borrowed from the 9front virtio
// drivers.

static int
find9p(V9pctl **vcs)
{
	int cnt = 0;
	// TODO: this seems to work as if MSI-X is not enabled (device conf space starts at 20).
	// Find out how to deduce msix_enabled from the device.
	int msix_enabled = 0;
	Pcidev *p;
	// Scan the collected PCI devices info, find possible 9p devices
	for(p = nil; p = pcimatch(p, PCI_VENDOR_ID_REDHAT_QUMRANET, 0);) {
		if(vcs != nil) {
			vcs[cnt] = malloc(sizeof(V9pctl));
			if(vcs[cnt] == nil) {
				print("\nv9p: failed to allocate device control structure for device %d\n", cnt);
				return cnt;
			}
			// Use the legacy interface
			// Allocate the BAR0 I/O space to the driver
			V9pctl *vc = vcs[cnt];
			vc->pci = p;
			vc->port = p->mem[0].bar & ~0x1;
			print("\nv9p: device[%d] port %lux\n", cnt, vc->port);
			if(ioalloc(vc->port, p->mem[0].size, 0, "virtio9p") < 0) {
				print("\nv9p: port %lux in use\n", vc->port);
				free(vc);
				vcs[cnt] = nil;
				return cnt;
			}
			// Device reset
			outb(vc->port + VIRTIO_PCI_STATUS, 0);
			vc->feat = inl(vc->port + VIRTIO_PCI_HOST_FEATURES);
			print("\nv9p: features %08x\n");
			outb(vc->port + VIRTIO_PCI_STATUS, VIRTIO_CONFIG_S_ACKNOWLEDGE|VIRTIO_CONFIG_S_DRIVER);
			int nqs = findvqs(vc->port, 0, nil);
			print("\nv9p: found %d queues\n", nqs);
			// For each vq allocate and populate its descriptor
			if(nqs > 0) {
				vc->vqs = malloc(nqs * sizeof(Virtq *));
				vc->nqs = nqs;
				findvqs(vc->port, nqs, vc->vqs);
				for(int i = 0; i < nqs; i++) {
					Virtq *q = vc->vqs[i];
					q->idx = i;
					q->pdev = vc;
					coherence();
					outl(vc->port + VIRTIO_PCI_QUEUE_PFN, PADDR(q->desc)/BY2PG);
				}
			}
			// Device config space contains data in consecutive 8bit input ports
			vc->dcfgoff = VIRTIO_PCI_CONFIG_OFF(msix_enabled);
			vc->dcfglen = vc->pci->mem[0].size - vc->dcfgoff;
		}
		cnt++;
	}
	return cnt;
}

// Driver initialization: find all virtio9p devices (by vendor & device ID).
// Sense the device configuration, figure out mount tags. We assume that the
// virtio9p devices do not appear or disappear between reboots.

void
v9pinit(void)
{
	print_func_entry();
	print("\nv9p initializing\n");
	nv9p = find9p(nil);
	print("\nv9p: found %d devices\n", nv9p);
	if(nv9p == 0) {
		return;
	}
	cv9p = malloc(nv9p * sizeof(V9pctl *));
	if(cv9p == nil) {
		print("\nv9p: failed to allocate control structures\n");
		return;
	}
	find9p(cv9p);
}

Dev v9pdevtab = {
	.dc = 'J',
	.name = "v9p",

	.reset = devreset,
	.init = v9pinit,
	.shutdown = devshutdown,
	.attach = v9pattach,
	.walk = v9pwalk,
	.stat = v9pstat,
	.open = v9popen,
	.create = devcreate,
	.close = v9pclose,
	.read = v9pread,
	.bread = devbread,
	.write = v9pwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};
