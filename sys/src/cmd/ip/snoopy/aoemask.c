#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dat.h"
#include "protos.h"

typedef struct {
	uchar	res;
	uchar	cmd;
	uchar	err;
	uchar	cnt;
} Hdr;

enum {
	Ocmd,
	Oerr,
	Ocnt,

	Hsize	= 4,
};

static Field p_fields[] =
{
	{ "cmd",	Fnum,	Ocmd,	"command", },
	{ "err",	Fnum,	Oerr,	"error", },
	{ "cnt",	Fnum,	Ocnt,	"count", },
	nil
};

static Mux p_mux[] = {
	{ "aoemd",	0 },
	{ "aoemd",	1 },
	nil
};

static void
p_compile(Filter *f)
{
	Mux *m;

	if(f->op == '='){
		compile_cmp(aoerr.name, f, p_fields);
		return;
	}
	for(m = p_mux; m->name; m++)
		if(strcmp(f->s, m->name) == 0){
			f->pr = m->pr;
			f->ulv = m->val;
			f->subop = Ocmd;
			return;
		}
	sysfatal("unknown aoemask field: %s", f->s);
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
	case Ocmd:
		return h->cmd == f->ulv;
	case Oerr:
		return h->err == f->ulv;
	case Ocnt:
		return h->cnt == f->ulv;
	}
	return 0;
}

static char *ctab[] = {
	"read",
	"edit",
};

static char *etab[] = {
	"",
	"bad",
	"full",
};

static int
p_seprint(Msg *m)
{
	char *s, *t;
	Hdr *h;

	if(m->pe - m->ps < Hsize)
		return 0;

	h = (Hdr*)m->ps;
	m->ps += Hsize;

	demux(p_mux, h->cmd, h->cmd, m, &dump);

	s = "unk";
	if(h->cmd < nelem(ctab))
		s = ctab[h->cmd];
	t = "unk";
	if(h->err < nelem(etab))
		s = etab[h->err];
	m->p = seprint(m->p, m->e, "cmd=%d %s err=%d %s cnt=%d\n",
		h->cmd, s, h->err, t, h->cnt);
	return 0;
}

Proto aoemask = {
	"aoemask",
	p_compile,
	p_filter,
	p_seprint,
	p_mux,
	"%lud",
	p_fields,
	defaultframer,
};
