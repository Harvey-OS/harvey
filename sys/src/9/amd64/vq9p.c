/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

// vq9p.c: 9p flavor of read-write operations on virtqueue.

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

// The specifics of QEMU virtio 9p implementation is that the device requires at least one buffer sent
// and one received within the same request, otherwise QEMU crashes. Since we are working via file handle,
// and no multi-buffer (sg-list) operations are available, it is reasonable to expect that a write request
// (message sent to the 9p server) will be followed by a read request (response received from the 9p server).
// Thus, write requests are taken asynchronously (the issuing process does not wait), and read requests
// cause both buffers to be added to the virtqueue. It is highly expected that these requests are properly serialized,
// and the file handle is not used by several processes simultaneously. Basically, internal state needs to be maintained
// to manipulate these requests. Also, it is likely that the requesting process would reuse the same buffer for both
// request and response. It is therefore necessary to make a copy of the request buffer and add the copy to the virtqueue.

enum
{
	STATE_INIT = 0,				// initial state
	STATE_HOLD_WRITE,			// a write request is held in queue
	STATE_HOLD_READ,			// a read request us held in queue
	STATE_SENT,					// a read request was submitted, and both were sent to the queue
};

typedef struct vq9data {
	uint16_t state;				// internal state
	void *outbuf;				// output buffer to hold
	int32_t outlen;				// its length
	void *inbuf;				// input buffer to hold
	int32_t inlen;				// its length
	int32_t freeout;			// if true, free the outbuf when done
} Vq9data;

#define VQ9DATA(q) (Vq9data *)((q)->flavor->vdata)

// Clean up the data.

static void
vq9cleanup(Vq9data *vqd)
{
	if(vqd->freeout) free(vqd->outbuf);
	vqd->freeout = 0;
	vqd->inbuf = nil;
	vqd->inlen = 0;
	vqd->outbuf = nil;
	vqd->outlen = 0;
	vqd->state = STATE_SENT;
}

// Proceed with request.

static int32_t
do_request(Virtq *vq)
{
	Vq9data *vqd = VQ9DATA(vq);
	uint16_t descr[1];
	int rc = getdescr(vq, 1, descr);
	if(rc < 1) {
		error("Insufficient number of descriptors in virtqueue");
		return -1;
	}
	struct vring_desc req[2] = {
		{
			.addr = PADDR(vqd->outbuf),
			.len = vqd->outlen,
			.flags = VRING_DESC_F_NEXT,
			.next = 1
		},
		{
			.addr = PADDR(vqd->inbuf),
			.len = vqd->inlen,
			.flags = VRING_DESC_F_WRITE,
			.next = 0
		}
	};
	q2descr(vq, descr[0])->addr = PADDR(&req);
	q2descr(vq, descr[0])->len = sizeof(req);
	q2descr(vq, descr[0])->flags = VRING_DESC_F_INDIRECT;
	q2descr(vq, descr[0])->next = 0;
	queuedescr(vq, 1, descr);
	reldescr(vq, 1, descr);
	int32_t retval;
	if(vqd->state == STATE_HOLD_READ) {			// write request was the trigger
		retval = vqd->outlen;
	} else if(vqd->state == STATE_HOLD_WRITE) {	// read request was the trigger, parse the message
		Fcall f;								// get length, return it
		uint8_t xbuf[vqd->inlen];
		memmove(xbuf, vqd->inbuf, vqd->inlen);
		uint l = convM2S(xbuf, vqd->inlen, &f);
		retval = l;
	} else retval = -1;
	vq9cleanup(vqd);
	return retval;
}


// Read (receive a response) needs a write request to be held already to send both.
// Obtain 2 descriptors from the virtqueue, the first holds the request buffer,
// the second holds the response buffer.

static int32_t
vq9pread(Virtq *vq, Chan *c, void *va, int32_t n, int64_t offset)
{
	Vq9data *vqd = VQ9DATA(vq);
	switch(vqd->state) {
		default:
			error("vq9pwrite: internal state error");
			return -1;
		case STATE_INIT:					// initial state, or just sent and holding nothing
		case STATE_SENT:					// save the input buffer and return to the caller
			vqd->inbuf = va;
			vqd->inlen = n;
			vqd->state = STATE_HOLD_READ;
			return n;
		case STATE_HOLD_READ:				// input buffer already queued, this is a client error
			error("Internal state error: already holding input read buffer");
			return -1;
		case STATE_HOLD_WRITE:				// already holding an output buffer, proceed with request
			vqd->inbuf = va;
			vqd->inlen = n;
			return do_request(vq);
	}
	return -1;
}

// Write (send a request) returns immediately, holding the copy of the buffer
// supplied, so the caller may reuse the same buffer for the server's response.

static int32_t
vq9pwrite(Virtq *vq, Chan *c, void *va, int32_t n, int64_t offset)
{
	Vq9data *vqd = VQ9DATA(vq);
	switch(vqd->state) {
		default:
			error("vq9pwrite: internal state error");
			return -1;
		case STATE_INIT:					// initial state, or just sent and holding nothing
		case STATE_SENT:					// save the output buffer and return to the caller
			vqd->outbuf = mallocz(n, 1);
			if(vqd->outbuf == nil) {
				error("No memory for write buffer copy");
				return -1;
			}
			memmove(vqd->outbuf, va, n);
			vqd->outlen = n;
			vqd->state = STATE_HOLD_WRITE;
			vqd->freeout = 1;
			return n;
		case STATE_HOLD_WRITE:				// output buffer already queued, this is a client error
			error("Internal state error: already holding output write buffer");
			return -1;
		case STATE_HOLD_READ:				// read buffer is queued, proceed with request
			vqd->outbuf = va;
			vqd->outlen = n;
			vqd->freeout = 0;
			return do_request(vq);
	}
	return -1;
}

// Allocate a per-queue read-write operations flavor descriptor. A 9p device is expected to have
// only one virt queue, so indices larger than 0 result in dummy flavor allocation (this function
// will return nil).

static Flavor *
mk9pflavor(Vqctl *vc, int idx)
{
	if(idx > 0)
		return nil;
	Flavor *flv = mallocz(sizeof(Flavor), 1);
	if(flv == nil) {
		error("mk9pflavor: memory exhausted");
		return nil;
	}
	*flv = flavor_9p;
	flv->vdata = mallocz(sizeof(Vq9data), 1);
	if(flv->vdata == nil) {
		error("mk9pflavor: memory exhausted");
		return nil;
	}
	Vq9data *vd = flv->vdata;
	vd->state = STATE_INIT;
	return flv;
}

Flavor flavor_9p = {
	.vdata = nil,
	.mkflavor = mk9pflavor,
	.cleanup = (cleanup_t)vq9cleanup,
	.read = vq9pread,
	.write = vq9pwrite
};
