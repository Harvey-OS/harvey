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

enum
{
	OfferTimeout=	60,		/* when an offer times out */
	MaxLease=	60*60,		/* longest lease for dynamic binding */
	MinLease=	15*60,		/* shortest lease for dynamic binding */
	StaticLease=	30*60,		/* lease for static binding */

	IPUDPHDRSIZE=	28,		/* size of an IP plus UDP header */
	MINSUPPORTED=	576,		/* biggest IP message the client must support */

	/* lengths of some bootp fields */
	Maxhwlen=	16,
	Maxfilelen=	128,
	Maxoptlen=	312-4,

	/* bootp types */
	Bootrequest=	1,
	Bootreply= 	2,

	/* bootp flags */
	Fbroadcast=	1<<15,
};

typedef struct Hdr	Hdr;
struct Hdr
{
	uint8_t	op;			/* opcode */
	uint8_t	htype;			/* hardware type */
	uint8_t	hlen;			/* hardware address len */
	uint8_t	hops;			/* hops */
	uint8_t	xid[4];			/* a random number */
	uint8_t	secs[2];		/* elapsed since client started booting */
	uint8_t	flags[2];
	uint8_t	ciaddr[IPv4addrlen];	/* client IP address (client tells server) */
	uint8_t	yiaddr[IPv4addrlen];	/* client IP address (server tells client) */
	uint8_t	siaddr[IPv4addrlen];	/* server IP address */
	uint8_t	giaddr[IPv4addrlen];	/* gateway IP address */
	uint8_t	chaddr[Maxhwlen];	/* client hardware address */
	char	sname[64];		/* server host name (optional) */
	char	file[Maxfilelen];	/* boot file name */
	uint8_t	optmagic[4];
	uint8_t	optdata[Maxoptlen];
};

enum
{
	Oca,
	Osa,
	Ot,
};

static Field p_fields[] =
{
	{"ca",		Fv4ip,	Oca,	"client IP addr",	} ,
	{"sa",		Fv4ip,	Osa,	"server IP addr",	} ,
	{0}
};

#define plan9opt ((uint32_t)(('p'<<24) | ('9'<<16) | (' '<<8) | ' '))
#define genericopt (0x63825363UL)

static Mux p_mux[] =
{
	{"dhcp", 	genericopt,},
	{"plan9bootp",	plan9opt,},
	{"dump",	0,},
	{0}
};

static void
p_compile(Filter *f)
{
	Mux *m;

	if(f->op == '='){
		compile_cmp(bootp.name, f, p_fields);
		return;
	}
	for(m = p_mux; m->name != nil; m++)
		if(strcmp(f->s, m->name) == 0){
			f->pr = m->pr;
			f->ulv = m->val;
			f->subop = Ot;
			return;
		}
	sysfatal("unknown bootp field: %s", f->s);
}

static int
p_filter(Filter *f, Msg *m)
{
	Hdr *h;

	h = (Hdr*)m->ps;

	if(m->pe < (uint8_t*)h->sname)
		return 0;
	m->ps = h->optdata;

	switch(f->subop){
	case Oca:
		return NetL(h->ciaddr) == f->ulv || NetL(h->yiaddr) == f->ulv;
	case Osa:
		return NetL(h->siaddr) == f->ulv;
	case Ot:
		return NetL(h->optmagic) == f->ulv;
	}
	return 0;
}

static char*
op(int i)
{
	static char x[20];

	switch(i){
	case Bootrequest:
		return "Req";
	case Bootreply:
		return "Rep";
	default:
		sprint(x, "%d", i);
		return x;
	}
}


static int
p_seprint(Msg *m)
{
	Hdr *h;
	uint32_t x;

	h = (Hdr*)m->ps;

	if(m->pe < (uint8_t*)h->sname)
		return -1;

	/* point past data */
	m->ps = h->optdata;

	/* next protocol */
	m->pr = nil;
	if(m->pe >= (uint8_t*)h->optdata){
		x = NetL(h->optmagic);
		demux(p_mux, x, x, m, &dump);
	}

	m->p = seprint(m->p, m->e, "t=%s ht=%d hl=%d hp=%d xid=%x sec=%d fl=%4.4x ca=%V ya=%V sa=%V ga=%V cha=%E magic=%lx",
		op(h->op), h->htype, h->hlen, h->hops,
		NetL(h->xid), NetS(h->secs), NetS(h->flags),
		h->ciaddr, h->yiaddr, h->siaddr, h->giaddr, h->chaddr,
		(uint32_t)NetL(h->optmagic));
	if(m->pe > (uint8_t*)h->sname && *h->sname)
		m->p = seprint(m->p, m->e, " snam=%s", h->sname);
	if(m->pe > (uint8_t*)h->file && *h->file)
		m->p = seprint(m->p, m->e, " file=%s", h->file);
	return 0;
}

Proto bootp =
{
	"bootp",
	p_compile,
	p_filter,
	p_seprint,
	p_mux,
	"%#.8lux",
	p_fields,
	defaultframer,
};
