/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dat.h"
#include "protos.h"

typedef struct Hdr Hdr;
struct Hdr {
	uchar	hdr;		/* RTP header */
	uchar	marker;		/* Payload and marker */
	uchar	seq[2];		/* Sequence number */
	uchar	ts[4];		/* Time stamp */
	uchar	ssrc[4];	/* Synchronization source identifier */
};

enum{
	RTPLEN = 12,		/* Minimum size of an RTP header */
};

static int
p_seprint(Msg *m)
{
	int cc, i;
	ushort seq;
	ulong ssrc, ts;
	Hdr*h;

	if(m->pe - m->ps < RTPLEN)
		return -1;

	h = (Hdr*)m->ps;
	cc = h->hdr & 0xf;
	if(m->pe - m->ps < RTPLEN + cc * 4)
		return -1;

	m->ps += RTPLEN;

	seq = NetS(h->seq);
	ts = NetL(h->ts);
	ssrc = NetL(h->ssrc);

	m->p = seprint(m->p, m->e, "version=%d x=%d cc=%d seq=%d ts=%ld ssrc=%ulx",
		(h->hdr >> 6) & 3, (h->hdr >> 4) & 1, cc, seq, ts, ssrc);
	for(i = 0; i < cc; i++){
		m->p = seprint(m->p, m->e, " csrc[%d]=%d", i, NetL(m->ps));
		m->ps += 4;
	}
	m->pr = nil;
	return 0;
}

Proto rtp = {
	"rtp",
	nil,
	nil,
	p_seprint,
	nil,
	nil,
	nil,
	defaultframer,
};
