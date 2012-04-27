#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dat.h"
#include "protos.h"

/*
 GRE version 0 is specified in rfc1701.
 GRE version 0 has been respecified in rfc2784 as a subset of rfc1701.
 GRE version 1, as used by pptp, has been specified in rfc2637.
*/


/* GRE flag bits */
enum {
	GRE_chksum	= (1<<15),
	GRE_routing	= (1<<14),
	GRE_key		= (1<<13),
	GRE_seq		= (1<<12),
	GRE_srcrt		= (1<<11),
	GRE_recur	= (7<<8),
	GRE_ack		= (1<<7),
	GRE_version	= 0x7,
};


typedef struct Hdr	Hdr;
struct Hdr
{
	ushort flags;
	ushort proto;
	uchar version;
	ushort chksum;
	ushort offset;
	ulong key;
	ulong seq;
	ulong route;
	ulong ack;
};

enum
{
	Oproto,
};

static Field p_fields[] = 
{
	{"proto",		Fnum,	Oproto,	"encapsulated protocol",	} ,
	{0}
};

static Mux p_mux[] =
{
	{"pup",	0x0200, },
	{"xns",	0x0600, },
	{"ip",		0x0800, },
	{"chaos",	0x0804, },
	{"arp",	0x0806, },
	{"frarp",	0x0808, },
	{"vines",	0x0bad, },
	{"vinesecho",	0x0bae, },
	{"vinesloop",	0x0baf, },
	{"ppp",	0x880b, },
	{"llc",	0x007a, },
	{"dot1q",	0x8100, },
	{"eapol",	0x888e, },
	{0},
};

int
parthdrlen(ushort flags)
{
	return 4 + 
		(flags&GRE_chksum || flags&GRE_routing) ? 4 : 0 +
		flags&GRE_key ? 4 : 0 +
		flags&GRE_seq ? 4 : 0 +
		flags&GRE_ack ? 4 : 0;
}

int
parsehdr(Hdr *h, uchar *s, uchar *e)
{
	uchar *p;
	uchar n;

	if(e - s < 4)
		return -1;

	p = s;

	h->flags = NetS(p);
	p += 2;
	h->proto = NetS(p);
	p += 2;
	h->version = h->flags&GRE_version;

	if(parthdrlen(h->flags) > e - s)
		return -1;

	if(h->flags&(GRE_chksum|GRE_routing)){
		h->chksum = NetS(p);
		p += 2;
		h->offset = NetS(p);
		p += 2;
	}
	if(h->flags&GRE_key){
		h->key = NetL(p);
		p += 4;
	}
	if(h->flags&GRE_seq){
		h->seq = NetL(p);
		p += 4;
	}
	if(h->flags&GRE_ack){
		h->ack = NetL(p);
		p += 4;
	}
	if(h->flags&GRE_routing){
		for(;;){
			if(e - p < 4)
				return -1;
			if((n = p[3]) == 0)
				break;
			p += n;
		}
	}

	return p - s;
}

static void
p_compile(Filter *f)
{
	Mux *m;

	if(f->op == '='){
		compile_cmp(gre.name, f, p_fields);
		return;
	}
	for(m = p_mux; m->name != nil; m++)
		if(strcmp(f->s, m->name) == 0){
			f->pr = m->pr;
			f->ulv = m->val;
			f->subop = Oproto;
			return;
		}
	sysfatal("unknown gre field or protocol: %s", f->s);
}

static int
p_filter(Filter *f, Msg *m)
{
	Hdr h;
	int len;

	len = parsehdr(&h, m->ps, m->pe);
	if(len < 0)
		return -1;
	m->ps += len;

	switch(f->subop){
	case Oproto:
		return h.proto == f->ulv;
	}
	return 0;
}

static int
p_seprint(Msg *m)
{
	Hdr h;
	int len;

	len = parsehdr(&h, m->ps, m->pe);
	if(len < 0)
		return -1;
	m->ps += len;

	demux(p_mux, h.proto, h.proto, m, &dump);

	m->p = seprint(m->p, m->e, "version=%d proto=%#ux flags=%#.4ux", h.version, h.proto, h.flags);
	if(h.flags&GRE_chksum)
		m->p = seprint(m->p, m->e, " checksum=%#.4ux", h.chksum);
	if(h.flags&GRE_key)
		m->p = seprint(m->p, m->e, " key=%#.8ulx", h.key);
	if(h.flags&GRE_seq)
		m->p = seprint(m->p, m->e, " seq=%#.8ulx", h.seq);
	if(h.flags&GRE_ack)
		m->p = seprint(m->p, m->e, " ack=%#.8ulx", h.ack);
	if(h.flags&GRE_routing)
		m->p = seprint(m->p, m->e, " offset=%#ux haverouting", h.offset);
	if(h.version == 0)
		m->p = seprint(m->p, m->e, " recursion=%ud", (h.flags&GRE_recur)>>8);
	
	return 0;
}

Proto gre =
{
	"gre",
	p_compile,
	p_filter,
	p_seprint,
	p_mux,
	"%#.4ux",
	p_fields,
	defaultframer,
};
