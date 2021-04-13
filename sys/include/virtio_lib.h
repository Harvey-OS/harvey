/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

// Definitions for the virtqueue handling functions library.

// Specific device control structures. They are preallocated during the driver
// initialization and remain mainly constant during the kernel uptime.

struct virtq;

typedef struct vqctl		// per-device control structure
{
	Pcidev *pci;			// PCI device descriptor
	u32 port;			// base I/O port for the legacy port-based interface
	u32 feat;			// host features
	u32 nqs;			// virt queues count
	struct virtq **vqs;		// virt queues descriptors
	u32 dcfglen;		// device config area length
	u32 dcfgoff;		// device config area offset (20 or 24)
	long dcmtime;			// device config area modification time
	char devname[32];		// device name to show in port and interrupt allocations
} Vqctl;

typedef struct rock {
	int done;
	Rendez *sleep;
} Rock;

// Customized virtqueue

typedef struct virtq {
	struct vring vr;			// vring descriptor per spec
	u8 *vq;				// vring data shared between host and guest
	u16 lastused;
	u16 waiting;
	Lock l;
	u32 free;
	u32 nfree;
	void *pdev;					// use this to reference the virtio device control structure which is per-driver
	int idx;					// driver use only, index of the queue per device
	void *pqdata;				// per-queue private data (may be shared between queues of the same device)
	Rock *rock[];				// array of pointers to the waiting processes, length same as queue length
} Virtq;

// Common virtqueue functions and macros.

int getdescr(Virtq *q, int n, u16 *descr);

int queuedescr(Virtq *q, int n, u16 *descr);

void reldescr(Virtq *q, int n, u16 *descr);

int initvdevs(Vqctl **vcs);

int vqalloc(Virtq **pq, int qs);

void finalinitvdev(Vqctl *vc);

int readvdevcfg(Vqctl *vc, void *va, i32 n, i64 offset);

Vqctl *vdevbyidx(u32 idx);

u32 vdevfeat(Vqctl *vc, u32(*ffltr)(u32));

u32 getvdevnum(void);

u32 getvdevsbypciid(int pciid, Vqctl **vqs, u32 n);

static inline struct vring_desc * q2descr(Virtq *q, int i) { return q->vr.desc + i; }

// Unified QID conversions between values and device/queue indices. We allocate bits:
// 0 - 3 for QID type (specific to each driver)
// 4 - 15 for device index (to use with vdevbyidx)
// 16 - 32 for queue index within device

// Extract QID type

#define TYPE(q)			((u32)(q).path & 0x0F)

// Extract device index

#define DEV(q)			((u32)(((q).path >> 4) & 0x0FFF))

// Extract queue index

#define VQ(q)			((u32)(((q).path >> 16) & 0x0FFFF))

// Construct a non-queue aware QID (to address a per-device file)

#define QID(c, t)		((((c) & 0x0FFF)<<4) | ((t) & 0x0F))

// Construct a queue-aware QID (to address a per-queue file)

#define VQQID(q, c, t)	((((q) & 0x0FFFF)<<16) | (((c) & 0x0FFF)<<4) | ((t) & 0x0F))
