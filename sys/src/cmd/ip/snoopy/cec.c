#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dat.h"
#include "protos.h"

typedef struct{
	uchar	type;
	uchar	conn;
	uchar	seq;
	uchar	len;
}Hdr;

enum{
	Hsize	= 4,
};

enum{
	Otype,
	Oconn,
	Oseq,
	Olen,
};

static Field p_fields[] =
{
	{"type",	Fnum,	Otype,		"type",	},
	{"conn",	Fnum,	Oconn,		"conn",	},
	{"seq",		Fnum,	Oseq,		"seq",	},
	{"len",		Fnum,	Olen,		"len",	},
	{0}
};

static void
p_compile(Filter *f)
{
	if(f->op == '='){
		compile_cmp(aoe.name, f, p_fields);
		return;
	}
	sysfatal("unknown aoe field: %s", f->s);
}

static int
p_filter(Filter *f, Msg *m)
{
	Hdr *h;

	if(m->pe - m->ps < Hsize)
		return 0;

	h = (Hdr*)m->ps;
	m->ps += Hsize;

	switch(f->subop){
	case Otype:
		return h->type == f->ulv;
	case Oconn:
		return h->conn = f->ulv;
	case Oseq:
		return h->seq = f->ulv;
	case Olen:
		return h->len = f->ulv;
	}
	return 0;
}

static char* ttab[] = {
	"Tinita",
	"Tinitb",
	"Tinitc",
	"Tdata",
	"Tack",
	"Tdiscover",
	"Toffer",
	"Treset",
};

static int
p_seprint(Msg *m)
{
	char *s, *p, buf[4];
	Hdr *h;

	if(m->pe - m->ps < Hsize)
		return 0;

	h = (Hdr*)m->ps;
	m->ps += Hsize;

	m->pr = nil;

	if(h->type < nelem(ttab))
		s = ttab[h->type];
	else{
		snprint(buf, sizeof buf, "%d", h->type);
		s = buf;
	}

	p = (char*)m->ps;
	m->p = seprint(m->p, m->e, "type=%s conn=%d seq=%d len=%d %.*s",
		s, h->conn, h->seq, h->len,
		(int)utfnlen(p, h->len), p);
	return 0;
}

Proto cec =
{
	"cec",
	p_compile,
	p_filter,
	p_seprint,
	nil,
	nil,
	p_fields,
	defaultframer,
};
