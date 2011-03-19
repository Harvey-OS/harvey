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
		m->p = seprint(m->p, m->e, " cpu=%s os=%s",
			rr->cpu->name, rr->os->name);
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
		m->p = seprint(m->p, m->e, " null=%.*H",
			rr->null->dlen, rr->null->data);
		break;
	case Trp:
		m->p = seprint(m->p, m->e, " rmb=%s", rr->rmb->name);
		m->p = seprint(m->p, m->e, " rp=%s", rr->rp->name);
		break;
	case Tkey:
		m->p = seprint(m->p, m->e, " flags=%d proto=%d alg=%d data=%.*H",
			rr->key->flags, rr->key->proto, rr->key->alg,
			rr->key->dlen, rr->key->data);
		break;
	case Tsig:
		m->p = seprint(m->p, m->e,
" type=%d alg=%d labels=%d ttl=%lud exp=%lud incep=%lud tag=%d signer=%s data=%.*H",
			rr->sig->type, rr->sig->alg, rr->sig->labels,
			rr->sig->ttl, rr->sig->exp, rr->sig->incep, rr->sig->tag,
			rr->sig->signer->name, rr->sig->dlen, rr->sig->data);
		break;
	case Tcert:
		m->p = seprint(m->p, m->e, " type=%d tag=%d alg=%d data=%.*H",
			rr->cert->type, rr->cert->tag, rr->cert->alg,
			rr->cert->dlen, rr->cert->data);
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

	if((e = convM2DNS(m->ps, m->pe-m->ps, &dm, nil)) != nil){
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

int debug;				/* for ndb/dns.h */
ulong now = 0;

void
dnslog(char *fmt, ...)			/* don't log */
{
	USED(fmt);
}

/*************************************************
 * Everything below here is copied from /sys/src/cmd/ndb/dn.c
 * without modification and can be recopied to update.
 */

/*
 *  convert an integer RR type to it's ascii name
 */
char*
rrname(int type, char *buf, int len)
{
	char *t;

	t = nil;
	if(type >= 0 && type <= Tall)
		t = rrtname[type];
	if(t==nil){
		snprint(buf, len, "%d", type);
		t = buf;
	}
	return t;
}

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
	setmalloctag(rp, rp->pc);
	switch(type){
	case Tsoa:
		rp->soa = emalloc(sizeof(*rp->soa));
		rp->soa->slaves = nil;
		setmalloctag(rp->soa, rp->pc);
		break;
	case Tsrv:
		rp->srv = emalloc(sizeof(*rp->srv));
		setmalloctag(rp->srv, rp->pc);
		break;
	case Tkey:
		rp->key = emalloc(sizeof(*rp->key));
		setmalloctag(rp->key, rp->pc);
		break;
	case Tcert:
		rp->cert = emalloc(sizeof(*rp->cert));
		setmalloctag(rp->cert, rp->pc);
		break;
	case Tsig:
		rp->sig = emalloc(sizeof(*rp->sig));
		setmalloctag(rp->sig, rp->pc);
		break;
	case Tnull:
		rp->null = emalloc(sizeof(*rp->null));
		setmalloctag(rp->null, rp->pc);
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
			assert(nrp != rp);	/* "rrfree of live rr" */
	}

	switch(rp->type){
	case Tsoa:
		freeserverlist(rp->soa->slaves);
		memset(rp->soa, 0, sizeof *rp->soa);	/* cause trouble */
		free(rp->soa);
		break;
	case Tsrv:
		memset(rp->srv, 0, sizeof *rp->srv);	/* cause trouble */
		free(rp->srv);
		break;
	case Tkey:
		free(rp->key->data);
		memset(rp->key, 0, sizeof *rp->key);	/* cause trouble */
		free(rp->key);
		break;
	case Tcert:
		free(rp->cert->data);
		memset(rp->cert, 0, sizeof *rp->cert);	/* cause trouble */
		free(rp->cert);
		break;
	case Tsig:
		free(rp->sig->data);
		memset(rp->sig, 0, sizeof *rp->sig);	/* cause trouble */
		free(rp->sig);
		break;
	case Tnull:
		free(rp->null->data);
		memset(rp->null, 0, sizeof *rp->null);	/* cause trouble */
		free(rp->null);
		break;
	case Ttxt:
		while(rp->txt != nil){
			t = rp->txt;
			rp->txt = t->next;
			free(t->p);
			memset(t, 0, sizeof *t);	/* cause trouble */
			free(t);
		}
		break;
	}

	rp->magic = ~rp->magic;
	memset(rp, 0, sizeof *rp);		/* cause trouble */
	free(rp);
}
