#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dat.h"
#include "protos.h"

typedef struct {
	uchar	res;
	uchar	cmd;
	uchar	ea[6];
} Hdr;

enum {
	Ocmd,
	Oea,

	Hsize	= 8,
};

static Field p_fields[] = {
	{"cmd",	Fnum,	Ocmd,	"command",	},
	{"ea",	Fnum,	Oea,	"ethernet addr", },
	nil
};

static void
p_compile(Filter *f)
{
	if(f->op == '='){
		compile_cmp(aoemd.name, f, p_fields);
		return;
	}
	sysfatal("unknown aoemd field: %s", f->s);
}

static int
p_filter(Filter *f, Msg *m)
{
	uchar buf[6];
	int i;
	Hdr *h;

	if(m->pe - m->ps < Hsize)
		return 0;

	h = (Hdr*)m->ps;
	m->ps += Hsize;

	switch(f->subop){
	case Ocmd:
		return h->cmd == f->ulv;
	case Oea:
		for(i = 0; i < 6; i++)
			buf[i] = f->ulv >> ((5 - i)*8);
		return memcmp(buf, h->ea, 6) == 0;
	}
	return 0;
}

static char *ctab[] = {
	"  ",
	" +",
	" -",
};

static int
p_seprint(Msg *m)
{
	char *s;
	Hdr *h;

	if(m->pe - m->ps < Hsize)
		return 0;

	h = (Hdr*)m->ps;
	m->ps += Hsize;

	/* no next protocol */
	m->pr = nil;

	s = "unk";
	if(h->cmd < nelem(ctab))
		s = ctab[h->cmd];
	m->p = seprint(m->p, m->e, "cmd=%d%s ea=%E\n", h->cmd, s, h->ea);
	return 0;
}

Proto aoemd = {
	"aoemd",
	p_compile,
	p_filter,
	p_seprint,
	nil,
	nil,
	p_fields,
	defaultframer,
};
