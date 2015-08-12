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
	uint8_t res;
	uint8_t cmd;
	uint8_t ea[6];
} Hdr;

enum { Ocmd,
       Oea,

       Hsize = 8,
};

static Field p_fields[] = {{
                            "cmd", Fnum, Ocmd, "command",
                           },
                           {
                            "ea", Fnum, Oea, "ethernet addr",
                           },
                           nil};

static void
p_compile(Filter* f)
{
	if(f->op == '=') {
		compile_cmp(aoemd.name, f, p_fields);
		return;
	}
	sysfatal("unknown aoemd field: %s", f->s);
}

static int
p_filter(Filter* f, Msg* m)
{
	uint8_t buf[6];
	int i;
	Hdr* h;

	if(m->pe - m->ps < Hsize)
		return 0;

	h = (Hdr*)m->ps;
	m->ps += Hsize;

	switch(f->subop) {
	case Ocmd:
		return h->cmd == f->ulv;
	case Oea:
		for(i = 0; i < 6; i++)
			buf[i] = f->ulv >> ((5 - i) * 8);
		return memcmp(buf, h->ea, 6) == 0;
	}
	return 0;
}

static char* ctab[] = {
    "  ", " +", " -",
};

static int
p_seprint(Msg* m)
{
	char* s;
	Hdr* h;

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
    "aoemd", p_compile, p_filter, p_seprint, nil, nil, p_fields, defaultframer,
};
