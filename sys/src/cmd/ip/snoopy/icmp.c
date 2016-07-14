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
{	uint8_t	type;
	uint8_t	code;
	uint8_t	cksum[2];	/* Checksum */
	uint8_t	data[1];
};

enum
{
	ICMPLEN=	4,
};

enum
{
	Ot,	/* type */
	Op,	/* next protocol */
};

static Field p_fields[] = 
{
	{"t",		Fnum,	Ot,	"type",	} ,
	{0}
};

enum
{
	EchoRep=	0,
	Unreachable=	3,
	SrcQuench=	4,
	Redirect=	5,
	EchoReq=	8,
	TimeExceed=	11,
	ParamProb=	12,
	TSreq=		13,
	TSrep=		14,
	InfoReq=	15,
	InfoRep=	16,
};

static Mux p_mux[] =
{
	{"ip",	Unreachable, },
	{"ip",	SrcQuench, },
	{"ip",	Redirect, },
	{"ip",	TimeExceed, },
	{"ip",	ParamProb, },
	{0},
};

char *icmpmsg[256] =
{
[EchoRep]	"EchoRep",
[Unreachable]	"Unreachable",
[SrcQuench]	"SrcQuench",
[Redirect]	"Redirect",
[EchoReq]	"EchoReq",
[TimeExceed]	"TimeExceed",
[ParamProb]	"ParamProb",
[TSreq]		"TSreq",
[TSrep]		"TSrep",
[InfoReq]	"InfoReq",
[InfoRep]	"InfoRep",
};

static void
p_compile(Filter *f)
{
	if(f->op == '='){
		compile_cmp(icmp.name, f, p_fields);
		return;
	}
	if(strcmp(f->s, "ip") == 0){
		f->pr = p_mux->pr;
		f->subop = Op;
		return;
	}
	sysfatal("unknown icmp field or protocol: %s", f->s);
}

static int
p_filter(Filter *f, Msg *m)
{
	Hdr *h;

	if(m->pe - m->ps < ICMPLEN)
		return 0;

	h = (Hdr*)m->ps;
	m->ps += ICMPLEN;

	switch(f->subop){
	case Ot:
		if(h->type == f->ulv)
			return 1;
		break;
	case Op:
		switch(h->type){
		case Unreachable:
		case TimeExceed:
		case SrcQuench:
		case Redirect:
		case ParamProb:
			m->ps += 4;
			return 1;
		}
	}
	return 0;
}

static int
p_seprint(Msg *m)
{
	Hdr *h;
	char *tn;
	char *p = m->p;
	char *e = m->e;
	uint16_t cksum2, cksum;

	h = (Hdr*)m->ps;
	m->ps += ICMPLEN;
	m->pr = &dump;

	if(m->pe - m->ps < ICMPLEN)
		return -1;

	tn = icmpmsg[h->type];
	if(tn == nil)
		p = seprint(p, e, "t=%u c=%d ck=%4.4x", h->type,
			h->code, (uint16_t)NetS(h->cksum));
	else
		p = seprint(p, e, "t=%s c=%d ck=%4.4x", tn,
			h->code, (uint16_t)NetS(h->cksum));
	if(Cflag){
		cksum = NetS(h->cksum);
		h->cksum[0] = 0;
		h->cksum[1] = 0;
		cksum2 = ~ptclbsum((uint8_t*)h, m->pe - m->ps + ICMPLEN) & 0xffff;
		if(cksum != cksum2)
			p = seprint(p,e, " !ck=%4.4x", cksum2);
	}
	switch(h->type){
	case EchoRep:
	case EchoReq:
		m->ps += 4;
		p = seprint(p, e, " id=%x seq=%x",
			NetS(h->data), NetS(h->data+2));
		break;
	case TSreq:
	case TSrep:
		m->ps += 12;
		p = seprint(p, e, " orig=%u rcv=%x xmt=%x",
			NetL(h->data), NetL(h->data+4),
			NetL(h->data+8));
		m->pr = nil;
		break;
	case InfoReq:
	case InfoRep:
		break;
	case Unreachable:
	case TimeExceed:
	case SrcQuench:
		m->ps += 4;
		m->pr = &ip;
		break;
	case Redirect:
		m->ps += 4;
		m->pr = &ip;
		p = seprint(p, e, "gw=%V", h->data);
		break;
	case ParamProb:
		m->ps += 4;
		m->pr = &ip;
		p = seprint(p, e, "ptr=%2.2x", h->data[0]);
		break;
	}
	m->p = p;
	return 0;
}

Proto icmp =
{
	"icmp",
	p_compile,
	p_filter,
	p_seprint,
	p_mux,
	"%lu",
	p_fields,
	defaultframer,
};
