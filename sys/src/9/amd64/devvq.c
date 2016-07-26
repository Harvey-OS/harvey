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
#include	"virtio_def.h"

// Include the definitions from VIRTIO spec v1.0
// http://docs.oasis-open.org/virtio/virtio/v1.0/csprd02/listings/virtio_ring.h

#include	"virtio_ring.h"

#include	"virtio_config.h"
#include	"virtio_pci.h"

#define TYPE(q)			((uint32_t)(q).path & 0x0F)
#define DEV(q)			((uint32_t)(((q).path >> 4) & 0x0FFF))
#define VQ(q)			((uint32_t)(((q).path >> 16) & 0x0FFFF))
#define QID(c, t)		((((c) & 0x0FFF)<<4) | ((t) & 0x0F))
#define VQQID(q, c, t)	((((q) & 0x0FFFF)<<16) | (((c) & 0x0FFF)<<4) | ((t) & 0x0F))

#define BY2PG PGSZ

enum
{
	Qtopdir = 0,			// top directory
	Qvirtqs,				// virtqs directory under the top
	Qdevtop,				// toplevel for each device, based on the PCI device number
	Qdevcfg,				// device config area read as a file
	Qvqdata,				// virtqueue file entry for reading-writing messages
	Qvqctl,					// virtqueue file entry for control operations, write-only
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

static int nvq;			// number of the detected virtio9p devices

typedef struct vqctl		// per-device control structure
{
	Pcidev *pci;			// PCI device descriptor
	uint32_t port;			// base I/O port for the legacy port-based interface
	uint32_t feat;			// host features
	uint32_t nqs;			// virt queues count
	Virtq **vqs;			// virt queues descriptors
	uint32_t dcfglen;		// device config area length
	uint32_t dcfgoff;		// device config area offset (20 or 24)
	int ownpid;				// PID of the process owning the device
} vqctl;

static vqctl **cvq;			// array of device control structure pointers, length = nvq

// Get the virtqueue reference from a QID. Returns a valid VQ reference, or nil
// if the QID does not correspond to an existing VQ.

static Virtq *
qid2vq(Qid q)
{
	int vdidx = DEV(q);
	int vqidx = VQ(q);
	if(vdidx >= nvq)
		return nil;
	vqctl *vc = cvq[vdidx];
	if(vqidx >= vc->nqs)
		return nil;
	return vc->vqs[vqidx];
}

static int
viodone(void *arg)
{
	return ((Rock*)arg)->done;
}

// The interrupt handler.

static void
vqinterrupt(Virtq *q)
{
	int id, free, m;
	Rock *r;
	Rendez *z;
	m = q->num - 1;
	ilock(&q->l);
	while((q->lastused ^ q->used->idx) & m) {
		id = q->used->ring[q->lastused++ & m].id;
		if(r = q->rock[id]){
			q->rock[id] = nil;
			z = r->sleep;
			r->done = 1;	/* hands off */
			if(z != nil)
				wakeup(z);
		}
		do {
			free = id;
			id = q->desc[free].next;
			q->desc[free].next = q->free;
			q->free = free;
			q->nfree++;
		} while(q->desc[free].flags & VIRTQ_DESC_F_NEXT);
	}
	iunlock(&q->l);
}

// Generic buffer read-write operation, based on 9front sdvirtio logic. Take buffer address and length,
// read/write direction. Get a free descriptor from the queue, fill it with the buffer information.
// Update queue index, notify the device. The Rock structure (sleep/wakeup semaphore) is kept on the
// process stack, and its address is stored in the queue descriptor's rock array at the index same
// as the index of the used buffer descriptor. When the device processes the buffer, it sends an interrupt.
// The interrupt handler scans the used descriptiors ring, and if a descriptor is found on which a process
// is waiting, the process is awaken, and the descriptor returns to the available ring.
// Return 0 if OK, -1 if error.
// Q: is it secure to give interrupt handler access to the sleeping process stack? Would it be better
// to preallocate all Rock's as part of the queue descriptor (128 bit per each vs. 64bit per a pointer).


int
vqrw(Virtq *q, void *a, uint64_t len, int rw)	// rw can be either 0 for read or VIRTQ_DESC_F_WRITE (2, other bits will be cleared)
{
	Proc *up = externup();
	int free, head;
	rw &= VIRTQ_DESC_F_WRITE;
	ilock(&q->l);
	while(q->nfree < 1) {
		iunlock(&q->l);
		if(!waserror())
			tsleep(&up->sleep, return0, 0, 500);
		poperror();
		ilock(&q->l);
	}
	head = free = q->free;
	struct virtq_desc *d = &q->desc[free];
	free = d->next;
	d->addr = PADDR(a);
	d->len = len;
	d->flags = rw;
	Rock rock;								// the sleep-wakeup semaphore on the process stack
	rock.done = 0;
	rock.sleep = &up->sleep;
	q->rock[head] = &rock;
	q->avail->ring[q->avail->idx & (q->num - 1)] = head;
	coherence();
	q->avail->idx++;
	iunlock(&q->l);
	if((q->used->flags & VIRTQ_AVAIL_F_NO_INTERRUPT) == 0) {
		vqctl *dev = q->pdev;
		outs(dev->port + VIRTIO_PCI_QUEUE_NOTIFY, q->idx);
	}
	while(!rock.done) {
		while(waserror())
			;
		tsleep(rock.sleep, viodone, &rock, 1000);
	}
	if(!rock.done)
		vqinterrupt(q);
	return 0;
}

static int
vqgen(Chan *c, char *d, Dirtab* dir, int i, int s, Dir *dp)
{
	Proc *up = externup();
	Qid q;
	int t = TYPE(c->qid);
	int vdidx = DEV(c->qid);
	
//print("\nvqgen: type %d, s %d\n", t, s);
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
		char *dmap = nil;
		for(int i = 0; i < nelem(dmtab) ; i++) {
			if(cvq[s]->pci->did == dmtab[i].did) {
				dmap = dmtab[i].desc;
				break;
			}
		}
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
			return 1;
		} else {						// virtqueues: odd s yield data entries, even s yield control entries
			int t = s - 1;
			if(t >= cvq[vdidx]->nqs * 2)
				return -1;
			int nq = (t & ~1) >> 1;
			q = (Qid) {VQQID(nq, vdidx, (t&1)?Qvqctl:Qvqdata), 0, 0};
			snprint(up->genbuf, sizeof up->genbuf, "vq%03d.%s", nq, (t&1)?"ctl":"data");
			devdir(c, q, up->genbuf, (t&1)?0:cvq[vdidx]->vqs[nq]->num, eve, (t&1)?0222:0666, dp);
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
print("vqattach %s\n", spec);
	return devattach(vqdevtab.dc, spec);
}


Walkqid*
vqwalk(Chan* c, Chan *nc, char** name, int nname)
{
//print("vqwalk %d -> %d\n", c?TYPE(c->qid):-1, nc?TYPE(nc->qid):-1);
	return devwalk(c, nc, name, nname, (Dirtab *)0, 0, vqgen);
}

static int32_t
vqstat(Chan* c, uint8_t* dp, int32_t n)
{
//print("vqstat %d\n", TYPE(c->qid));
	return devstat(c, dp, n, (Dirtab *)0, 0L, vqgen);
}

// A process holding open the device configuration area has access to its virtqueues.
// Other processes will be denied any access. PID of the claiming process is stored
// in the ownpid field of the device descriptor. An unclaimed device has -1 in it.

static Chan*
vqopen(Chan *c, int omode)
{
//print("v9open %d\n", TYPE(c->qid));
	Proc *up = externup();
	uint t = TYPE(c->qid);
	uint vdidx = DEV(c->qid);
	if(vdidx >= nvq)
		error(Ebadarg);
	vqctl *vc = cvq[vdidx];
	switch(t) {
		case Qdevcfg:
			if(vc->ownpid != -1)
				error(Eperm);
			vc->ownpid = up->pid;
		case Qvqdata:
		case Qvqctl:
			if(vc->ownpid != up->pid)
				error(Eperm);
			break;
		default:
			break;
	}
	c = devopen(c, omode, (Dirtab*)0, 0, vqgen);
	switch(t) {
	default:
		break;
	}
	return c;
}

static void
vqclose(Chan* c)
{
//print("vqclose %d\n", TYPE(c->qid));
	Proc *up = externup();
	uint t = TYPE(c->qid);
	uint vdidx = DEV(c->qid);
	if(vdidx >= nvq)
		return;
	vqctl *vc = cvq[vdidx];
	if (vc->ownpid != up->pid)
		return;
	switch(t) {
		case Qdevcfg:
			vc->ownpid = -1;
			break;
		default:
			break;
	}
}



// Reading from the device configuration area is byte-by-byte (8bit ports) to avoid dealing with
// endianness.

static int32_t
vqread(Chan *c, void *va, int32_t n, int64_t offset)
{
//print("vqread %d %d %d\n", TYPE(c->qid), n, offset);
	int vdidx = DEV(c->qid);
	int8_t *a;
	uint32_t r;
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
	default:
		return -1;
	}
}

static int32_t
vqwrite(Chan *c, void *va, int32_t n, int64_t offset)
{
//print("vqwrite %d %d %d\n", TYPE(c->qid), n, offset);
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
//print("\nvq queue %d size %d\n", cnt, qs);
		if(cnt >= 16 || qs == 0 || (qs & (qs-1)) != 0)
			break;
		if(vqs != nil) {
			// Allocate vq's descriptor space, used and available spaces, all page-aligned.
			vqs[cnt] = malloc(sizeof(Virtq) + qs * sizeof(Rock *));
			Virtq *q = vqs[cnt];
			q->num = qs;
			uint64_t descsz = qs * sizeof(struct virtq_desc);
			uint64_t availsz = sizeof(struct virtq_avail) + qs * sizeof(le16);
			uint64_t usedsz = sizeof(struct virtq_used) + qs * sizeof(struct virtq_used_elem);
			uint8_t *p = mallocalign(PGROUND(descsz + availsz + sizeof(le16)) + PGROUND(usedsz + sizeof(le16)), BY2PG, 0, 0);
			q->desc = (struct virtq_desc *)p;
			q->avail = (struct virtq_avail *)(p + descsz);
			q->used = (struct virtq_used *)(p + PGROUND(descsz + availsz));
			q->usedevent = (le16 *)(p + descsz + availsz);
			q->availevent = (le16 *)(p + PGROUND(descsz + availsz) + usedsz);
			q->free = -1;
			q->nfree = qs;
			// So in 9front: each descriptor's free field points to the next descriptor
			for(int i = 0; i < qs; i++) {
				q->desc[i].next = q->free;
				q->free = i;
			}
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
// descriptors of sufficient length. Portions of code are borrowed from the 9front virtio
// drivers.

static int
find9p(vqctl **vcs)
{
	int cnt = 0;
	// TODO: this seems to work as if MSI-X is not enabled (device conf space starts at 20).
	// Find out how to deduce msix_enabled from the device.
	int msix_enabled = 0;
	Pcidev *p;
	// Scan the collected PCI devices info, find possible 9p devices
	for(p = nil; p = pcimatch(p, PCI_VENDOR_ID_REDHAT_QUMRANET, 0);) {
		if(vcs != nil) {
			vcs[cnt] = malloc(sizeof(vqctl));
			if(vcs[cnt] == nil) {
				print("\nvq: failed to allocate device control structure for device %d\n", cnt);
				return cnt;
			}
			// Use the legacy interface
			// Allocate the BAR0 I/O space to the driver
			vqctl *vc = vcs[cnt];
			vc->pci = p;
			vc->port = p->mem[0].bar & ~0x1;
			print("\nvq: device[%d] port %lux\n", cnt, vc->port);
			if(ioalloc(vc->port, p->mem[0].size, 0, "virtio9p") < 0) {
				print("\nvq: port %lux in use\n", vc->port);
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
			// No process has claimed the device yet
			vc->ownpid = -1;
		}
		cnt++;
	}
	return cnt;
}

// Driver initialization: find all virtio9p devices (by vendor & device ID).
// Sense the device configuration, figure out mount tags. We assume that the
// virtio9p devices do not appear or disappear between reboots.

void
vqinit(void)
{
	print_func_entry();
	print("\nvq initializing\n");
	nvq = find9p(nil);
	print("\nvq: found %d devices\n", nvq);
	if(nvq == 0) {
		return;
	}
	cvq = malloc(nvq * sizeof(vqctl *));
	if(cvq == nil) {
		print("\nvq: failed to allocate control structures\n");
		return;
	}
	find9p(cvq);
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
