#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dns.h"

typedef struct Scan	Scan;
struct Scan
{
	uchar	*base;
	uchar	*p;
	uchar	*ep;

	char	*err;
	char	errbuf[256];	/* hold a formatted error sometimes */
	int	errflags;	/* outgoing reply flags */
};

#define NAME(x)		gname(x, rp, sp)
#define SYMBOL(x)	(x = gsym(rp, sp))
#define STRING(x)	(x = gstr(rp, sp))
#define USHORT(x)	(x = gshort(rp, sp))
#define ULONG(x)	(x = glong(rp, sp))
#define UCHAR(x)	(x = gchar(rp, sp))
#define V4ADDR(x)	(x = gv4addr(rp, sp))
#define V6ADDR(x)	(x = gv6addr(rp, sp))
#define BYTES(x, y)	(y = gbytes(rp, sp, &x, len - (sp->p - data)))

static int
errneg(RR *rp, Scan *sp, int actual)
{
	snprint(sp->errbuf, sizeof sp->errbuf, "negative len %d: %R",
		actual, rp);
	sp->err = sp->errbuf;
	return 0;
}

static int
errtoolong(RR *rp, Scan *sp, int actual, int nominal, char *where)
{
	char *p, *ep;
	char ptype[64];

	p =  sp->errbuf;
	ep = sp->errbuf + sizeof sp->errbuf - 1;
	if (where)
		p = seprint(p, ep, "%s: ", where);
	if (rp)
		p = seprint(p, ep, "type %s RR: ",
			rrname(rp->type, ptype, sizeof ptype));
	p = seprint(p, ep, "wrong length (actual %d, nominal %d)",
		actual, nominal);
	if (rp)
		seprint(p, ep, ": %R", rp);
	sp->err = sp->errbuf;
	return 0;
}

/*
 *  get a ushort/ulong
 */
static ushort
gchar(RR *rp, Scan *sp)
{
	ushort x;

	if(sp->err)
		return 0;
	if(sp->ep - sp->p < 1)
		return errtoolong(rp, sp, sp->ep - sp->p, 1, "gchar");
	x = sp->p[0];
	sp->p += 1;
	return x;
}
static ushort
gshort(RR *rp, Scan *sp)
{
	ushort x;

	if(sp->err)
		return 0;
	if(sp->ep - sp->p < 2)
		return errtoolong(rp, sp, sp->ep - sp->p, 2, "gshort");
	x = sp->p[0]<<8 | sp->p[1];
	sp->p += 2;
	return x;
}
static ulong
glong(RR *rp, Scan *sp)
{
	ulong x;

	if(sp->err)
		return 0;
	if(sp->ep - sp->p < 4)
		return errtoolong(rp, sp, sp->ep - sp->p, 4, "glong");
	x = sp->p[0]<<24 | sp->p[1]<<16 | sp->p[2]<<8 | sp->p[3];
	sp->p += 4;
	return x;
}

/*
 *  get an ip address
 */
static DN*
gv4addr(RR *rp, Scan *sp)
{
	char addr[32];

	if(sp->err)
		return 0;
	if(sp->ep - sp->p < 4)
		return (DN*)errtoolong(rp, sp, sp->ep - sp->p, 4, "gv4addr");
	snprint(addr, sizeof(addr), "%V", sp->p);
	sp->p += 4;

	return dnlookup(addr, Cin, 1);
}
static DN*
gv6addr(RR *rp, Scan *sp)
{
	char addr[64];

	if(sp->err)
		return 0;
	if(sp->ep - sp->p < IPaddrlen)
		return (DN*)errtoolong(rp, sp, sp->ep - sp->p, IPaddrlen,
			"gv6addr");
	snprint(addr, sizeof(addr), "%I", sp->p);
	sp->p += IPaddrlen;

	return dnlookup(addr, Cin, 1);
}

/*
 *  get a string.  make it an internal symbol.
 */
static DN*
gsym(RR *rp, Scan *sp)
{
	int n;
	char sym[Strlen+1];

	if(sp->err)
		return 0;
	n = 0;
	if (sp->p < sp->ep)
		n = *(sp->p++);
	if(sp->ep - sp->p < n)
		return (DN*)errtoolong(rp, sp, sp->ep - sp->p, n, "gsym");

	if(n > Strlen){
		sp->err = "illegal string (symbol)";
		return 0;
	}
	strncpy(sym, (char*)sp->p, n);
	sym[n] = 0;
	if (strlen(sym) != n)
		sp->err = "symbol shorter than declared length";
	sp->p += n;

	return dnlookup(sym, Csym, 1);
}

/*
 *  get a string.  don't make it an internal symbol.
 */
static Txt*
gstr(RR *rp, Scan *sp)
{
	int n;
	char sym[Strlen+1];
	Txt *t;

	if(sp->err)
		return 0;
	n = 0;
	if (sp->p < sp->ep)
		n = *(sp->p++);
	if(sp->ep - sp->p < n)
		return (Txt*)errtoolong(rp, sp, sp->ep - sp->p, n, "gstr");

	if(n > Strlen){
		sp->err = "illegal string";
		return 0;
	}
	strncpy(sym, (char*)sp->p, n);
	sym[n] = 0;
	if (strlen(sym) != n)
		sp->err = "string shorter than declared length";
	sp->p += n;

	t = emalloc(sizeof(*t));
	t->next = nil;
	t->p = estrdup(sym);
	return t;
}

/*
 *  get a sequence of bytes
 */
static int
gbytes(RR *rp, Scan *sp, uchar **p, int n)
{
	*p = nil;			/* i think this is a good idea */
	if(sp->err)
		return 0;
	if(n < 0)
		return errneg(rp, sp, n);
	if(sp->ep - sp->p < n)
		return errtoolong(rp, sp, sp->ep - sp->p, n, "gbytes");
	*p = emalloc(n);
	memmove(*p, sp->p, n);
	sp->p += n;

	return n;
}

/*
 *  get a domain name.  'to' must point to a buffer at least Domlen+1 long.
 */
static char*
gname(char *to, RR *rp, Scan *sp)
{
	int len, off, pointer, n;
	char *tostart, *toend;
	uchar *p;

	tostart = to;
	if(sp->err)
		goto err;
	pointer = 0;
	p = sp->p;
	toend = to + Domlen;
	for(len = 0; *p && p < sp->ep; len += pointer ? 0 : (n+1)){
		if((*p & 0xc0) == 0xc0){
			/* pointer to other spot in message */
			if(pointer++ > 10){
				sp->err = "pointer loop";
				goto err;
			}
			off = (p[0]<<8 | p[1]) & 0x3ff;
			p = sp->base + off;
			if(p >= sp->ep){
				sp->err = "bad pointer";
				goto err;
			}
			n = 0;
			continue;
		}
		n = 0;
		if (p < sp->ep)
			n = *p++;
		if(len + n < Domlen - 1){
			if(n > toend - to){
				errtoolong(rp, sp, toend - to, n, "gname 1");
				goto err;
			}
			memmove(to, p, n);
			to += n;
		}
		p += n;
		if(*p){
			if(to >= toend){
				errtoolong(rp, sp, to-tostart, toend-tostart,
					"gname 2");
				goto err;
			}
			*to++ = '.';
		}
	}
	*to = 0;
	if(pointer)
		sp->p += len + 2;	/* + 2 for pointer */
	else
		sp->p += len + 1;	/* + 1 for the null domain */
	return tostart;
err:
	*tostart = 0;
	return tostart;
}

/*
 * ms windows 2000 seems to get the bytes backward in the type field
 * of ptr records, so return a format error as feedback.
 */
static void
mstypehack(Scan *sp, int type, char *where)
{
	if ((uchar)type == 0 && (uchar)(type>>8) != 0) {
		syslog(0, "dns",
			"%s: byte-swapped type field in ptr rr from win2k",
			where);
		if (sp->errflags == 0)
			sp->errflags = Rformat;
	}
}

/*
 *  convert the next RR from a message
 */
static RR*
convM2RR(Scan *sp, char *what)
{
	RR *rp = nil;
	int type, class, len;
	uchar *data;
	char dname[Domlen+1];
	Txt *t, **l;

retry:
	NAME(dname);
	USHORT(type);
	USHORT(class);

	mstypehack(sp, type, "convM2RR");
	rp = rralloc(type);
	rp->owner = dnlookup(dname, class, 1);
	rp->type = type;

	ULONG(rp->ttl);
	rp->ttl += now;
	USHORT(len);
	data = sp->p;

	/*
	 * ms windows generates a lot of badly-formatted hints.
	 * hints are only advisory, so don't log complaints about them.
	 * it also generates answers in which p overshoots ep by exactly
	 * one byte; this seems to be harmless, so don't log them either.
	 */
	if (sp->ep - sp->p < len &&
	   !(strcmp(what, "hints") == 0 ||
	     sp->p == sp->ep + 1 && strcmp(what, "answers") == 0)) {
		syslog(0, "dns", "%s sp: base %#p p %#p ep %#p len %d", what,
			sp->base, sp->p, sp->ep, len);	/* DEBUG */
		errtoolong(rp, sp, sp->ep - sp->p, len, "convM2RR");
	}
	if(sp->err || sp->errflags){
		rrfree(rp);
		return 0;
	}

	switch(type){
	default:
		/* unknown type, just ignore it */
		sp->p = data + len;
		rrfree(rp);
		rp = nil;
		goto retry;
	case Thinfo:
		SYMBOL(rp->cpu);
		SYMBOL(rp->os);
		break;
	case Tcname:
	case Tmb:
	case Tmd:
	case Tmf:
	case Tns:
		rp->host = dnlookup(NAME(dname), Cin, 1);
		break;
	case Tmg:
	case Tmr:
		rp->mb  = dnlookup(NAME(dname), Cin, 1);
		break;
	case Tminfo:
		rp->rmb = dnlookup(NAME(dname), Cin, 1);
		rp->mb  = dnlookup(NAME(dname), Cin, 1);
		break;
	case Tmx:
		USHORT(rp->pref);
		rp->host = dnlookup(NAME(dname), Cin, 1);
		break;
	case Ta:
		V4ADDR(rp->ip);
		break;
	case Taaaa:
		V6ADDR(rp->ip);
		break;
	case Tptr:
		rp->ptr = dnlookup(NAME(dname), Cin, 1);
		break;
	case Tsoa:
		rp->host = dnlookup(NAME(dname), Cin, 1);
		rp->rmb  = dnlookup(NAME(dname), Cin, 1);
		ULONG(rp->soa->serial);
		ULONG(rp->soa->refresh);
		ULONG(rp->soa->retry);
		ULONG(rp->soa->expire);
		ULONG(rp->soa->minttl);
		break;
	case Ttxt:
		l = &rp->txt;
		*l = nil;
		while(sp->p - data < len){
			STRING(t);
			*l = t;
			l = &t->next;
		}
		break;
	case Tnull:
		BYTES(rp->null->data, rp->null->dlen);
		break;
	case Trp:
		rp->rmb = dnlookup(NAME(dname), Cin, 1);
		rp->rp  = dnlookup(NAME(dname), Cin, 1);
		break;
	case Tkey:
		USHORT(rp->key->flags);
		UCHAR(rp->key->proto);
		UCHAR(rp->key->alg);
		BYTES(rp->key->data, rp->key->dlen);
		break;
	case Tsig:
		USHORT(rp->sig->type);
		UCHAR(rp->sig->alg);
		UCHAR(rp->sig->labels);
		ULONG(rp->sig->ttl);
		ULONG(rp->sig->exp);
		ULONG(rp->sig->incep);
		USHORT(rp->sig->tag);
		rp->sig->signer = dnlookup(NAME(dname), Cin, 1);
		BYTES(rp->sig->data, rp->sig->dlen);
		break;
	case Tcert:
		USHORT(rp->cert->type);
		USHORT(rp->cert->tag);
		UCHAR(rp->cert->alg);
		BYTES(rp->cert->data, rp->cert->dlen);
		break;
	}
	if(sp->p - data != len) {
		char ptype[64];

		/*
		 * ms windows 2000 generates cname queries for reverse lookups
		 * with this particular error.  don't bother logging it.
		 *
		 * server: input error: bad cname RR len (actual 2 != len 0):
		 * 235.9.104.135.in-addr.arpa cname
		 *	235.9.104.135.in-addr.arpa from 135.104.9.235
		 */
		if (type == Tcname && sp->p - data == 2 && len == 0)
			return rp;

		snprint(sp->errbuf, sizeof sp->errbuf,
			"bad %s RR len (actual %lud != len %d): %R",
			rrname(type, ptype, sizeof ptype),
			sp->p - data, len, rp);
		sp->err = sp->errbuf;
	}
	return rp;
}

/*
 *  convert the next question from a message
 */
static RR*
convM2Q(Scan *sp)
{
	char dname[Domlen+1];
	int type, class;
	RR *rp = nil;

	NAME(dname);
	USHORT(type);
	USHORT(class);
	if(sp->err || sp->errflags)
		return 0;

	mstypehack(sp, type, "convM2Q");
	rp = rralloc(type);
	rp->owner = dnlookup(dname, class, 1);

	return rp;
}

static RR*
rrloop(Scan *sp, char *what, int count, int quest)
{
	int i;
	RR *first, *rp, **l;

	if(sp->err || sp->errflags)
		return 0;
	l = &first;
	first = 0;
	for(i = 0; i < count; i++){
		rp = quest ? convM2Q(sp) : convM2RR(sp, what);
		if(rp == nil)
			break;
		if(sp->err || sp->errflags){
			rrfree(rp);
			break;
		}
		*l = rp;
		l = &rp->next;
	}
	return first;
}

/*
 *  convert the next DNS from a message stream.
 *  if there are formatting errors or the like during parsing
 *  of the message, set *errp to the outgoing DNS flags (e.g., Rformat),
 *  which will abort processing and reply immediately with the outgoing flags.
 */
char*
convM2DNS(uchar *buf, int len, DNSmsg *m, int *errp)
{
	Scan scan;
	Scan *sp;
	char *err;
	RR *rp = nil;

	if (errp)
		*errp = 0;
	assert(len >= 0);
	scan.base = buf;
	scan.p = buf;
	scan.ep = buf + len;
	scan.err = nil;
	scan.errbuf[0] = '\0';
	scan.errflags = 0;
	sp = &scan;

	memset(m, 0, sizeof *m);
	USHORT(m->id);
	USHORT(m->flags);
	USHORT(m->qdcount);
	USHORT(m->ancount);
	USHORT(m->nscount);
	USHORT(m->arcount);

	m->qd = rrloop(sp, "questions",	m->qdcount, 1);
	m->an = rrloop(sp, "answers",	m->ancount, 0);
	m->ns = rrloop(sp, "nameservers",m->nscount, 0);
	err = scan.err;				/* live with bad ar's */
	m->ar = rrloop(sp, "hints",	m->arcount, 0);
	if (errp)
		*errp = scan.errflags;
	return err;
}
