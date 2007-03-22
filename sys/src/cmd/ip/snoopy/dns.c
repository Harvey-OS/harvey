#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dat.h"
#include "protos.h"
#include "../../ndb/dns.h"

/* names of RR types - /sys/src/cmd/ndb/dn.c:/rrtname */
char *rrtname[] =
{
[Ta]		"ip",
[Tns]		"ns",
[Tmd]		"md",
[Tmf]		"mf",
[Tcname]	"cname",
[Tsoa]		"soa",
[Tmb]		"mb",
[Tmg]		"mg",
[Tmr]		"mr",
[Tnull]		"null",
[Twks]		"wks",
[Tptr]		"ptr",
[Thinfo]	"hinfo",
[Tminfo]	"minfo",
[Tmx]		"mx",
[Ttxt]		"txt",
[Trp]		"rp",
[Tafsdb]	"afsdb",
[Tx25]		"x.25",
[Tisdn]		"isdn",
[Trt]		"rt",
[Tnsap]		"nsap",
[Tnsapptr]	"nsap-ptr",
[Tsig]		"sig",
[Tkey]		"key",
[Tpx]		"px",
[Tgpos]		"gpos",
[Taaaa]		"ipv6",
[Tloc]		"loc",
[Tnxt]		"nxt",
[Teid]		"eid",
[Tnimloc]	"nimrod",
[Tsrv]		"srv",
[Tatma]		"atma",
[Tnaptr]	"naptr",
[Tkx]		"kx",
[Tcert]		"cert",
[Ta6]		"a6",
[Tdname]	"dname",
[Tsink]		"sink",
[Topt]		"opt",
[Tapl]		"apl",
[Tds]		"ds",
[Tsshfp]	"sshfp",
[Tipseckey]	"ipseckey",
[Trrsig]	"rrsig",
[Tnsec]		"nsec",
[Tdnskey]	"dnskey",
[Tspf]		"spf",
[Tuinfo]	"uinfo",
[Tuid]		"uid",
[Tgid]		"gid",
[Tunspec]	"unspec",
[Ttkey]		"tkey",
[Ttsig]		"tsig",
[Tixfr]		"ixfr",
[Taxfr]		"axfr",
[Tmailb]	"mailb",
[Tmaila]	"maila",
[Tall]		"all",
		0,
};
static char*
rrtypestr(int t)
{
	char buf[20];

	if(t >= 0 && t < nelem(rrtname) && rrtname[t])
		return rrtname[t];
	snprint(buf, sizeof buf, "type%d", t);
	return buf;
}

static void
fmtrr(Msg *m, RR **rrp, int quest)
{
	Txt *t;
	RR *rr;

	rr = *rrp;
	if(rr == nil)
		return;
	*rrp = rr->next;

	m->p = seprint(m->p, m->e, "%s name=%s ttl=%lud",
		rrtypestr(rr->type),
		rr->owner->name, rr->ttl);
	if(!quest)
	switch(rr->type){
	default:
		break;
	case Thinfo:
		m->p = seprint(m->p, m->e, " cpu=%s os=%s", rr->cpu->name, rr->os->name);
		break;
	case Tcname:
	case Tmb:
	case Tmd:
	case Tmf:
	case Tns:
		m->p = seprint(m->p, m->e, " host=%s", rr->host->name);
		break;
	case Tmg:
	case Tmr:
		m->p = seprint(m->p, m->e, " mb=%s", rr->mb->name);
		break;
	case Tminfo:
		m->p = seprint(m->p, m->e, " rmb=%s", rr->rmb->name);
		m->p = seprint(m->p, m->e, " mb=%s", rr->mb->name);
		break;
	case Tmx:
		m->p = seprint(m->p, m->e, " pref=%lud", rr->pref);
		m->p = seprint(m->p, m->e, " host=%s", rr->host->name);
		break;
	case Ta:
	case Taaaa:
		m->p = seprint(m->p, m->e, " ip=%s", rr->ip->name);
		break;
	case Tptr:
		m->p = seprint(m->p, m->e, " ptr=%s", rr->ptr->name);
		break;
	case Tsoa:
		m->p = seprint(m->p, m->e, " host=%s", rr->host->name);
		m->p = seprint(m->p, m->e, " rmb=%s", rr->rmb->name);
		m->p = seprint(m->p, m->e, " soa.serial=%lud", rr->soa->serial);
		m->p = seprint(m->p, m->e, " soa.refresh=%lud", rr->soa->refresh);
		m->p = seprint(m->p, m->e, " soa.retry=%lud", rr->soa->retry);
		m->p = seprint(m->p, m->e, " soa.expire=%lud", rr->soa->expire);
		m->p = seprint(m->p, m->e, " soa.minttl=%lud", rr->soa->minttl);
		break;
	case Ttxt:
		for(t=rr->txt; t; t=t->next)
			m->p = seprint(m->p, m->e, " txt=%q", t->p);
		break;
	case Tnull:
		m->p = seprint(m->p, m->e, " null=%.*H", rr->null->dlen, rr->null->data);
		break;
	case Trp:
		m->p = seprint(m->p, m->e, " rmb=%s", rr->rmb->name);
		m->p = seprint(m->p, m->e, " rp=%s", rr->rp->name);
		break;
	case Tkey:
		m->p = seprint(m->p, m->e, " flags=%d proto=%d alg=%d data=%.*H", rr->key->flags, rr->key->proto, rr->key->alg, rr->key->dlen, rr->key->data);
		break;
	case Tsig:
		m->p = seprint(m->p, m->e, " type=%d alg=%d labels=%d ttl=%lud exp=%lud incep=%lud tag=%d signer=%s data=%.*H",
			rr->sig->type, rr->sig->alg, rr->sig->labels,
			rr->sig->ttl, rr->sig->exp, rr->sig->incep, rr->sig->tag,
			rr->sig->signer->name, rr->sig->dlen, rr->sig->data);
		break;
	case Tcert:
		m->p = seprint(m->p, m->e, " type=%d tag=%d alg=%d data=%.*H", rr->cert->type, rr->cert->tag, rr->cert->alg, rr->cert->dlen, rr->cert->data);
		break;
	}
	rrfree(rr);
}

void freealldn(void);
static Proto dnsqd, dnsan, dnsns, dnsar;

static void donext(Msg*);
static DNSmsg dm;

static int
p_seprint(Msg *m)
{
	char *e;

	if((e = convM2DNS(m->ps, m->pe-m->ps, &dm)) != nil){
		m->p = seprint(m->p, m->e, "error: %s", e);
		return 0;
	}
	m->p = seprint(m->p, m->e, "id=%d flags=%#ux", dm.id, dm.flags);
	donext(m);
	return 0;
}

static void
donext(Msg *m)
{
	if(dm.qd)
		m->pr = &dnsqd;
	else if(dm.an)
		m->pr = &dnsan;
	else if(dm.ns)
		m->pr = &dnsns;
	else if(dm.ar)
		m->pr = &dnsar;
	else{
		freealldn();
		memset(&dm, 0, sizeof dm);
		m->pr = nil;
	}
}

static int
p_seprintqd(Msg *m)
{
	fmtrr(m, &dm.qd, 1);
	donext(m);
	return 0;
}

static int
p_seprintan(Msg *m)
{
	fmtrr(m, &dm.an, 0);
	donext(m);
	return 0;
}

static int
p_seprintns(Msg *m)
{
	fmtrr(m, &dm.ns, 1);
	donext(m);
	return 0;
}

static int
p_seprintar(Msg *m)
{
	fmtrr(m, &dm.ar, 1);
	donext(m);
	return 0;
}

Proto dns =
{
	"dns",
	nil,
	nil,
	p_seprint,
	nil,
	nil,
	nil,
	defaultframer,
};

static Proto dnsqd =
{
	"dns.qd",
	nil,
	nil,
	p_seprintqd,
	nil,
	nil,
	nil,
	defaultframer,
};

static Proto dnsan =
{
	"dns.an",
	nil,
	nil,
	p_seprintan,
	nil,
	nil,
	nil,
	defaultframer,
};

static Proto dnsns =
{
	"dns.ns",
	nil,
	nil,
	p_seprintns,
	nil,
	nil,
	nil,
	defaultframer,
};

static Proto dnsar =
{
	"dns.ar",
	nil,
	nil,
	p_seprintar,
	nil,
	nil,
	nil,
	defaultframer,
};


void*
emalloc(int n)
{
	void *v;

	v = mallocz(n, 1);
	if(v == nil)
		sysfatal("out of memory");
	return v;
}

char*
estrdup(char *s)
{
	s = strdup(s);
	if(s == nil)
		sysfatal("out of memory");
	return s;
}

DN *alldn;

DN*
dnlookup(char *name, int class, int)
{
	DN *dn;

	dn = emalloc(sizeof *dn);
	dn->name = estrdup(name);
	dn->class = class;
	dn->magic = DNmagic;
	dn->next = alldn;
	alldn = dn;
	return dn;
}

void
freealldn(void)
{
	DN *dn;

	while(dn = alldn){
		alldn = dn->next;
		free(dn->name);
		free(dn);
	}
}


#define now 0

/*************************************************
 Everything below here is copied from /sys/src/cmd/ndb
 without modification and can be recopied to update.
 First parts from dn.c, then all of convM2DNS.c.
*/

/*
 *  free a list of resource records and any related structs
 */
void
rrfreelist(RR *rp)
{
	RR *next;

	for(; rp; rp = next){
		next = rp->next;
		rrfree(rp);
	}
}
void
freeserverlist(Server *s)
{
	Server *next;

	for(; s != nil; s = next){
		next = s->next;
		free(s);
	}
}

/*
 *  allocate a resource record of a given type
 */
RR*
rralloc(int type)
{
	RR *rp;

	rp = emalloc(sizeof(*rp));
	rp->magic = RRmagic;
	rp->pc = getcallerpc(&type);
	rp->type = type;
	switch(type){
	case Tsoa:
		rp->soa = emalloc(sizeof(*rp->soa));
		rp->soa->slaves = nil;
		break;
	case Tkey:
		rp->key = emalloc(sizeof(*rp->key));
		break;
	case Tcert:
		rp->cert = emalloc(sizeof(*rp->cert));
		break;
	case Tsig:
		rp->sig = emalloc(sizeof(*rp->sig));
		break;
	case Tnull:
		rp->null = emalloc(sizeof(*rp->null));
		break;
	}
	rp->ttl = 0;
	rp->expire = 0;
	rp->next = 0;
	return rp;
}

/*
 *  free a resource record and any related structs
 */
void
rrfree(RR *rp)
{
	DN *dp;
	RR *nrp;
	Txt *t;

	assert(rp->magic = RRmagic);
	assert(!rp->cached);

	dp = rp->owner;
	if(dp){
		assert(dp->magic == DNmagic);
		for(nrp = dp->rr; nrp; nrp = nrp->next)
			assert(nrp != rp); /* "rrfree of live rr" */;
	}

	switch(rp->type){
	case Tsoa:
		freeserverlist(rp->soa->slaves);
		free(rp->soa);
		break;
	case Tkey:
		free(rp->key->data);
		free(rp->key);
		break;
	case Tcert:
		free(rp->cert->data);
		free(rp->cert);
		break;
	case Tsig:
		free(rp->sig->data);
		free(rp->sig);
		break;
	case Tnull:
		free(rp->null->data);
		free(rp->null);
		break;
	case Ttxt:
		while(rp->txt != nil){
			t = rp->txt;
			rp->txt = t->next;
			free(t->p);
			free(t);
		}
		break;
	}

	rp->magic = ~rp->magic;
	free(rp);
}

typedef struct Scan	Scan;
struct Scan
{
	uchar	*base;
	uchar	*p;
	uchar	*ep;
	char	*err;
};

#define NAME(x)		gname(x, sp)
#define SYMBOL(x)	(x = gsym(sp))
#define STRING(x)	(x = gstr(sp))
#define USHORT(x)	(x = gshort(sp))
#define ULONG(x)	(x = glong(sp))
#define UCHAR(x)	(x = gchar(sp))
#define V4ADDR(x)	(x = gv4addr(sp))
#define V6ADDR(x)	(x = gv6addr(sp))
#define BYTES(x, y)	(y = gbytes(sp, &x, len - (sp->p - data)))

static char *toolong = "too long";

/*
 *  get a ushort/ulong
 */
static ushort
gchar(Scan *sp)
{
	ushort x;

	if(sp->err)
		return 0;
	if(sp->ep - sp->p < 1){
		sp->err = toolong;
		return 0;
	}
	x = sp->p[0];
	sp->p += 1;
	return x;
}
static ushort
gshort(Scan *sp)
{
	ushort x;

	if(sp->err)
		return 0;
	if(sp->ep - sp->p < 2){
		sp->err = toolong;
		return 0;
	}
	x = (sp->p[0]<<8) | sp->p[1];
	sp->p += 2;
	return x;
}
static ulong
glong(Scan *sp)
{
	ulong x;

	if(sp->err)
		return 0;
	if(sp->ep - sp->p < 4){
		sp->err = toolong;
		return 0;
	}
	x = (sp->p[0]<<24) | (sp->p[1]<<16) | (sp->p[2]<<8) | sp->p[3];
	sp->p += 4;
	return x;
}

/*
 *  get an ip address
 */
static DN*
gv4addr(Scan *sp)
{
	char addr[32];

	if(sp->err)
		return 0;
	if(sp->ep - sp->p < 4){
		sp->err = toolong;
		return 0;
	}
	snprint(addr, sizeof(addr), "%V", sp->p);
	sp->p += 4;

	return dnlookup(addr, Cin, 1);
}
static DN*
gv6addr(Scan *sp)
{
	char addr[64];

	if(sp->err)
		return 0;
	if(sp->ep - sp->p < IPaddrlen){
		sp->err = toolong;
		return 0;
	}
	snprint(addr, sizeof(addr), "%I", sp->p);
	sp->p += IPaddrlen;

	return dnlookup(addr, Cin, 1);
}

/*
 *  get a string.  make it an internal symbol.
 */
static DN*
gsym(Scan *sp)
{
	int n;
	char sym[Strlen+1];

	if(sp->err)
		return 0;
	n = *(sp->p++);
	if(sp->p+n > sp->ep){
		sp->err = toolong;
		return 0;
	}

	if(n > Strlen){
		sp->err = "illegal string";
		return 0;
	}
	strncpy(sym, (char*)sp->p, n);
	sym[n] = 0;
	sp->p += n;

	return dnlookup(sym, Csym, 1);
}

/*
 *  get a string.  don't make it an internal symbol.
 */
static Txt*
gstr(Scan *sp)
{
	int n;
	char sym[Strlen+1];
	Txt *t;

	if(sp->err)
		return 0;
	n = *(sp->p++);
	if(sp->p+n > sp->ep){
		sp->err = toolong;
		return 0;
	}

	if(n > Strlen){
		sp->err = "illegal string";
		return 0;
	}
	strncpy(sym, (char*)sp->p, n);
	sym[n] = 0;
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
gbytes(Scan *sp, uchar **p, int n)
{
	if(sp->err)
		return 0;
	if(sp->p+n > sp->ep || n < 0){
		sp->err = toolong;
		return 0;
	}
	*p = emalloc(n);
	memmove(*p, sp->p, n);
	sp->p += n;

	return n;
}

/*
 *  get a domain name.  'to' must point to a buffer at least Domlen+1 long.
 */
static char*
gname(char *to, Scan *sp)
{
	int len, off;
	int pointer;
	int n;
	char *tostart;
	char *toend;
	uchar *p;

	tostart = to;
	if(sp->err)
		goto err;
	pointer = 0;
	p = sp->p;
	toend = to + Domlen;
	for(len = 0; *p; len += pointer ? 0 : (n+1)){
		if((*p & 0xc0) == 0xc0){
			/* pointer to other spot in message */
			if(pointer++ > 10){
				sp->err = "pointer loop";
				goto err;
			}
			off = ((p[0]<<8) + p[1]) & 0x3ff;
			p = sp->base + off;
			if(p >= sp->ep){
				sp->err = "bad pointer";
				goto err;
			}
			n = 0;
			continue;
		}
		n = *p++;
		if(len + n < Domlen - 1){
			if(to + n > toend){
				sp->err = toolong;
				goto err;
			}
			memmove(to, p, n);
			to += n;
		}
		p += n;
		if(*p){
			if(to >= toend){
				sp->err = toolong;
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
 *  convert the next RR from a message
 */
static RR*
convM2RR(Scan *sp)
{
	RR *rp;
	int type;
	int class;
	uchar *data;
	int len;
	char dname[Domlen+1];
	Txt *t, **l;

retry:
	NAME(dname);
	USHORT(type);
	USHORT(class);

	rp = rralloc(type);
	rp->owner = dnlookup(dname, class, 1);
	rp->type = type;

	ULONG(rp->ttl);
	rp->ttl += now;
	USHORT(len);
	data = sp->p;

	if(sp->p + len > sp->ep)
		sp->err = toolong;
	if(sp->err){
		rrfree(rp);
		return 0;
	}

	switch(type){
	default:
		/* unknown type, just ignore it */
		sp->p = data + len;
		rrfree(rp);
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
		rp->mb = dnlookup(NAME(dname), Cin, 1);
		break;
	case Tminfo:
		rp->rmb = dnlookup(NAME(dname), Cin, 1);
		rp->mb = dnlookup(NAME(dname), Cin, 1);
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
		rp->rmb = dnlookup(NAME(dname), Cin, 1);
		ULONG(rp->soa->serial);
		ULONG(rp->soa->refresh);
		ULONG(rp->soa->retry);
		ULONG(rp->soa->expire);
		ULONG(rp->soa->minttl);
		break;
	case Ttxt:
		l = &rp->txt;
		*l = nil;
		while(sp->p-data < len){
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
		rp->rp = dnlookup(NAME(dname), Cin, 1);
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
	if(sp->p - data != len)
		sp->err = "bad RR len";
	return rp;
}

/*
 *  convert the next question from a message
 */
static RR*
convM2Q(Scan *sp)
{
	char dname[Domlen+1];
	int type;
	int class;
	RR *rp;

	NAME(dname);
	USHORT(type);
	USHORT(class);
	if(sp->err)
		return 0;

	rp = rralloc(type);
	rp->owner = dnlookup(dname, class, 1);

	return rp;
}

static RR*
rrloop(Scan *sp, int count, int quest)
{
	int i;
	static char errbuf[64];
	RR *first, *rp, **l;

	if(sp->err)
		return 0;
	l = &first;
	first = 0;
	for(i = 0; i < count; i++){
		rp = quest ? convM2Q(sp) : convM2RR(sp);
		if(rp == 0)
			break;
		if(sp->err){
			rrfree(rp);
			break;
		}
		*l = rp;
		l = &rp->next;
	}
	return first;
}

/*
 *  convert the next DNS from a message stream
 */
char*
convM2DNS(uchar *buf, int len, DNSmsg *m)
{
	Scan scan;
	Scan *sp;
	char *err;

	scan.base = buf;
	scan.p = buf;
	scan.ep = buf + len;
	scan.err = 0;
	sp = &scan;
	memset(m, 0, sizeof(DNSmsg));
	USHORT(m->id);
	USHORT(m->flags);
	USHORT(m->qdcount);
	USHORT(m->ancount);
	USHORT(m->nscount);
	USHORT(m->arcount);
	m->qd = rrloop(sp, m->qdcount, 1);
	m->an = rrloop(sp, m->ancount, 0);
	m->ns = rrloop(sp, m->nscount, 0);
	err = scan.err;				/* live with bad ar's */
	m->ar = rrloop(sp, m->arcount, 0);
	return err;
}
