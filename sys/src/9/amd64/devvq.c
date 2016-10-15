/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

// devvq.c ('#Q'): a generic virtqueue driver.

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

#include	"virtio_config.h"
#include	"virtio_pci.h"

#include	"virtio_custom.h"

#define TYPE(q)			((uint32_t)(q).path & 0x0F)
#define DEV(q)			((uint32_t)(((q).path >> 4) & 0x0FFF))
#define VQ(q)			((uint32_t)(((q).path >> 16) & 0x0FFFF))
#define QID(c, t)		((((c) & 0x0FFF)<<4) | ((t) & 0x0F))
#define VQQID(q, c, t)	((((q) & 0x0FFFF)<<16) | (((c) & 0x0FFF)<<4) | ((t) & 0x0F))

#define BY2PG PGSZ

#define MAXVQS 8 			// maximal number of VQs per device

enum
{
	Qtopdir = 0,			// top directory
	Qvirtqs,				// virtqs directory under the top
	Qdevtop,				// toplevel for each device, based on the PCI device number
	Qdevcfg,				// device config area read as a file
	Qvqdata,				// virtqueue file entry for reading-writing messages
	QVqctl,					// virtqueue file entry for control operations, write-only
};

enum
{
	Vqqsize = 64*1024		// default size for the device read queue size
};

static Dirtab topdir[] = {
	".",		{ Qtopdir, 0, QTDIR },	0,	DMDIR|0555,
	"virtqs",	{ Qvirtqs, 0, QTDIR },	0,	DMDIR|0555,
};

extern Dev vqdevtab;

// For devices not yet implemented, readwrite operations error out.

static Flavor flavor_dummy;

static int32_t
dummyrw(Virtq *v, Chan *c, void *va, int32_t n, int64_t offset)
{
	error(Eio);
	return -1;
}

static Flavor *
mkdummyrw(Vqctl *dev, int idx)
{
	return &flavor_dummy;
}

static Flavor 
flavor_dummy = {
	.vdata = nil,
	.mkflavor = mkdummyrw,
	.cleanup = nil,
	.read = dummyrw,
	.write = dummyrw
};

// Map device identifiers to descriptive strings to display as virtio device names.

typedef struct
{
	uint16_t did;
	char *desc;
	Flavor *flv;
} didmap;

static didmap dmtab[] = {
	PCI_DEVICE_ID_VIRTIO_NET, "virtio-net", &flavor_dummy,
	PCI_DEVICE_ID_VIRTIO_BLOCK, "virtio-block", &flavor_dummy,
	PCI_DEVICE_ID_VIRTIO_BALLOON, "virtio-balloon", &flavor_dummy,
	PCI_DEVICE_ID_VIRTIO_CONSOLE, "virtio-console", &flavor_dummy,
	PCI_DEVICE_ID_VIRTIO_SCSI, "virtio-scsi", &flavor_dummy,
	PCI_DEVICE_ID_VIRTIO_RNG, "virtio-rng", &flavor_dummy,
	PCI_DEVICE_ID_VIRTIO_9P, "virtio-9p", &flavor_9p
};

// Specific device control structures. They are preallocated during the driver
// initialization and remain mainly constant during the kernel uptime.

static int nvq;			// number of the detected virtio9p devices

static Vqctl **cvq;			// array of device control structure pointers, length = nvq

// Get the virtqueue reference from a QID. Returns a valid VQ reference, or nil
// if the QID does not correspond to an existing VQ.

static Virtq *
qid2vq(Qid q)
{
	int vdidx = DEV(q);
	int vqidx = VQ(q);
	if(vdidx >= nvq)
		return nil;
	Vqctl *vc = cvq[vdidx];
	if(vqidx >= vc->nqs)
		return nil;
	return vc->vqs[vqidx];
}

static int
viodone(void *arg)
{
	return ((Rock*)arg)->done;
}

static void
vqinterrupt(Virtq *q);

// The interrupt handler entry point. Handler will be dispatched based on
// the bit set in the interrupt status register. If bit 1 is set then a virtqueue
// has to be handled, otherwise the device connfig area was updated. Reflect this
// in the reported device config area modification time.
// If the device's vq IO map is entirely zero, service all existing queues in turn.
// Otherwise only those queues whose bit is set.

static void 
vqintr(Ureg *x, void *arg)
{
	Vqctl *dev = arg;
	uint8_t isr = inb(dev->port + VIRTIO_PCI_ISR);
	if(isr & 2) {
		dev->dcmtime = seconds();
		return;
	} else if(isr & 1) {
		for(int i = 0; i < dev->nqs; i++) {
			vqinterrupt(dev->vqs[i]);
		}
	}
	return;
}

// The interrupt handler part that handles the virtqueue.

static void
vqinterrupt(Virtq *q)
{
	int id, m;
	Rock *r;
	Rendez *z;
	m = q->vr.num - 1;
	ilock(&q->l);
	while((q->lastused ^ q->vr.used->idx) & m) {
		id = q->vr.used->ring[q->lastused++ & m].id;
		if(r = q->rock[id]){
			q->rock[id] = nil;
			z = r->sleep;
			r->done = 1;	/* hands off */
			if(z != nil)
				wakeup(z);
		}
	}
	iunlock(&q->l);
}

// Release a given number of descriptors back to the virtqueue.

void
reldescr(Virtq *q, int n, uint16_t *descr)
{
	ilock(&q->l);
	for(int i = 0; i < n; i++) {
		q2descr(q, descr[i])->next = q->free;
		q->free = descr[i];
		q->nfree++;
	}
	iunlock(&q->l);
}

// Obtain a given number of descriptiors from the virtqueue. If the number of free descriptors is low, wait
// until available. Return value: number of descriptors allocated (should be same as requested),
// or -1 (if number of descriptors requested is more than the queue length). The caller should preallocate
// an array of uint16_t values of the same or larger size as the number of descriptors requested;
// this array will be populated. If more than one descriptor is requested, the descriptors allocated
// will be chained in the order of the increasing indice of the array.

int
getdescr(Virtq *q, int n, uint16_t *descr)
{
	if(n > q->vr.num)
		return -1;
	Proc *up = externup();
	ilock(&q->l);
	while(q->nfree < n) {
		iunlock(&q->l);
		if(!waserror())
			tsleep(&up->sleep, return0, 0, 500);
		poperror();
		ilock(&q->l);
	}
	for(int i = 0; i < n; i++) {
		int di = q->free;
		descr[i] = di;
		struct vring_desc *d = &q->vr.desc[di];
		q->free = d->next;
		q->nfree--;
		d->flags = 0;
		d->next = 0;
		if(i > 0) {
			struct vring_desc *pd = &q->vr.desc[descr[i - 1]];
			pd->flags = VRING_DESC_F_NEXT;
			pd->next = di;
		}
	}
	iunlock(&q->l);
	return n;
}

// Place a given number of populated descriptors into the virtqueue. Descriptor indices are
// provided in an array. Descriptors will be queued in the order of increasing array index.

int
queuedescr(Virtq *q, int n, uint16_t *descr)
{
	Proc *up = externup();
	int head = descr[0];
	uint16_t mask = q->vr.num - 1;				// q->num is power of 2 so mask has all bits set
	Rock rock;								// the sleep-wakeup semaphore on the process stack
	rock.done = 0;
	rock.sleep = &up->sleep;
	ilock(&q->l);
	q->rock[head] = &rock;
	for(int i = 0; i < n; i++) {
		q->vr.avail->ring[q->vr.avail->idx & mask] = descr[i];
		q->vr.avail->idx++;
	}
	coherence();
	iunlock(&q->l);
	if((q->vr.used->flags & VRING_USED_F_NO_NOTIFY) == 0) {
		uint32_t nport = ((Vqctl *)(q->pdev))->port + VIRTIO_PCI_QUEUE_NOTIFY;
		outs(nport, q->idx);
	}
	while(!rock.done) {
		sleep(rock.sleep, viodone, &rock);
	}
	return 0;
}

// Find a device type by its PCI device identifier, used to assign device name in the filesystem,
// and to determine the flavor of read-write operations.

static didmap *
finddev(Vqctl *vc)
{
	for(int i = 0; i < nelem(dmtab) ; i++) {
		if(vc->pci->did == dmtab[i].did) {
			return (dmtab + i);
		}
	}
	return nil;
}

// Map PCI device identifier to a readable name for the filesystem entry.

static char *
mapdev(Vqctl *vc)
{
	char *dmap = nil;
	didmap *dm = finddev(vc);
	if(dm != nil)
		dmap = dm->desc;
	return dmap;
}

// Allocate a flavor structure for a device per its PCI device identifier.
// If a flavor cannot be determined, dummy flavor will be returned. If a flavor
// was allocated earlier it will not be allocated again.

static Flavor *
mkdevflavor(Vqctl *vc, int idx)
{
	if(vc->vqs[idx]->flavor != nil)
		return vc->vqs[idx]->flavor;
	didmap *dm = finddev(vc);
	if(dm == nil)
		return flavor_dummy.mkflavor(vc, idx);
	Flavor *res = (dm->flv->mkflavor(vc, idx));
	return (res?res:flavor_dummy.mkflavor(vc, idx));
}

static int
vqgen(Chan *c, char *d, Dirtab* dir, int i, int s, Dir *dp)
{
	Proc *up = externup();
	Qid q;
	int t = TYPE(c->qid);
	int vdidx = DEV(c->qid);
	
	switch(t){
	case Qtopdir:
		if(s == DEVDOTDOT){
			q = (Qid){QID(0, Qtopdir), 0, QTDIR};
			snprint(up->genbuf, sizeof up->genbuf, "#%C", vqdevtab.dc);
			devdir(c, q, up->genbuf, 0, eve, DMDIR|0555, dp);
			return 1;
		}
		return devgen(c, nil, topdir, nelem(topdir), s, dp);
	case Qvirtqs:
		if(s == DEVDOTDOT){
			q = (Qid){QID(0, Qtopdir), 0, QTDIR};
			snprint(up->genbuf, sizeof up->genbuf, "#%C", vqdevtab.dc);
			devdir(c, q, up->genbuf, 0, eve, DMDIR|0555, dp);
			return 1;
		}
		if(s >= nvq)
			return -1;
		char *dmap = mapdev(cvq[s]);
		snprint(up->genbuf, sizeof up->genbuf, "%s-%d", dmap?dmap:"virtio", s);
		q = (Qid) {QID(s, Qdevtop), 0, QTDIR};
		devdir(c, q, up->genbuf, 0, eve, DMDIR|0555, dp);
		return 1;
	case Qdevtop:
		vdidx = DEV(c->qid);
		if(vdidx >= nvq)
			return -1;
		if(s == 0) {					// device configuration area, report as a read-only file
			snprint(up->genbuf, sizeof up->genbuf, "devcfg");
			q = (Qid) {QID(vdidx, Qdevcfg), 0, 0};
			devdir(c, q, up->genbuf, cvq[vdidx]->dcfglen, eve, 0444, dp);
			if(cvq[vdidx]->dcmtime == -1)			// initialize lazily because at the moment of the driver bring-up
				cvq[vdidx]->dcmtime = seconds();	// seconds() does not seem to return correct value
			dp->mtime = cvq[vdidx]->dcmtime;
			return 1;
		} else {						// virtqueues: odd s yield data entries, even s yield control entries
			int t = s - 1;
			if(t >= cvq[vdidx]->nqs * 2)
				return -1;
			int nq = (t & ~1) >> 1;
			q = (Qid) {VQQID(nq, vdidx, (t&1)?QVqctl:Qvqdata), 0, 0};
			snprint(up->genbuf, sizeof up->genbuf, "vq%03d.%s", nq, (t&1)?"ctl":"data");
			devdir(c, q, up->genbuf, (t&1)?0:cvq[vdidx]->vqs[nq]->vr.num, eve, (t&1)?0222:0666, dp);
			return 1;
		}
	default:
		return -1;
	}
	return -1;
}


static Chan*
vqattach(char *spec)
{
	return devattach(vqdevtab.dc, spec);
}


Walkqid*
vqwalk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, (Dirtab *)0, 0, vqgen);
}

static int32_t
vqstat(Chan* c, uint8_t* dp, int32_t n)
{
	return devstat(c, dp, n, (Dirtab *)0, 0L, vqgen);
}

static Chan*
vqopen(Chan *c, int omode)
{
	uint t = TYPE(c->qid);
	uint vdidx = DEV(c->qid);
	if(vdidx >= nvq)
		error(Ebadarg);
	c = devopen(c, omode, (Dirtab*)0, 0, vqgen);
	switch(t) {
	default:
		break;
	}
	return c;
}

// To close a queue file, remove its flavor and free anything that can be held.

static void
vqclose(Chan* c)
{
	uint t = TYPE(c->qid);
	Virtq *vq;
	if(t == Qvqdata) {
		vq = qid2vq(c->qid);
		if (vq != nil && vq->flavor != nil && vq->flavor->cleanup != nil && vq->flavor->vdata != nil) {
			vq->flavor->cleanup(vq->flavor->vdata);
		}
	}
}


// Reading from the device configuration area is byte-by-byte (8bit ports) to avoid dealing with
// endianness.

static int32_t
vqread(Chan *c, void *va, int32_t n, int64_t offset)
{
	int vdidx = DEV(c->qid);
	int8_t *a;
	uint32_t r;
	Virtq *vq;
	switch(TYPE(c->qid)) {
	case Qtopdir:
	case Qvirtqs:
	case Qdevtop:
		return devdirread(c, va, n, (Dirtab *)0, 0L, vqgen);
	case Qdevcfg:
		if(vdidx >= nvq) {
			error(Ebadarg);
		}
		a = va;
		r = offset;
		int i;
		for(i = 0; i < n; a++, i++) {
			if(i + r >= cvq[vdidx]->dcfglen)
				break;
			uint8_t b = inb(cvq[vdidx]->port + cvq[vdidx]->dcfgoff + i + r);
			PBIT8(a, b);
		}
		return i;
	case Qvqdata:
		vq = qid2vq(c->qid);
		if(vq ==nil) {
			error(Ebadarg);
		}
		int rc = vq->flavor->read(vq, c, va, n, offset);
		if(rc >= 0) {
			return rc;
		} else {
			error(Eio);
		}
	default:
		return -1;
	}
}

static int32_t
vqwrite(Chan *c, void *va, int32_t n, int64_t offset)
{
	Virtq *vq;
	switch(TYPE(c->qid)) {
	case Qvqdata:
		vq = qid2vq(c->qid);
		if(vq ==nil) {
			error(Ebadarg);
		}
		int rc = vq->flavor->write(vq, c, va, n, offset);
		if(rc >= 0) {
			return rc;
		} else {
			error(Eio);
		}
	default:
		error(Eperm);
		return -1;
	}
}

// Scan virtqueues for the given device. If the vqs argument is not nil then
// nvq is expected to contain the length of the array vqs points to. In this case
// populate the Virtq structures for each virtqueue found. Otherwise just return
// the number of virtqueues detected. The port argument contains the base port
// for the device being scanned.
// Some devices like console report very large number of virtqueues. Whether it is a bug in QEMU
// or normal behavior we limit the maximum number of virtqueues serviced to 8.

static int
findvqs(uint32_t port, int nvq, Virtq **vqs)
{
	int cnt = 0;
	while(1) {
		outs(port + VIRTIO_PCI_QUEUE_SEL, cnt);
		int qs = ins(port + VIRTIO_PCI_QUEUE_NUM);
		if(cnt >= MAXVQS || qs == 0 || (qs & (qs-1)) != 0)
			break;
		if(vqs != nil) {
			// Allocate vq's descriptor space, used and available spaces, all page-aligned.
			vqs[cnt] = mallocz(sizeof(Virtq) + qs * sizeof(Rock *), 1);
			uint64_t vrsize = vring_size(qs, BY2PG);
			Virtq *q = vqs[cnt];
			q->vq = mallocalign(vrsize, BY2PG, 0, 0);
			memset(q->vq, 0, vrsize);
			vring_init(&q->vr, qs, q->vq, BY2PG);
			q->free = -1;
			q->nfree = qs;
			for(int i = 0; i < qs; i++) {
				q->vr.desc[i].next = q->free;
				q->free = i;
			}
			coherence();
			uint64_t paddr=PADDR(q->vq);
			outl(port + VIRTIO_PCI_QUEUE_PFN, paddr/BY2PG);
		}
		cnt++;
	}
	return cnt;
}

// Scan the PCI devices list for possible virtio devices. If the vcs argument
// is not nil then populate the array of control structures, otherwise just return
// the number of devices found. This function is intended to be called twice,
// once with vcs = nil just to count the devices, and the second time to populate
// the control structures, expecting vcs to point to an array of pointers to device
// descriptors of sufficient length.

static int
findvdevs(Vqctl **vcs)
{
	int cnt = 0;
	// TODO: this seems to work as if MSI-X is not enabled (device conf space starts at 20).
	// Find out how to deduce msix_enabled from the device.
	int msix_enabled = 0;
	Pcidev *p;
	// Scan the collected PCI devices info, find possible 9p devices
	for(p = nil; p = pcimatch(p, PCI_VENDOR_ID_REDHAT_QUMRANET, 0);) {
		if(vcs != nil) {
			vcs[cnt] = mallocz(sizeof(Vqctl), 1);
			if(vcs[cnt] == nil) {
				print("\nvq: failed to allocate device control structure for device %d\n", cnt);
				return cnt;
			}
			// Use the legacy interface
			// Allocate the BAR0 I/O space to the driver
			Vqctl *vc = vcs[cnt];
			vc->pci = p;
			vc->port = p->mem[0].bar & ~0x1;
			print("\nvq: device[%d] port %lx\n", cnt, vc->port);
			char name[32];
			char *dmap = mapdev(vc);
			snprint(name, sizeof(name), "%s-%d", dmap?dmap:"virtio", cnt);
			if(ioalloc(vc->port, p->mem[0].size, 0, name) < 0) {
				print("\nvq: port %lx in use\n", vc->port);
				free(vc);
				vcs[cnt] = nil;
				return cnt;
			}
			// Device reset
			outb(vc->port + VIRTIO_PCI_STATUS, 0);
			vc->feat = inl(vc->port + VIRTIO_PCI_HOST_FEATURES);
			print("\nvq: features %08x\n");
			outb(vc->port + VIRTIO_PCI_STATUS, VIRTIO_CONFIG_S_ACKNOWLEDGE|VIRTIO_CONFIG_S_DRIVER);
			int nqs = findvqs(vc->port, 0, nil);
			print("\nvq: found %d queues\n", nqs);
			// For each vq allocate and populate its descriptor
			if(nqs > 0) {
				vc->vqs = mallocz(nqs * sizeof(Virtq *), 1);
				vc->nqs = nqs;
				findvqs(vc->port, nqs, vc->vqs);
				for(int i = 0; i < nqs; i++) {
					Virtq *q = vc->vqs[i];
					q->idx = i;
					q->pdev = vc;
					q->flavor = mkdevflavor(q->pdev, q->idx);
				}
			}
			// Device config space contains data in consecutive 8bit input ports
			vc->dcfgoff = VIRTIO_PCI_CONFIG_OFF(msix_enabled);
			vc->dcfglen = vc->pci->mem[0].size - vc->dcfgoff;
			// Assume that the device config was modified just now
			vc->dcmtime = -1;
			// Enable interrupts, complete initialization
			intrenable(vc->pci->intl, vqintr, vc, vc->pci->tbdf, name);
			outb(vc->port + VIRTIO_PCI_STATUS, inb(vc->port + VIRTIO_PCI_STATUS) | VIRTIO_CONFIG_S_DRIVER_OK);
		}
		cnt++;
	}
	return cnt;
}

// Driver initialization: find all virtio9p devices (by vendor & device ID).
// Sense the device configuration.

void
vqinit(void)
{
	print("\nvq initializing\n");
	nvq = findvdevs(nil);
	print("\nvq: found %d devices\n", nvq);
	if(nvq == 0) {
		return;
	}
	cvq = mallocz(nvq * sizeof(Vqctl *), 1);
	if(cvq == nil) {
		print("\nvq: failed to allocate control structures\n");
		return;
	}
	findvdevs(cvq);
}

Dev vqdevtab = {
	.dc = 'Q',
	.name = "vq",

	.reset = devreset,
	.init = vqinit,
	.shutdown = devshutdown,
	.attach = vqattach,
	.walk = vqwalk,
	.stat = vqstat,
	.open = vqopen,
	.create = devcreate,
	.close = vqclose,
	.read = vqread,
	.bread = devbread,
	.write = vqwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};
