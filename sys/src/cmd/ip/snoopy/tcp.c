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
	uint8_t	sport[2];
	uint8_t	dport[2];
	uint8_t	seq[4];
	uint8_t	ack[4];
	uint8_t	flag[2];
	uint8_t	win[2];
	uint8_t	cksum[2];
	uint8_t	urg[2];
	uint8_t	opt[1];
};

typedef struct PseudoHdr{
	uint8_t	src[4];
	uint8_t	dst[4];
	uint8_t	zero;
	uint8_t	proto;
	uint8_t	length[2];
	uint8_t	hdrdata[1580];
} PseudoHdr;

enum
{
	TCPLEN= 20,
};

enum
{
	Os,
	Od,
	Osd,
};

static Field p_fields[] =
{
	{"s",		Fnum,	Os,	"source port",	} ,
	{"d",		Fnum,	Od,	"dest port",	} ,
	{"a",		Fnum,	Osd,	"source/dest port",	} ,
	{"sd",		Fnum,	Osd,	"source/dest port",	} ,
	{0}
};

static Mux p_mux[] =
{
	{"dns",		53, },
	{"ninep",	17007, },	/* exportfs */
	{"ninep",	564, },		/* 9fs */
	{"ninep",	17005, },	/* ocpu */
	{"ninep",	17010, },	/* ncpu */
	{"ninep",	17013, },	/* cpu */
	{0},
};

enum
{
	EOLOPT		= 0,
	NOOPOPT		= 1,
	MSSOPT		= 2,
	MSS_LENGTH	= 4,		/* Mean segment size */
	WSOPT		= 3,
	WS_LENGTH	= 3,		/* Bits to scale window size by */
};

static void
p_compile(Filter *f)
{
	Mux *m;

	if(f->op == '='){
		compile_cmp(tcp.name, f, p_fields);
		return;
	}
	for(m = p_mux; m->name != nil; m++)
		if(strcmp(f->s, m->name) == 0){
			f->pr = m->pr;
			f->ulv = m->val;
			f->subop = Osd;
			return;
		}
	sysfatal("unknown tcp field or protocol: %s", f->s);
}

static int
p_filter(Filter *f, Msg *m)
{
	Hdr *h;

	if(m->pe - m->ps < TCPLEN)
		return 0;

	h = (Hdr*)m->ps;
	m->ps += (NetS(h->flag)>>10) & 0x3f;

	switch(f->subop){
	case Os:
		return NetS(h->sport) == f->ulv;
	case Od:
		return NetS(h->dport) == f->ulv;
	case Osd:
		return NetS(h->sport) == f->ulv || NetS(h->dport) == f->ulv;
	}
	return 0;
}

enum
{
	URG		= 0x20,		/* Data marked urgent */
	ACK		= 0x10,		/* Aknowledge is valid */
	PSH		= 0x08,		/* Whole data pipe is pushed */
	RST		= 0x04,		/* Reset connection */
	SYN		= 0x02,		/* Pkt. is synchronise */
	FIN		= 0x01,		/* Start close down */
};

static char*
flags(int f)
{
	static char fl[20];
	char *p;

	p = fl;
	if(f & URG)
		*p++ = 'U';
	if(f & ACK)
		*p++ = 'A';
	if(f & PSH)
		*p++ = 'P';
	if(f & RST)
		*p++ = 'R';
	if(f & SYN)
		*p++ = 'S';
	if(f & FIN)
		*p++ = 'F';
	*p = 0;
	return fl;
}


static int
p_seprint(Msg *m)
{
	int dport, sport, len, flag, optlen;
	uint8_t *optr;
	Hdr *h;

	if(m->pe - m->ps < TCPLEN)
		return -1;
	h = (Hdr*)m->ps;

	/* get tcp header length */
	flag = NetS(h->flag);
	len = (flag>>10) & ~3;
	flag &= 0x3ff;
	m->ps += len;

	/* next protocol */
	dport = NetS(h->dport);
	sport = NetS(h->sport);
	demux(p_mux, sport, dport, m, &dump);

	m->p = seprint(m->p, m->e, "s=%d d=%d seq=%lu ack=%lu fl=%s win=%d ck=%4.4ux",
			NetS(h->sport), dport,
			(uint32_t)NetL(h->seq), (uint32_t)NetL(h->ack),
			flags(flag), NetS(h->win),
			NetS(h->cksum));

	/* tcp options */
	len -= TCPLEN;
	optr = h->opt;
	while(len > 0) {
		if(*optr == EOLOPT){
			m->p = seprint(m->p, m->e, " opt=EOL");
			break;
		}
		if(*optr == NOOPOPT) {
			m->p = seprint(m->p, m->e, " opt=NOOP");
			len--;
			optr++;
			continue;
		}
		optlen = optr[1];
		if(optlen < 2 || optlen > len)
			break;
		switch(*optr) {
		case MSSOPT:
			m->p = seprint(m->p, m->e, " opt%d=(mss %u)",
				optlen, nhgets(optr+2));
			break;
		case WSOPT:
			m->p = seprint(m->p, m->e, " opt%d=(wscale %u)",
				optlen, *(optr+2));
			break;
		default:
			m->p = seprint(m->p, m->e, " opt%d=(%u %.*H)",
				optlen, *optr, optlen-2, optr+2);
		}
		len -= optlen;
		optr += optlen;
	}
	return 0;
}

Proto tcp =
{
	"tcp",
	p_compile,
	p_filter,
	p_seprint,
	p_mux,
	"%lu",
	p_fields,
	defaultframer,
};
