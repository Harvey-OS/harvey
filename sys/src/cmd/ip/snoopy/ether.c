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
struct Hdr {
	uint8_t	d[6];
	uint8_t	s[6];
	uint8_t	type[2];
	char	data[1500];
};

#define	ETHERMINTU	60	/* minimum transmit size */
#define	ETHERMAXTU	1514	/* maximum transmit size */
#define ETHERHDRSIZE	14	/* size of an ethernet header */

static Mux p_mux[] =
{
	{"ip",		0x0800,	} ,
	{"arp",		0x0806,	} ,
	{"rarp",	0x0806,	} ,
	{"ip6", 	0x86dd, } ,
	{"pppoe_disc",	0x8863, },
	{"pppoe_sess",	0x8864, },
	{"eapol",	0x888e, },
	{"aoe",		0x88a2, } ,
	{"cec",		0xbcbc, } ,
	{0}
};

enum
{
	Os,	/* source */
	Od,	/* destination */
	Oa,	/* source or destination */
	Ot,	/* type */
};

static Field p_fields[] =
{
	{"s",	Fether,	Os,	"source address",	} ,
	{"d",	Fether,	Od,	"destination address",	} ,
	{"a",	Fether,	Oa,	"source|destination address" } ,
	{"sd",	Fether,	Oa,	"source|destination address" } ,
	{"t",	Fnum,	Ot,	"type" } ,
	{0}
};

static void
p_compile(Filter *f)
{
	Mux *m;

	if(f->op == '='){
		compile_cmp(ether.name, f, p_fields);
		return;
	}
	for(m = p_mux; m->name != nil; m++)
		if(strcmp(f->s, m->name) == 0){
			f->pr = m->pr;
			f->ulv = m->val;
			f->subop = Ot;
			return;
		}
	sysfatal("unknown ethernet field or protocol: %s", f->s);
}

static int
p_filter(Filter *f, Msg *m)
{
	Hdr *h;

	if(m->pe - m->ps < ETHERHDRSIZE)
		return 0;

	h = (Hdr*)m->ps;
	m->ps += ETHERHDRSIZE;

	switch(f->subop){
	case Os:
		return memcmp(h->s, f->a, 6) == 0;
	case Od:
		return memcmp(h->d, f->a, 6) == 0;
	case Oa:
		return memcmp(h->s, f->a, 6) == 0 || memcmp(h->d, f->a, 6) == 0;
	case Ot:
		return NetS(h->type) == f->ulv;
	}
	return 0;
}

static int
p_seprint(Msg *m)
{
	int len;
	unsigned int t;
	Hdr *h;

	len = m->pe - m->ps;
	if(len < ETHERHDRSIZE)
		return -1;

	h = (Hdr*)m->ps;
	m->ps += ETHERHDRSIZE;

	t = NetS(h->type);
	demux(p_mux, t, t, m, &dump);

	m->p = seprint(m->p, m->e, "s=%E d=%E pr=%4.4x ln=%d", h->s, h->d,
		t, len);
	return 0;
}

Proto ether =
{
	"ether",
	p_compile,
	p_filter,
	p_seprint,
	p_mux,
	"%#.4lux",
	p_fields,
	defaultframer
};
