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

typedef struct Hdr	Hdr;
struct Hdr
{
	uint8_t	vihl;		/* Version and header length */
	uint8_t	tos;		/* Type of service */
	uint8_t	length[2];	/* packet length */
	uint8_t	id[2];		/* ip->identification */
	uint8_t	frag[2];	/* Fragment information */
	uint8_t	ttl;		/* Time to live */
	uint8_t	proto;		/* Protocol */
	uint8_t	cksum[2];	/* Header checksum */
	uint8_t	src[4];		/* IP source */
	uint8_t	dst[4];		/* IP destination */
};

enum
{
	IPHDR		= 20,		/* sizeof(Iphdr) */
	IP_VER		= 0x40,		/* Using IP version 4 */
	IP_DF		= 0x4000,	/* Don't fragment */
	IP_MF		= 0x2000,	/* More fragments */
};

static Mux p_mux[] =
{
	{ "icmp", 1, },
	{ "igmp", 2, },
	{ "ggp", 3, },
	{ "ip", 4, },
	{ "st", 5, },
	{ "tcp", 6, },
	{ "ucl", 7, },
	{ "egp", 8, },
	{ "igp", 9, },
	{ "bbn-rcc-mon", 10, },
	{ "nvp-ii", 11, },
	{ "pup", 12, },
	{ "argus", 13, },
	{ "emcon", 14, },
	{ "xnet", 15, },
	{ "chaos", 16, },
	{ "udp", 17, },
	{ "mux", 18, },
	{ "dcn-meas", 19, },
	{ "hmp", 20, },
	{ "prm", 21, },
	{ "xns-idp", 22, },
	{ "trunk-1", 23, },
	{ "trunk-2", 24, },
	{ "leaf-1", 25, },
	{ "leaf-2", 26, },
	{ "rdp", 27, },
	{ "irtp", 28, },
	{ "iso-tp4", 29, },
	{ "netblt", 30, },
	{ "mfe-nsp", 31, },
	{ "merit-inp", 32, },
	{ "sep", 33, },
	{ "3pc", 34, },
	{ "idpr", 35, },
	{ "xtp", 36, },
	{ "ddp", 37, },
	{ "idpr-cmtp", 38, },
	{ "tp++", 39, },
	{ "il", 40, },
	{ "sip", 41, },
	{ "sdrp", 42, },
	{ "sip-sr", 43, },
	{ "sip-frag", 44, },
	{ "idrp", 45, },
	{ "rsvp", 46, },
	{ "gre", 47, },
	{ "mhrp", 48, },
	{ "bna", 49, },
	{ "sipp-esp", 50, },
	{ "sipp-ah", 51, },
	{ "i-nlsp", 52, },
	{ "swipe", 53, },
	{ "nhrp", 54, },
	{ "any", 61, },
	{ "cftp", 62, },
	{ "any", 63, },
	{ "sat-expak", 64, },
	{ "kryptolan", 65, },
	{ "rvd", 66, },
	{ "ippc", 67, },
	{ "any", 68, },
	{ "sat-mon", 69, },
	{ "visa", 70, },
	{ "ipcv", 71, },
	{ "cpnx", 72, },
	{ "cphb", 73, },
	{ "wsn", 74, },
	{ "pvp", 75, },
	{ "br-sat-mon", 76, },
	{ "sun-nd", 77, },
	{ "wb-mon", 78, },
	{ "wb-expak", 79, },
	{ "iso-ip", 80, },
	{ "vmtp", 81, },
	{ "secure-vmtp", 82, },
	{ "vines", 83, },
	{ "ttp", 84, },
	{ "nsfnet-igp", 85, },
	{ "dgp", 86, },
	{ "tcf", 87, },
	{ "igrp", 88, },
	{ "ospf", 89, },
	{ "sprite-rpc", 90, },
	{ "larp", 91, },
	{ "mtp", 92, },
	{ "ax.25", 93, },
	{ "ipip", 94, },
	{ "micp", 95, },
	{ "scc-sp", 96, },
	{ "etherip", 97, },
	{ "encap", 98, },
	{ "any", 99, },
	{ "gmtp", 100, },
	{ "rudp", 254, },
	{ 0 }
};

enum
{
	Os,	/* source */
	Od,	/* destination */
	Osd,	/* source or destination */
	Ot,	/* type */
};

static Field p_fields[] =
{
	{"s",	Fv4ip,	Os,	"source address",	} ,
	{"d",	Fv4ip,	Od,	"destination address",	} ,
	{"a",	Fv4ip,	Osd,	"source|destination address",} ,
	{"sd",	Fv4ip,	Osd,	"source|destination address",} ,
	{"t",	Fnum,	Ot,	"sub protocol number",	} ,
	{0}
};

static void
p_compile(Filter *f)
{
	Mux *m;

	if(f->op == '='){
		compile_cmp(ip.name, f, p_fields);
		return;
	}
	for(m = p_mux; m->name != nil; m++)
		if(strcmp(f->s, m->name) == 0){
			f->pr = m->pr;
			f->ulv = m->val;
			f->subop = Ot;
			return;
		}
	sysfatal("unknown ip field or protocol: %s", f->s);
}

static int
p_filter(Filter *f, Msg *m)
{
	Hdr *h;

	if(m->pe - m->ps < IPHDR)
		return 0;

	h = (Hdr*)m->ps;
	m->ps += (h->vihl & 0xf) << 2;

	switch(f->subop){
	case Os:
		return NetL(h->src) == f->ulv;
	case Od:
		return NetL(h->dst) == f->ulv;
	case Osd:
		return NetL(h->src) == f->ulv || NetL(h->dst) == f->ulv;
	case Ot:
		return h->proto == f->ulv;
	}
	return 0;
}

static int
p_seprint(Msg *m)
{
	int f, len, hl;
	uint8_t *p;
	Hdr *h;

	if(m->pe - m->ps < IPHDR)
		return -1;
	h = (Hdr*)m->ps;

	/* next protocol, just dump unless this is the first fragment */
	m->pr = &dump;
	f = NetS(h->frag);
	if((f & ~(IP_DF|IP_MF)) == 0)
		demux(p_mux, h->proto, h->proto, m, &dump);

	/* truncate the message if there's extra */
	len = NetS(h->length);
	if(len < m->pe - m->ps)
		m->pe = m->ps + len;

	/* next header */
	hl = (h->vihl  &0xf) << 2;

	m->p = seprint(m->p, m->e, "s=%V d=%V id=%4.4x frag=%4.4x ttl=%3d pr=%d ln=%d hl=%d",
		h->src, h->dst, NetS(h->id), NetS(h->frag), h->ttl, h->proto,
		NetS(h->length),
		(h->vihl & 0xf) << 2);

	m->ps += hl;
	p = (uint8_t *)(h + 1);
	if(p < m->ps){
		m->p = seprint(m->p, m->e, " opts=(");
		while(p < m->ps)
			m->p = seprint(m->p, m->e, "%.2x", *p++);
		m->p = seprint(m->p, m->e, ")");
	}

	return 0;
}

Proto ip =
{
	"ip",
	p_compile,
	p_filter,
	p_seprint,
	p_mux,
	"%lu",
	p_fields,
	defaultframer,
};
