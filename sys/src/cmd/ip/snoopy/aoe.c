#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dat.h"
#include "protos.h"

typedef struct{
	uchar	verflags;
	uchar	error;
	uchar	major[2];
	uchar	minor;
	uchar	cmd;
	uchar	tag[4];
}Hdr;

enum{
	Hsize	= 10,
};

enum{
	Omajor,
	Ominor,
	Ocmd,
};

static Mux p_mux[] = {
	{"aoeata",	0},
	{"aoecmd",	1},
	{"aoemask",	2},
	{"aoerr",	3},
	{0},
};

static Field p_fields[] =
{
	{"shelf",	Fnum,	Ominor,		"shelf", },
	{"slot",	Fnum,	Omajor,		"slot",	},
	{"cmd",		Fnum,	Ocmd,		"cmd",	},
	{0}
};

static void
p_compile(Filter *f)
{
	Mux *m;

	if(f->op == '='){
		compile_cmp(aoe.name, f, p_fields);
		return;
	}
	for(m = p_mux; m->name; m++)
		if(strcmp(f->s, m->name) == 0){
			f->pr = m->pr;
			f->ulv = m->val;
			f->subop = Ocmd;
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
	case Omajor:
		return NetS(h->major) == f->ulv;
	case Ominor:
		return h->minor == f->ulv;
	case Ocmd:
		return h->cmd == f->ulv;
	}
	return 0;
}

static int
p_seprint(Msg *m)
{
	Hdr *h;

	if(m->pe - m->ps < Hsize)
		return 0;

	h = (Hdr*)m->ps;
	m->ps += Hsize;

	demux(p_mux, h->cmd, h->cmd, m, &dump);

	m->p = seprint(m->p, m->e, "ver=%d flag=%4b err=%d %d.%d cmd=%ux tag=%ux",
		h->verflags >> 4, h->verflags & 0xf, h->error, NetS(h->major),
		h->minor, h->cmd, NetL(h->tag));
	return 0;
}

Proto aoe =
{
	"aoe",
	p_compile,
	p_filter,
	p_seprint,
	p_mux,
	"%lud",
	p_fields,
	defaultframer,
};
