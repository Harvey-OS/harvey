#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dat.h"
#include "protos.h"

typedef struct Hdr	Hdr;
struct Hdr
{
	uchar	code;
	uchar	id;
	uchar	len[2];	/* length including this header */

	uchar	tp;	/* optional, only for Request/Response */
};

enum
{
	EAPHDR=	4,	/* sizeof(code)+sizeof(id)+sizeof(len) */
	TPHDR= 1,	/* sizeof(tp) */

	/* eap types */
	Request = 1,
	Response,
	Success,
	Fail,

	/* eap request/response sub-types */
	Identity = 1,		/* Identity */
	Notify,		/* Notification */
	Nak,			/* Nak (Response only) */
	Md5,		/* MD5-challenge */
	Otp,			/* one time password */
	Gtc,			/* generic token card */
	Ttls = 21,		/* tunneled TLS */
	Xpnd = 254,	/* expanded types */
	Xprm,		/* experimental use */
};

enum
{
	Ot,
};

static Mux p_mux[] =
{
	{ "eap_identity", Identity, },
	{ "eap_notify", Notify, },
	{ "eap_nak", Nak, },
	{ "eap_md5", Md5, },
	{ "eap_otp", Otp, },
	{ "eap_gtc", Gtc, },
	{ "ttls", Ttls, },
	{ "eap_xpnd", Xpnd, },
	{ "eap_xprm", Xprm, }, 
	{ 0 }
};

static char *eapsubtype[256] =
{
[Identity]	"Identity",
[Notify]	"Notify",
[Nak]		"Nak",
[Md5]	"Md5",
[Otp]		"Otp",
[Gtc]		"Gtc",
[Ttls]		"Ttls",
[Xpnd]	"Xpnd",
[Xprm]	"Xprm",
};


static void
p_compile(Filter *f)
{
	Mux *m;

	for(m = p_mux; m->name != nil; m++)
		if(strcmp(f->s, m->name) == 0){
			f->pr = m->pr;
			f->ulv = m->val;
			f->subop = Ot;
			return;
		}
	sysfatal("unknown eap field or type: %s", f->s);
}

static int
p_filter(Filter *f, Msg *m)
{
	Hdr *h;
	int len;

	if(f->subop != Ot)
		return 0;

	if(m->pe - m->ps < EAPHDR)
		return -1;

	h = (Hdr*)m->ps;

	/* truncate the message if there's extra */
	/* len includes header */
	len = NetS(h->len);
	if(m->ps+len < m->pe)
		m->pe = m->ps+len;
	else if(m->ps+len > m->pe)
		return -1;
	m->ps += EAPHDR;

	if(h->code != Request && h->code != Response)
		return 0;
	m->ps += TPHDR;

	if(h->tp == f->ulv)
		return 1;

	return 0;
}

static char*
op(int i)
{
	static char x[20];

	switch(i){
	case Request:
		return "Request";
	case Response:
		return "Response";
	case Success:
		return "Success";
	case Fail:
		return "Fail";
	default:
		sprint(x, "%1d", i);
		return x;
	}
}

static char*
subop(uchar val)
{
	static char x[20], *p;

	p = eapsubtype[val];
	if(p != nil)
		return p;
	else {
		sprint(x, "%1d", val);
		return x;
	}
}

static int
p_seprint(Msg *m)
{
	Hdr *h;
	int len;
	char *p, *e;

	if(m->pe - m->ps < EAPHDR)
		return -1;

	p = m->p;
	e = m->e;
	h = (Hdr*)m->ps;

	/* resize packet (should already be done by eapol) */
	/* len includes header */
	len = NetS(h->len);
	if(m->ps+len < m->pe)
		m->pe = m->ps+len;
	else if(m->ps+len > m->pe)
		return -1;
	m->ps += EAPHDR;

	p = seprint(p, e, "id=%1d code=%s", h->id, op(h->code));
	switch(h->code) {
	case Request:
	case Response:
		m->ps += TPHDR;
		p = seprint(p, e, " type=%s", subop(h->tp));
		/* special case needed to print eap_notify notification as unicode */
		demux(p_mux, h->tp, h->tp, m, &dump);
		break;
	default:
		demux(p_mux, 0, 0, m, &dump);
		break;
	}
	m->p = seprint(p, e, " len=%1d", len);
	return 0;
}

static int
p_seprintidentity(Msg *m)
{
	char *ps, *pe, *z;
	int len;

	m->pr = nil;
	ps = (char*)m->ps;
	pe = (char*)m->pe;

	/* we would like to do this depending on the 'context':
	 *  - one for eap_identity request and
	 *  - one for eap_identity response
	 * but we've lost the context, or haven't we?
	 * so we treat them the same, so we might erroneously
	 * print a response as if it was a request. too bad. - axel
	 */
	for (z=ps; *z != '\0' && z+1 < pe; z++)
		;
	if (*z == '\0' && z+1 < pe) {
		m->p = seprint(m->p, m->e, "prompt=(%s)", ps);
		len = pe - (z+1);
		m->p = seprint(m->p, m->e, " options=(%.*s)", len, z+1);
	} else {
		len = pe - ps;
		m->p = seprint(m->p, m->e, "%.*s", len, ps);
	}
	return 0;
}

Proto eap =
{
	"eap",
	p_compile,
	p_filter,
	p_seprint,
	p_mux,
	"%lud",
	nil,
	defaultframer,
};

Proto eap_identity =
{
	"eap_identity",
	p_compile,
	p_filter,
	p_seprintidentity,
	nil,
	nil,
	nil,
	defaultframer,
};
