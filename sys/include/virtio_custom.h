/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

// Customized data types for virtio queue handling.

// Specific device control structures. They are preallocated during the driver
// initialization and remain mainly constant during the kernel uptime.

struct virtq;

typedef struct vqctl		// per-device control structure
{
	Pcidev *pci;			// PCI device descriptor
	uint32_t port;			// base I/O port for the legacy port-based interface
	uint32_t feat;			// host features
	uint32_t nqs;			// virt queues count
	struct virtq **vqs;		// virt queues descriptors
	uint32_t dcfglen;		// device config area length
	uint32_t dcfgoff;		// device config area offset (20 or 24)
	long dcmtime;			// device config area modification time
} Vqctl;

typedef struct rock {
	int done;
	Rendez *sleep;
} Rock;

// Virtio flavor. We set it per queue, however in most cases the flavor is the same across
// the whole device type. the structure defines read/write implementations as well as
// holds reference to a per-queue data.

typedef struct flavor {
	void *vdata;					// per-queue specific data
	struct flavor *(*mkflavor)(struct vqctl *dev, int idx);	// constructor takes device reference and queue index
	void (*cleanup)(void *vdata);
	int32_t (*read)(struct virtq *, Chan*, void*, int32_t, int64_t);
	int32_t (*write)(struct virtq *, Chan*, void*, int32_t, int64_t);
} Flavor;

typedef void (*cleanup_t)(void *);

extern Flavor flavor_9p;

// Customized virtqueue

typedef struct virtq {
		struct vring vr;			// vring descriptor per spec
		uint8_t *vq;				// vring data shared between host and guest
        uint16_t lastused;
        uint16_t waiting;
		Lock l;
        uint free;
        uint nfree;
        void *pdev;					// use this to reference the virtio device control structure which is per-driver
        int idx;					// driver use only, index of the queue per device
        Flavor *flavor;				// per-device flavor: different types of virtio devices require different behavior of read/write
        Rock *rock[];				// array of pointers to the waiting processes, length same as queue length
} Virtq;

// Common virtqueue functions and macros.

int getdescr(Virtq *q, int n, uint16_t *descr);

int queuedescr(Virtq *q, int n, uint16_t *descr);

void reldescr(Virtq *q, int n, uint16_t *descr);

static inline struct vring_desc * q2descr(Virtq *q, int i) { return q->vr.desc + i; }
