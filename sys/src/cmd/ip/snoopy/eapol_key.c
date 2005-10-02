#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dat.h"
#include "protos.h"

typedef struct Hdr
{
	uchar	desc;
} Hdr;

typedef struct Rc4KeyDesc
{
	uchar	ln[2];
	uchar	replay[8];
	uchar	iv[16];
	uchar	idx;
	uchar	md[16];
} Rc4KeyDesc;

enum
{
	HDR=	1,		/* sizeof(Hdr) */
	RC4KEYDESC=	43,	/* sizeof(Rc4KeyDesc) */

	DescTpRC4= 1,
};

enum
{
	Odesc,
};

static Mux p_mux[] =
{
	{ "rc4keydesc", DescTpRC4, },
	{ 0 }
};

static Mux p_muxrc4[] =
{
	{ "dump", 0, },
	{ 0 }
};

static void
p_compile(Filter *f)
{
	Mux *m;

	for(m = p_mux; m->name != nil; m++)
		if(strcmp(f->s, m->name) == 0){
			f->pr = m->pr;
			f->ulv = m->val;
			f->subop = Odesc;
			return;
		}
	sysfatal("unknown eap_key field or type: %s", f->s);
}

static int
p_filter(Filter *f, Msg *m)
{
	Hdr *h;

	if(m->pe - m->ps < HDR)
		return 0;

	h = (Hdr*)m->ps;
	m->ps += HDR;

	switch(f->subop){
	case Odesc:
		return h->desc == f->ulv;
	}
	return 0;
}

static char*
op(int i)
{
	static char x[20];

	switch(i){
	case DescTpRC4:
		return "RC4KeyDesc";
	default:
		sprint(x, "%1d", i);
		return x;
	}
}

static int
p_seprint(Msg *m)
{
	Hdr *h;

	if(m->pe - m->ps < HDR)
		return -1;

	h = (Hdr*)m->ps;
	m->ps += HDR;

	/* next protocol  depending on type*/
	demux(p_mux, h->desc, h->desc, m, &dump);

	m->p = seprint(m->p, m->e, "desc=%s", op(h->desc));
	return 0;
}

static int
p_seprintrc4(Msg *m)
{
	Rc4KeyDesc *h;
	int len;

	if(m->pe - m->ps < RC4KEYDESC)
		return -1;

	h = (Rc4KeyDesc*)m->ps;
	m->ps += RC4KEYDESC;
	m->pr = nil;
	len = m->pe - m->ps;

	m->p = seprint(m->p, m->e, "keylen=%1d replay=%1d iv=%1d idx=%1d md=%1d",
			NetS(h->ln), NetS(h->replay), NetS(h->iv), h->idx, NetS(h->md));
	m->p = seprint(m->p, m->e, " dataln=%d", len);
	if (len > 0)
		m->p = seprint(m->p, m->e, " data=%.*H", len, m->ps);
	return 0;
}

Proto eapol_key =
{
	"eapol_key",
	p_compile,
	p_filter,
	p_seprint,
	p_mux,
	"%lud",
	nil,
	defaultframer,
};

Proto rc4keydesc =
{
	"rc4keydesc",
	p_compile,
	nil,
	p_seprintrc4,
	nil,
	nil,
	nil,
	defaultframer,
};
