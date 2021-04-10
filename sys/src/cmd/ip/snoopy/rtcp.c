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
	u8	hdr;		/* RTCP header */
	u8	pt;		/* Packet type */
	u8	len[2];		/* Report length */
	u8	ssrc[4];	/* Synchronization source identifier */
	u8	ntp[8];		/* NTP time stamp */
	u8	rtp[4];		/* RTP time stamp */
	u8	pktc[4];	/* Sender's packet count */
	u8	octc[4];	/* Sender's octet count */
};

typedef struct Report Report;
struct Report {
	u8	ssrc[4];	/* SSRC identifier */
	u8	lost[4];	/* Fraction + cumu lost */
	u8	seqhi[4];	/* Highest seq number received */
	u8	jitter[4];	/* Interarrival jitter */
	u8	lsr[4];		/* Last SR */
	u8	dlsr[4];	/* Delay since last SR */
};

enum{
	RTCPLEN = 28,		/* Minimum size of an RTCP header */
	REPORTLEN = 24,
};

static int
p_seprint(Msg *m)
{
	int rc, i, frac;
	float dlsr;
	Hdr*h;
	Report*r;

	if(m->pe - m->ps < RTCPLEN)
		return -1;

	h = (Hdr*)m->ps;
	if(m->pe - m->ps < (NetS(h->len) + 1) * 4)
		return -1;

	rc = h->hdr & 0x1f;
	m->ps += RTCPLEN;
	m->p = seprint(m->p, m->e, "version=%d rc=%d tp=%d ssrc=%8ux "
		"ntp=%d.%.10u rtp=%d pktc=%d octc=%d hlen=%d",
		(h->hdr >> 6) & 3, rc, h->pt, NetL(h->ssrc), NetL(h->ntp),
		(unsigned int)NetL(&h->ntp[4]), NetL(h->rtp), NetL(h->pktc),
		NetL(h->octc), (NetS(h->len) + 1) * 4);

	for(i = 0; i < rc; i++){
		r = (Report*)m->ps;
		m->ps += REPORTLEN;

		frac = (int)((r->lost[0] * 100.) / 256.);
		r->lost[0] = 0;
		dlsr = NetL(r->dlsr) / 65536.;

		m->p = seprint(m->p, m->e, "\n\trr(csrc=%8ux frac=%3d%% "
			"cumu=%10d seqhi=%10ud jitter=%10d lsr=%8ux dlsr=%f)",
			NetL(r->ssrc), frac, NetL(r->lost), NetL(r->seqhi),
			NetL(r->jitter), NetL(r->lsr), dlsr);
	}
	m->pr = nil;
	return 0;
}

Proto rtcp = {
	"rtcp",
	nil,
	nil,
	p_seprint,
	nil,
	nil,
	nil,
	defaultframer,
};
