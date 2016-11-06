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
	uint8_t	hdr;		/* RTCP header */
	uint8_t	pt;		/* Packet type */
	uint8_t	len[2];		/* Report length */
	uint8_t	ssrc[4];	/* Synchronization source identifier */
	uint8_t	ntp[8];		/* NTP time stamp */
	uint8_t	rtp[4];		/* RTP time stamp */
	uint8_t	pktc[4];	/* Sender's packet count */
	uint8_t	octc[4];	/* Sender's octet count */
};

typedef struct Report Report;
struct Report {
	uint8_t	ssrc[4];	/* SSRC identifier */
	uint8_t	lost[4];	/* Fraction + cumu lost */
	uint8_t	seqhi[4];	/* Highest seq number received */
	uint8_t	jitter[4];	/* Interarrival jitter */
	uint8_t	lsr[4];		/* Last SR */
	uint8_t	dlsr[4];	/* Delay since last SR */
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
