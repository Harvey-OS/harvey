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
{
	uint8_t	flags;
	uint8_t	ln[4];	/* optional, present if L flag set*/
};

enum
{
	FLHDR=	1,	/* sizeof(flags) */
	LNHDR=	4,	/* sizeof(ln) */
};

enum
{
	FlagL = 1<<7, 
	FlagM = 1<<6,
	FlagS = 1<<5,
	Version = (1<<2)|(1<<1)|(1<<0),
};

static Mux p_mux[] =
{
	{ "dump", 0, },
	{ 0 }
};

static char*
flags(int f)
{
	static char fl[20];
	char *p;

	p = fl;
	if(f & FlagS)
		*p++ = 'S';
	if(f & FlagM)
		*p++ = 'M';
	if(f & FlagL)
		*p++ = 'L';
	*p = 0;
	return fl;
}

static int
p_seprint(Msg *m)
{
	Hdr *h;

	if(m->pe - m->ps < FLHDR)
		return -1;

	h = (Hdr*)m->ps;
	m->ps += FLHDR;

	if (h->flags & FlagL) {
		if(m->pe - m->ps < LNHDR)
			return -1;
		else
			m->ps += LNHDR;
	}

	/* next protocol  depending on type*/
	demux(p_mux, 0, 0, m, &dump);

	m->p = seprint(m->p, m->e, "ver=%1d", h->flags & Version);
	m->p = seprint(m->p, m->e, " fl=%s", flags(h->flags));

	if (h->flags & FlagL)
		m->p = seprint(m->p, m->e, " totallen=%1d", NetL(h->ln));

	/* these are not in the header, just print them for our convenience */
	m->p = seprint(m->p, m->e, " dataln=%1ld", m->pe - m->ps);
	if ((h->flags & (FlagL|FlagS|FlagM)) == 0 && m->ps == m->pe)
		m->p = seprint(m->p, m->e, " ack");

	return 0;
}

Proto ttls =
{
	"ttls",
	nil,
	nil,
	p_seprint,
	p_mux, /* we need this to get the dump printed */
	"%lu",
	nil,
	defaultframer,
};
