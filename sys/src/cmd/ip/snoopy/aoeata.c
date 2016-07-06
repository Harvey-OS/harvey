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

typedef struct{
	uint8_t	aflag;
	uint8_t	feat;
	uint8_t	sectors;
	uint8_t	cmd;
	uint8_t	lba[6];
}Hdr;

enum{
	Hsize	= 10,
};

enum{
	Oaflag,
	Ocmd,
	Ofeat,
	Osectors,
	Olba,

	Ostat,
	Oerr,
};

static Field p_fields[] =
{
	{"aflag",	Fnum,	Oaflag,		"aflag",		},
	{"cmd",		Fnum,	Ocmd,		"command register",	},
	{"feat",	Fnum,	Ofeat,		"features",		},
	{"sectors",	Fnum,	Osectors,	"number of sectors",	},
	{"lba",		Fnum,	Olba,		"lba",			},
	{"stat",	Fnum,	Ostat,		"status",		},
	{"err",		Fnum,	Oerr,		"error",		},
	{0}
};

static void
p_compile(Filter *f)
{
	if(f->op == '='){
		compile_cmp(aoeata.name, f, p_fields);
		return;
	}
	sysfatal("unknown aoeata field: %s", f->s);
}

uint64_t
llba(uint8_t *c)
{
	uint64_t l;

	l = c[0];
	l |= c[1]<<8;
	l |= c[2]<<16;
	l |= c[3]<<24;
	l |= (uint64_t)c[4]<<32;
	l |= (uint64_t)c[5]<<40;
	return l;
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
	case Oaflag:
		return h->aflag == f->ulv;
	case Ocmd:
		return h->cmd == f->ulv;
	case Ofeat:
		return h->feat == f->ulv;
	case Osectors:
		return h->sectors == f->ulv;
	case Olba:
		return llba(h->lba) == f->vlv;

	/* this is wrong, but we don't have access to the direction here */
	case Ostat:
		return h->cmd == f->ulv;
	case Oerr:
		return h->feat == f->ulv;
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

	/* no next protocol */
	m->pr = nil;

	m->p = seprint(m->p, m->e, "aflag=%x errfeat=%x sectors=%x cmdstat=%x lba=%lld",
		h->aflag, h->feat, h->sectors, h->cmd, llba(h->lba));
	return 0;
}

Proto aoeata =
{
	"aoeata",
	p_compile,
	p_filter,
	p_seprint,
	nil,
	nil,
	p_fields,
	defaultframer,
};
