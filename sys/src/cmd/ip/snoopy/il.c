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
	u8	sum[2];		/* Checksum including header */
	u8	len[2];		/* Packet length */
	u8	type;		/* Packet type */
	u8	spec;		/* Special */
	u8	sport[2];	/* Src port */
	u8	dport[2];	/* Dst port */
	u8	id[4];		/* Sequence id */
	u8	ack[4];		/* Acked sequence */
};

enum
{
	ILLEN= 18,
};

enum
{
	Os,
	Od,
	Osd,
};

static Field p_fields[] =
{
	{"s",		Fnum,	Os,	"source port",	} ,
	{"d",		Fnum,	Od,	"dest port",	} ,
	{"a",		Fnum,	Osd,	"source/dest port",	} ,
	{"sd",		Fnum,	Osd,	"source/dest port",	} ,
	{0}
};

static Mux p_mux[] =
{
	{"ninep",	17007, },	/* exportfs */
	{"ninep",	17008, },	/* 9fs */
	{"ninep",	17005, },	/* ocpu */
	{"ninep",	17010, },	/* ncpu */
	{"ninep",	17013, },	/* cpu */
	{0},
};

static void
p_compile(Filter *f)
{
	Mux *m;

	if(f->op == '='){
		compile_cmp(il.name, f, p_fields);
		return;
	}
	for(m = p_mux; m->name != nil; m++)
		if(strcmp(f->s, m->name) == 0){
			f->pr = m->pr;
			f->ulv = m->val;
			f->subop = Osd;
			return;
		}
	sysfatal("unknown il field or protocol: %s", f->s);
}

static int
p_filter(Filter *f, Msg *m)
{
	Hdr *h;

	if(m->pe - m->ps < ILLEN)
		return 0;
	h = (Hdr*)m->ps;
	m->ps += ILLEN;

	switch(f->subop){
	case Os:
		return NetS(h->sport) == f->ulv;
	case Od:
		return NetS(h->dport) == f->ulv;
	case Osd:
		return NetS(h->sport) == f->ulv || NetS(h->dport) == f->ulv;
	}
	return 0;
}

char *pktnames[] =
{
	"Sync",
	"Data",
	"Dataquery",
	"Ack",
	"Query",
	"State",
	"Close"
};

static char*
pkttype(int t)
{
	static char b[10];

	if(t > 6){
		sprint(b, "%d", t);
		return b;
	}
	return pktnames[t];
}

static int
p_seprint(Msg *m)
{
	Hdr *h;
	int dport, sport;

	if(m->pe - m->ps < ILLEN)
		return -1;
	h = (Hdr*)m->ps;
	m->ps += ILLEN;

	dport = NetS(h->dport);
	sport = NetS(h->sport);
	demux(p_mux, sport, dport, m, &dump);

	m->p = seprint(m->p, m->e, "s=%d d=%d t=%s id=%lu ack=%lu spec=%d ck=%4.4x ln=%d",
			sport, dport, pkttype(h->type),
			(u32)NetL(h->id), (u32)NetL(h->ack),
			h->spec,
			NetS(h->sum), NetS(h->len));
	return 0;
}

Proto il =
{
	"il",
	p_compile,
	p_filter,
	p_seprint,
	p_mux,
	"%lu",
	p_fields,
	defaultframer,
};
