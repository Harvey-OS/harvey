#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <ip.h>
#include <ndb.h>
#include "dns.h"

enum {
	Maxrequest=		128,
};

Cfg cfg;

static char *servername;
static RR *serverrr;
static RR *serveraddrs;

char	*dbfile;
int	debug;
uchar	ipaddr[IPaddrlen];	/* my ip address */
char	*logfile = "dnsdebug";
int	maxage  = 60*60;
char	mntpt[Maxpath];
int	needrefresh;
ulong	now;
vlong	nowns;
int	testing;
char	*trace;
int	traceactivity;
char	*zonerefreshprogram;

void	docmd(int, char**);
void	doquery(char*, char*);
void	preloadserveraddrs(void);
int	prettyrrfmt(Fmt*);
int	setserver(char*);
void	squirrelserveraddrs(void);

void
usage(void)
{
	fprint(2, "%s: [-rx] [-f db-file] [[@server] domain [type]]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int n;
	Biobuf in;
	char *p;
	char *f[4];

	strcpy(mntpt, "/net");
	cfg.inside = 1;

	ARGBEGIN{
	case 'f':
		dbfile = EARGF(usage());
		break;
	case 'r':
		cfg.resolver = 1;
		break;
	case 'x':
		dbfile = "/lib/ndb/external";
		strcpy(mntpt, "/net.alt");
		break;
	default:
		usage();
	}ARGEND

	now = time(nil);
	nowns = nsec();
	dninit();
	fmtinstall('R', prettyrrfmt);
	if(myipaddr(ipaddr, mntpt) < 0)
		sysfatal("can't read my ip address");
	opendatabase();

	if(cfg.resolver)
		squirrelserveraddrs();

	debug = 1;

	if(argc > 0){
		docmd(argc, argv);
		exits(0);
	}

	Binit(&in, 0, OREAD);
	for(print("> "); p = Brdline(&in, '\n'); print("> ")){
		p[Blinelen(&in)-1] = 0;
		n = tokenize(p, f, 3);
		if(n>=1) {
			dnpurge();		/* flush the cache */
			docmd(n, f);
		}
	}
	exits(0);
}

static char*
longtime(long t)
{
	int d, h, m, n;
	static char x[128];

	for(d = 0; t >= 24*60*60; t -= 24*60*60)
		d++;
	for(h = 0; t >= 60*60; t -= 60*60)
		h++;
	for(m = 0; t >= 60; t -= 60)
		m++;
	n = 0;
	if(d)
		n += sprint(x, "%d day ", d);
	if(h)
		n += sprint(x+n, "%d hr ", h);
	if(m)
		n += sprint(x+n, "%d min ", m);
	if(t || n == 0)
		sprint(x+n, "%ld sec", t);
	return x;
}

int
prettyrrfmt(Fmt *f)
{
	RR *rp;
	char buf[3*Domlen];
	char *p, *e;
	Txt *t;

	rp = va_arg(f->args, RR*);
	if(rp == 0){
		strcpy(buf, "<null>");
		goto out;
	}

	p = buf;
	e = buf + sizeof(buf);
	p = seprint(p, e, "%-32.32s %-15.15s %-5.5s", rp->owner->name,
		longtime(rp->db? rp->ttl: (rp->ttl - now)),
		rrname(rp->type, buf, sizeof buf));

	if(rp->negative){
		seprint(p, e, "negative rcode %d", rp->negrcode);
		goto out;
	}

	switch(rp->type){
	case Thinfo:
		seprint(p, e, "\t%s %s", rp->cpu->name, rp->os->name);
		break;
	case Tcname:
	case Tmb:
	case Tmd:
	case Tmf:
	case Tns:
		seprint(p, e, "\t%s", (rp->host? rp->host->name: ""));
		break;
	case Tmg:
	case Tmr:
		seprint(p, e, "\t%s", (rp->mb? rp->mb->name: ""));
		break;
	case Tminfo:
		seprint(p, e, "\t%s %s", (rp->mb? rp->mb->name: ""),
			(rp->rmb? rp->rmb->name: ""));
		break;
	case Tmx:
		seprint(p, e, "\t%lud %s", rp->pref,
			(rp->host? rp->host->name: ""));
		break;
	case Ta:
	case Taaaa:
		seprint(p, e, "\t%s", (rp->ip? rp->ip->name: ""));
		break;
	case Tptr:
		seprint(p, e, "\t%s", (rp->ptr? rp->ptr->name: ""));
		break;
	case Tsoa:
		seprint(p, e, "\t%s %s %lud %lud %lud %lud %lud",
			rp->host->name, rp->rmb->name, rp->soa->serial,
			rp->soa->refresh, rp->soa->retry,
			rp->soa->expire, rp->soa->minttl);
		break;
	case Tsrv:
		seprint(p, e, "\t%ud %ud %ud %s",
			rp->srv->pri, rp->srv->weight, rp->port, rp->host->name);
		break;
	case Tnull:
		seprint(p, e, "\t%.*H", rp->null->dlen, rp->null->data);
		break;
	case Ttxt:
		p = seprint(p, e, "\t");
		for(t = rp->txt; t != nil; t = t->next)
			p = seprint(p, e, "%s", t->p);
		break;
	case Trp:
		seprint(p, e, "\t%s %s", rp->rmb->name, rp->rp->name);
		break;
	case Tkey:
		seprint(p, e, "\t%d %d %d", rp->key->flags, rp->key->proto,
			rp->key->alg);
		break;
	case Tsig:
		seprint(p, e, "\t%d %d %d %lud %lud %lud %d %s",
			rp->sig->type, rp->sig->alg, rp->sig->labels,
			rp->sig->ttl, rp->sig->exp, rp->sig->incep,
			rp->sig->tag, rp->sig->signer->name);
		break;
	case Tcert:
		seprint(p, e, "\t%d %d %d",
			rp->sig->type, rp->sig->tag, rp->sig->alg);
		break;
	}
out:
	return fmtstrcpy(f, buf);
}

void
logsection(char *flag, RR *rp)
{
	if(rp == nil)
		return;
	print("\t%s%R\n", flag, rp);
	for(rp = rp->next; rp != nil; rp = rp->next)
		print("\t      %R\n", rp);
}

void
logreply(int id, uchar *addr, DNSmsg *mp)
{
	RR *rp;
	char buf[12], resp[32];

	switch(mp->flags & Rmask){
	case Rok:
		strcpy(resp, "OK");
		break;
	case Rformat:
		strcpy(resp, "Format error");
		break;
	case Rserver:
		strcpy(resp, "Server failed");
		break;
	case Rname:
		strcpy(resp, "Nonexistent");
		break;
	case Runimplimented:
		strcpy(resp, "Unimplemented");
		break;
	case Rrefused:
		strcpy(resp, "Refused");
		break;
	default:
		sprint(resp, "%d", mp->flags & Rmask);
		break;
	}

	print("%d: rcvd %s from %I (%s%s%s%s%s)\n", id, resp, addr,
		mp->flags & Fauth? "authoritative": "",
		mp->flags & Ftrunc? " truncated": "",
		mp->flags & Frecurse? " recurse": "",
		mp->flags & Fcanrec? " can_recurse": "",
		(mp->flags & (Fauth|Rmask)) == (Fauth|Rname)? " nx": "");
	for(rp = mp->qd; rp != nil; rp = rp->next)
		print("\tQ:    %s %s\n", rp->owner->name,
			rrname(rp->type, buf, sizeof buf));
	logsection("Ans:  ", mp->an);
	logsection("Auth: ", mp->ns);
	logsection("Hint: ", mp->ar);
}

void
logsend(int id, int subid, uchar *addr, char *sname, char *rname, int type)
{
	char buf[12];

	print("%d.%d: sending to %I/%s %s %s\n", id, subid,
		addr, sname, rname, rrname(type, buf, sizeof buf));
}

RR*
getdnsservers(int class)
{
	RR *rr;

	if(servername == nil)
		return dnsservers(class);

	rr = rralloc(Tns);
	rr->owner = dnlookup("local#dns#servers", class, 1);
	rr->host = dnlookup(servername, class, 1);

	return rr;
}

void
squirrelserveraddrs(void)
{
	int v4;
	char *attr;
	RR *rr, *rp, **l;
	Request req;

	/* look up the resolver address first */
	cfg.resolver = 0;
	debug = 0;
	if(serveraddrs)
		rrfreelist(serveraddrs);
	serveraddrs = nil;
	rr = getdnsservers(Cin);
	l = &serveraddrs;
	for(rp = rr; rp != nil; rp = rp->next){
		attr = ipattr(rp->host->name);
		v4 = strcmp(attr, "ip") == 0;
		if(v4 || strcmp(attr, "ipv6") == 0){
			*l = rralloc(v4? Ta: Taaaa);
			(*l)->owner = rp->host;
			(*l)->ip = rp->host;
			l = &(*l)->next;
			continue;
		}
		req.isslave = 1;
		req.aborttime = NS2MS(nowns) + Maxreqtm;
		*l = dnresolve(rp->host->name, Cin, Ta, &req, 0, 0, Recurse, 0, 0);
		if(*l == nil)
			*l = dnresolve(rp->host->name, Cin, Taaaa, &req,
				0, 0, Recurse, 0, 0);
		while(*l != nil)
			l = &(*l)->next;
	}
	cfg.resolver = 1;
	debug = 1;
}

void
preloadserveraddrs(void)
{
	RR *rp, **l, *first;

	l = &first;
	for(rp = serveraddrs; rp != nil; rp = rp->next){
		lock(&dnlock);
		rrcopy(rp, l);
		unlock(&dnlock);
		rrattach(first, Authoritative);
	}
}

int
setserver(char *server)
{
	if(servername != nil){
		free(servername);
		servername = nil;
		cfg.resolver = 0;
	}
	if(server == nil || *server == 0)
		return 0;
	servername = strdup(server);
	squirrelserveraddrs();
	if(serveraddrs == nil){
		print("can't resolve %s\n", servername);
		cfg.resolver = 0;
	} else
		cfg.resolver = 1;
	return cfg.resolver? 0: -1;
}

void
doquery(char *name, char *tstr)
{
	int len, type, rooted;
	char *p, *np;
	char buf[1024];
	RR *rr, *rp;
	Request req;

	if(cfg.resolver)
		preloadserveraddrs();

	/* default to an "ip" request if alpha, "ptr" if numeric */
	if(tstr == nil || *tstr == 0)
		if(strcmp(ipattr(name), "ip") == 0)
			tstr = "ptr";
		else
			tstr = "ip";

	/* if name end in '.', remove it */
	len = strlen(name);
	if(len > 0 && name[len-1] == '.'){
		rooted = 1;
		name[len-1] = 0;
	} else
		rooted = 0;

	/* inverse queries may need to be permuted */
	strncpy(buf, name, sizeof buf);
	if(strcmp("ptr", tstr) == 0 && cistrstr(name, ".arpa") == nil){
		/* TODO: reversing v6 addrs is harder */
		for(p = name; *p; p++)
			;
		*p = '.';
		np = buf;
		len = 0;
		while(p >= name){
			len++;
			p--;
			if(*p == '.'){
				memmove(np, p+1, len);
				np += len;
				len = 0;
			}
		}
		memmove(np, p+1, len);
		np += len;
		strcpy(np, "in-addr.arpa");	/* TODO: ip6.arpa for v6 */
	}

	/* look it up */
	type = rrtype(tstr);
	if(type < 0){
		print("!unknown type %s\n", tstr);
		return;
	}

	memset(&req, 0, sizeof req);
	getactivity(&req, 0);
	req.isslave = 1;
	req.aborttime = NS2MS(nowns) + Maxreqtm;
	rr = dnresolve(buf, Cin, type, &req, 0, 0, Recurse, rooted, 0);
	if(rr){
		print("----------------------------\n");
		for(rp = rr; rp; rp = rp->next)
			print("answer %R\n", rp);
		print("----------------------------\n");
	}
	rrfreelist(rr);

	putactivity(0);
}

void
docmd(int n, char **f)
{
	int tmpsrv;
	char *name, *type;

	name = type = nil;
	tmpsrv = 0;

	if(*f[0] == '@') {
		if(setserver(f[0]+1) < 0)
			return;

		switch(n){
		case 3:
			type = f[2];
			/* fall through */
		case 2:
			name = f[1];
			tmpsrv = 1;
			break;
		}
	} else
		switch(n){
		case 2:
			type = f[1];
			/* fall through */
		case 1:
			name = f[0];
			break;
		}

	if(name == nil)
		return;

	doquery(name, type);

	if(tmpsrv)
		setserver("");
}
