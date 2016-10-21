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

// Include the definitions from VIRTIO spec v1.0
// http://docs.oasis-open.org/virtio/virtio/v1.0/csprd02/listings/virtio_ring.h

#include	"virtio_ring.h"

#include	"virtio_config.h"
#include	"virtio_pci.h"

#include	"virtio_lib.h"

#define MAXVQS 8 			// maximal detectable number of VQs per device

static uint32_t nvq;		// number of the detected virtio9p devices

static Vqctl **cvq;			// array of device control structure pointers, length = nvq

// Map device identifiers to descriptive strings to display in IO port
// and interrupt allocation maps.

typedef struct
{
	uint16_t did;
	char *desc;
} didmap;

static didmap dmtab[] = {
	PCI_DEVICE_ID_VIRTIO_NET, "virtio-net",
	PCI_DEVICE_ID_VIRTIO_BLOCK, "virtio-block",
	PCI_DEVICE_ID_VIRTIO_BALLOON, "virtio-balloon",
	PCI_DEVICE_ID_VIRTIO_CONSOLE, "virtio-console",
	PCI_DEVICE_ID_VIRTIO_SCSI, "virtio-scsi",
	PCI_DEVICE_ID_VIRTIO_RNG, "virtio-rng",
	PCI_DEVICE_ID_VIRTIO_9P, "virtio-9p"
};

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
// The process issuing this call will be suspended until the I/O operation on the virtqueue
// completes. It is the calling process responsibility to return the used descriptors 
// to the queue.

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

// Allocate space for a single queue and initialize its descriptor. This is normally called at startup
// for every device's every queue discovered. It may however be necessary to process virtqueue hotplug
// events as with virtio-console, so this procedure can be called independently.

int
vqalloc(Virtq **pq, int qs)
{
	*pq = mallocz(sizeof(Virtq) + qs * sizeof(Rock *), 1);
	if(*pq == nil)
		return -1;
	Virtq *q = *pq;
	uint64_t vrsize = vring_size(qs, PGSZ);
	q->vq = mallocalign(vrsize, PGSZ, 0, 0);
	if(q->vq == nil)
		return -1;
	memset(q->vq, 0, vrsize);
	vring_init(&q->vr, qs, q->vq, PGSZ);
	q->free = -1;
	q->nfree = qs;
	for(int i = 0; i < qs; i++) {
		q->vr.desc[i].next = q->free;
		q->free = i;
	}
	return 0;
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
			if(vqalloc(&vqs[cnt], qs) < 0) {
				print("no memory to allocate a virtqueue\n");
				break;
			}
			coherence();
			uint64_t paddr=PADDR(vqs[cnt]->vq);
			outl(port + VIRTIO_PCI_QUEUE_PFN, paddr/PGSZ);
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

int
initvdevs(Vqctl **vcs)
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
				return cnt;
			}
			// Use the legacy interface
			// Allocate the BAR0 I/O space to the driver
			Vqctl *vc = vcs[cnt];
			vc->pci = p;
			vc->port = p->mem[0].bar & ~0x1;
			char *dmap = mapdev(vc);
			snprint(vc->devname, sizeof(vc->devname), "%s-%d", dmap?dmap:"virtio-pci", cnt);
			if(ioalloc(vc->port, p->mem[0].size, 0, vc->devname) < 0) {
				free(vc);
				vcs[cnt] = nil;
				return cnt;
			}
			// Device reset
			outb(vc->port + VIRTIO_PCI_STATUS, 0);
			outb(vc->port + VIRTIO_PCI_STATUS, VIRTIO_CONFIG_S_ACKNOWLEDGE|VIRTIO_CONFIG_S_DRIVER);
			int nqs = findvqs(vc->port, 0, nil);
			// For each vq allocate and populate its descriptor
			if(nqs > 0) {
				vc->vqs = mallocz(nqs * sizeof(Virtq *), 1);
				vc->nqs = nqs;
				findvqs(vc->port, nqs, vc->vqs);
				for(int i = 0; i < nqs; i++) {
					Virtq *q = vc->vqs[i];
					q->idx = i;
					q->pdev = vc;
				}
			}
			// Device config space contains data in consecutive 8bit input ports
			vc->dcfgoff = VIRTIO_PCI_CONFIG_OFF(msix_enabled);
			vc->dcfglen = vc->pci->mem[0].size - vc->dcfgoff;
			// Assume that the device config was modified just now
			vc->dcmtime = -1;
		}
		cnt++;
	}
	return cnt;
}

// Identity finction for device features.

static uint32_t 
acceptallfeat(uint32_t feat)
{
	return feat;
}

// Negotiate on device features. Read in the features bitmap, alter as needed by the function
// provided, write back to the device. If nil is provided as the function, write back unchanged
// that is, accept whatever is offered (often nothing). Return the feature bits accepted, store
// the same in the device control structure.

uint32_t
vdevfeat(Vqctl *vc, uint32_t(*ffltr)(uint32_t))
{
	uint32_t feat = inl(vc->port + VIRTIO_PCI_HOST_FEATURES);
	uint32_t rfeat = ffltr?(*ffltr)(feat):acceptallfeat(feat);
	rfeat &= feat;					// do not introduce new bits, we can only reject existing
	vc->feat = rfeat;
	outl(vc->port + VIRTIO_PCI_GUEST_FEATURES, rfeat);
	return rfeat;
}

// Final device initialization, enable interrupts.
// While initvdevs should be called once for all devices during the OS startup, finalinitdev
// should be called once per device, from the device-specific part of the driver. If the driver
// needs other interrupt handler than the default one, this function should not be called, and
// custom logic should be provided instead.

void
finalinitvdev(Vqctl *vc)
{
			intrenable(vc->pci->intl, vqintr, vc, vc->pci->tbdf, vc->devname);
			outb(vc->port + VIRTIO_PCI_STATUS, inb(vc->port + VIRTIO_PCI_STATUS) | VIRTIO_CONFIG_S_DRIVER_OK);
}

// Read device configuration area into the given buffer at the given offset in the area.
// Returned is number of bytes actually read. Reading is performed byte by byte, so endianness
// is preserved. The program that reads the configuration area should take care of endianness conversion.

int
readvdevcfg(Vqctl *vc, void *va, int32_t n, int64_t offset)
{
	int8_t *a = va;
	uint32_t r = offset;
	int i;
	for(i = 0; i < n; a++, i++) {
		if(i + r >= vc->dcfglen)
			break;
		uint8_t b = inb(vc->port + vc->dcfgoff + i + r);
		PBIT8(a, b);
	}
	return i;
}

// Initialize virtio globally (to be called once during startup).

void
virtiosetup()
{
	if(nvq != 0 || cvq != nil) 
		return;						// avoid repeated calls
	print("virtio: initializing\n");
	nvq = initvdevs(nil);
	if(nvq == 0) {
		nvq = -1;
		return;						// nothing found
	}
	cvq = mallocz(nvq * sizeof(Vqctl *), 1);
	if(cvq == nil) {
		print("virtiosetup: failed to allocate control structures\n");
		nvq = -1;
		return;
	}
	initvdevs(cvq);
	print("virtio: initialized\n");
}

// Get pointer to a virtio device by its index. Nil is returned if idx is out of range.

Vqctl *
vdevbyidx(uint32_t idx)
{
	if(idx >= nvq)
		return nil;
	return cvq[idx];
}

// Get total number of virtio devices defined at the moment.

uint32_t
getvdevnum(void)
{
	return nvq;
}

// Find all devices of given type (e. g. PCI_DEVICE_ID_VIRTIO_NET). An array of sufficient length
// should be provided; it will be filled out with the device references found. Returned is the number
// of devices found.

uint32_t
getvdevsbypciid(int pciid, Vqctl **vqs, uint32_t n)
{
	uint32_t j = 0;
	if(n < 1)
		return 0;
	for(int i = 0; i < nvq ; i++) {
		if(cvq[i]->pci->did == pciid)
			vqs[j++] = cvq[i];
		if(j >= n)
			break;
	}
	return j;
}
