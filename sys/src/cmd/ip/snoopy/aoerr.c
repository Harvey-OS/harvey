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

typedef struct {
	u8	cmd;
	u8	nea;
} Hdr;

enum {
	Ocmd,
	Onea,
	Oea,

	Hsize	= 2,
};

static Field p_fields[] = {
	{"cmd",	Fnum,	Ocmd,	"command",	},
	{"nea",	Fnum,	Onea,	"ea count",	},
	{"ea",	Fnum,	Onea,	"ethernet addr", },
	nil
};

static void
p_compile(Filter *f)
{
	if(f->op == '='){
		compile_cmp(aoerr.name, f, p_fields);
		return;
	}
	sysfatal("unknown aoerr field: %s", f->s);
}

static int
p_filter(Filter *f, Msg *m)
{
	u8 buf[6];
	int i;
	Hdr *h;

	if(m->pe - m->ps < Hsize)
		return 0;

	h = (Hdr*)m->ps;
	m->ps += Hsize;

	switch(f->subop){
	case Ocmd:
		return h->cmd == f->ulv;
	case Onea:
		return h->nea == f->ulv;
	case Oea:
		if(m->pe - m->ps < 6*h->nea)
			return 0;
		for(i = 0; i < 6; i++)
			buf[i] = f->ulv >> ((5 - i)*8);
		for(i = 0; i < h->nea; i++)
			if(memcmp(m->ps + 6*i, buf, 6) == 0)
				return 1;
		return 0;
	}
	return 0;
}

static char *ctab[] = {
	"read",
	"write",
	"force",
};

static int
p_seprint(Msg *m)
{
	char *s;
	int i;
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
	m->p = seprint(m->p, m->e, "cmd=%d %s nea=%d", h->cmd, s, h->nea);
	for(i = 0;; i++){
		if(h->nea < i)
			break;
		if(i == 3){
			m->p = seprint(m->p, m->e, " ...");
			break;
		}
		if(m->pe - m->ps < 6*i){
			m->p = seprint(m->p, m->e, " *short*");
			break;
		}
		m->p = seprint(m->p, m->e, " %E", m->pe + 6*i);
	}
	m->p = seprint(m->p, m->e, "\n");
	return 0;
}

Proto aoerr = {
	"aoerr",
	p_compile,
	p_filter,
	p_seprint,
	nil,
	nil,
	p_fields,
	defaultframer,
};
