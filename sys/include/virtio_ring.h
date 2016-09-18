/* An interface for efficient virtio implementation.
 *
 * This header is BSD licensed so anyone can use the definitions
 * to implement compatible drivers/servers.
 *
 * Copyright 2007, 2009, IBM Corporation
 * Copyright 2011, Red Hat, Inc
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of IBM nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* This marks a buffer as continuing via the next field. */
#define VRING_DESC_F_NEXT       1
/* This marks a buffer as write-only (otherwise read-only). */
#define VRING_DESC_F_WRITE      2
/* This means the buffer contains a list of buffer descriptors. */
#define VRING_DESC_F_INDIRECT   4

/* The device uses this in used->flags to advise the driver: don't kick me
 * when you add a buffer.  It's unreliable, so it's simply an
 * optimization. */
#define VRING_USED_F_NO_NOTIFY  1
/* The driver uses this in avail->flags to advise the device: don't
 * interrupt me when you consume a buffer.  It's unreliable, so it's
 * simply an optimization.  */
#define VRING_AVAIL_F_NO_INTERRUPT      1

/* Support for indirect descriptors */
#define VIRTIO_RING_F_INDIRECT_DESC    28

/* Support for avail_idx and used_idx fields */
#define VIRTIO_RING_F_EVENT_IDX        29

/* Arbitrary descriptor layouts. */
#define VIRTIO_F_ANY_LAYOUT            27

/* Virtio ring descriptors: 16 bytes.
 * These can chain together via "next". */
struct vring_desc {
        /* Address (guest-physical). */
        uint64_t addr;
        /* Length. */
        uint32_t len;
        /* The flags as indicated above. */
        uint16_t flags;
        /* We chain unused descriptors via this, too */
        uint16_t next;
};

struct vring_avail {
        uint16_t flags;
        uint16_t idx;
        uint16_t ring[];
        /* Only if VIRTIO_RING_F_EVENT_IDX: uint16_t used_event; */
};

/* uint32_t is used here for ids for padding reasons. */
struct vring_used_elem {
        /* Index of start of used descriptor chain. */
        uint32_t id;
        /* Total length of the descriptor chain which was written to. */
        uint32_t len;
};

struct vring_used {
        uint16_t flags;
        uint16_t idx;
        struct vring_used_elem ring[];
        /* Only if VIRTIO_RING_F_EVENT_IDX: uint16_t avail_event; */
};

struct vring {
        unsigned int num;

        struct vring_desc *desc;
        struct vring_avail *avail;
        struct vring_used *used;
};

/* The standard layout for the ring is a continuous chunk of memory which
 * looks like this.  We assume num is a power of 2.
 *
 * struct vring {
 *      // The actual descriptors (16 bytes each)
 *      struct vring_desc desc[num];
 *
 *      // A ring of available descriptor heads with free-running index.
 *      uint16_t avail_flags;
 *      uint16_t avail_idx;
 *      uint16_t available[num];
 *      uint16_t used_event_idx; // Only if VIRTIO_RING_F_EVENT_IDX
 *
 *      // Padding to the next align boundary.
 *      char pad[];
 *
 *      // A ring of used descriptor heads with free-running index.
 *      uint16_t used_flags;
 *      uint16_t used_idx;
 *      struct vring_used_elem used[num];
 *      uint16_t avail_event_idx; // Only if VIRTIO_RING_F_EVENT_IDX
 * };
 * Note: for virtio PCI, align is 4096.
 */
static inline void vring_init(struct vring *vr, unsigned int num, void *p,
                              unsigned long align)
{
        vr->num = num;
        vr->desc = p;
        vr->avail = p + num*sizeof(struct vring_desc);
        vr->used = (void *)(((unsigned long)&vr->avail->ring[num] + sizeof(uint16_t)
                              + align-1)
                            & ~(align - 1));
}

static inline unsigned vring_size(unsigned int num, unsigned long align)
{
        return ((sizeof(struct vring_desc)*num + sizeof(uint16_t)*(3+num)
                 + align - 1) & ~(align - 1))
                + sizeof(uint16_t)*3 + sizeof(struct vring_used_elem)*num;
}

static inline int vring_need_event(uint16_t event_idx, uint16_t new_idx, uint16_t old_idx)
{
         return (uint16_t)(new_idx - event_idx - 1) < (uint16_t)(new_idx - old_idx);
}

/* Get location of event indices (only with VIRTIO_RING_F_EVENT_IDX) */
static inline uint16_t *vring_used_event(struct vring *vr)
{
        /* For backwards compat, used event index is at *end* of avail ring. */
        return &vr->avail->ring[vr->num];
}

static inline uint16_t *vring_avail_event(struct vring *vr)
{
        /* For backwards compat, avail event index is at *end* of used ring. */
        return (uint16_t *)&vr->used->ring[vr->num];
}
