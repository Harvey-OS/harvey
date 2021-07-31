#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <ip.h>
#include "dns.h"

enum {
	Nibwidth = 4,
	Nibmask = (1<<Nibwidth) - 1,
	V6maxrevdomdepth = 128 / Nibwidth,	/* bits / bits-per-nibble */

	/*
	 * ttl for generated ptr records.  it was zero, which might seem
	 * like a good idea, but some dns implementations seem to be
	 * confused by a zero ttl, and instead of using the data and then
	 * discarding the RR, they conclude that they don't have valid data.
	 */
	Ptrttl = 120,
};

static Ndb *db;
static Lock	dblock;

static RR*	addrrr(Ndbtuple*, Ndbtuple*);
static RR*	cnamerr(Ndbtuple*, Ndbtuple*);
static void	createptrs(void);
static RR*	dblookup1(char*, int, int, int);
static RR*	doaxfr(Ndb*, char*);
static Ndbtuple*look(Ndbtuple*, Ndbtuple*, char*);
static RR*	mxrr(Ndbtuple*, Ndbtuple*);
static RR*	nsrr(Ndbtuple*, Ndbtuple*);
static RR*	nullrr(Ndbtuple*, Ndbtuple*);
static RR*	ptrrr(Ndbtuple*, Ndbtuple*);
static RR*	soarr(Ndbtuple*, Ndbtuple*);
static RR*	srvrr(Ndbtuple*, Ndbtuple*);
static RR*	txtrr(Ndbtuple*, Ndbtuple*);

static int	implemented[Tall] =
{
	[Ta]		1,
	[Taaaa]		1,
	[Tcname]	1,
	[Tmx]		1,
	[Tns]		1,
	[Tnull]		1,
	[Tptr]		1,
	[Tsoa]		1,
	[Tsrv]		1,
	[Ttxt]		1,
};

/* straddle server configuration */
static Ndbtuple *indoms, *innmsrvs, *outnmsrvs;

static void
nstrcpy(char *to, char *from, int len)
{
	strncpy(to, from, len);
	to[len-1] = 0;
}

int
opendatabase(void)
{
	char netdbnm[256];
	Ndb *xdb, *netdb;

	if (db)
		return 0;

	xdb = ndbopen(dbfile);		/* /lib/ndb */

	snprint(netdbnm, sizeof netdbnm, "%s/ndb", mntpt);
	netdb = ndbopen(netdbnm);	/* /net/ndb */
	if(netdb)
		netdb->nohash = 1;

	db = ndbcat(netdb, xdb);	/* both */
	return db? 0: -1;
}

/*
 *  lookup an RR in the network database, look for matches
 *  against both the domain name and the wildcarded domain name.
 *
 *  the lock makes sure only one process can be accessing the data
 *  base at a time.  This is important since there's a lot of
 *  shared state there.
 *
 *  e.g. for x.research.bell-labs.com, first look for a match against
 *       the x.research.bell-labs.com.  If nothing matches,
 *	 try *.research.bell-labs.com.
 */
RR*
dblookup(char *name, int class, int type, int auth, int ttl)
{
	int err;
	char *wild, *cp;
	char buf[256];
	RR *rp, *tp;
	DN *dp, *ndp;
	static int parallel;
	static int parfd[2];
	static char token[1];

	/* so far only internet lookups are implemented */
	if(class != Cin)
		return 0;

	err = Rname;

	if(type == Tall){
		lock(&dnlock);
		rp = nil;
		for (type = Ta; type < Tall; type++)
			if(implemented[type])
				rrcat(&rp, dblookup(name, class, type, auth, ttl));
		unlock(&dnlock);
		return rp;
	}

	rp = nil;

	lock(&dblock);
	dp = dnlookup(name, class, 1);

	if(opendatabase() < 0)
		goto out;
	if(dp->rr)
		err = 0;

	/* first try the given name */
	if(cfg.cachedb)
		rp = rrlookup(dp, type, NOneg);
	else
		rp = dblookup1(name, type, auth, ttl);
	if(rp)
		goto out;

	/* try lower case version */
	for(cp = name; *cp; cp++)
		*cp = tolower(*cp);
	if(cfg.cachedb)
		rp = rrlookup(dp, type, NOneg);
	else
		rp = dblookup1(name, type, auth, ttl);
	if(rp)
		goto out;

	/* walk the domain name trying the wildcard '*' at each position */
	for(wild = strchr(name, '.'); wild; wild = strchr(wild+1, '.')){
		snprint(buf, sizeof buf, "*%s", wild);
		ndp = dnlookup(buf, class, 1);
		if(ndp->rr)
			err = 0;
		if(cfg.cachedb)
			rp = rrlookup(ndp, type, NOneg);
		else
			rp = dblookup1(buf, type, auth, ttl);
		if(rp)
			break;
	}
out:
	/* add owner to uncached records */
	if(rp)
		for(tp = rp; tp; tp = tp->next)
			tp->owner = dp;
	else {
		/*
		 * don't call it non-existent if it's not ours
		 * (unless we're a resolver).
		 */
		if(err == Rname && (!inmyarea(name) || cfg.resolver))
			err = Rserver;
		dp->respcode = err;
	}

	unlock(&dblock);
	return rp;
}

static ulong
intval(Ndbtuple *entry, Ndbtuple *pair, char *attr, ulong def)
{
	Ndbtuple *t = look(entry, pair, attr);

	return (t? strtoul(t->val, 0, 10): def);
}

/*
 *  lookup an RR in the network database
 */
static RR*
dblookup1(char *name, int type, int auth, int ttl)
{
	Ndbtuple *t, *nt;
	RR *rp, *list, **l;
	Ndbs s;
	char dname[Domlen];
	char *attr;
	DN *dp;
	RR *(*f)(Ndbtuple*, Ndbtuple*);
	int found, x;

	dp = nil;
	switch(type){
	case Tptr:
		attr = "ptr";
		f = ptrrr;
		break;
	case Ta:
		attr = "ip";
		f = addrrr;
		break;
	case Taaaa:
		attr = "ipv6";
		f = addrrr;
		break;
	case Tnull:
		attr = "nullrr";
		f = nullrr;
		break;
	case Tns:
		attr = "ns";
		f = nsrr;
		break;
	case Tsoa:
		attr = "soa";
		f = soarr;
		break;
	case Tsrv:
		attr = "srv";
		f = srvrr;
		break;
	case Tmx:
		attr = "mx";
		f = mxrr;
		break;
	case Tcname:
		attr = "cname";
		f = cnamerr;
		break;
	case Taxfr:
	case Tixfr:
		return doaxfr(db, name);
	default:
//		dnslog("dnlookup1(%s) bad type", name);
		return nil;
	}

	/*
	 *  find a matching entry in the database
	 */
	t = nil;
	free(ndbgetvalue(db, &s, "dom", name, attr, &t));

	/*
	 *  hack for local names
	 */
	if(t == nil && strchr(name, '.') == nil)
		free(ndbgetvalue(db, &s, "sys", name, attr, &t));
	if(t == nil) {
//		dnslog("dnlookup1(%s) name not found", name);
		return nil;
	}

	/* search whole entry for default domain name */
	strncpy(dname, name, sizeof dname);
	for(nt = t; nt; nt = nt->entry)
		if(strcmp(nt->attr, "dom") == 0){
			nstrcpy(dname, nt->val, sizeof dname);
			break;
		}

	/* ttl is maximum of soa minttl and entry's ttl ala rfc883 */
	x = intval(t, s.t, "ttl", 0);
	if(x > ttl)
		ttl = x;

	/* default ttl is one day */
	if(ttl < 0)
		ttl = DEFTTL;

	/*
	 *  The database has 2 levels of precedence; line and entry.
	 *  Pairs on the same line bind tighter than pairs in the
	 *  same entry, so we search the line first.
	 */
	found = 0;
	list = 0;
	l = &list;
	for(nt = s.t;; ){
		if(found == 0 && strcmp(nt->attr, "dom") == 0){
			nstrcpy(dname, nt->val, sizeof dname);
			found = 1;
		}
		if(cistrcmp(attr, nt->attr) == 0){
			rp = (*f)(t, nt);
			rp->auth = auth;
			rp->db = 1;
			if(ttl)
				rp->ttl = ttl;
			if(dp == nil)
				dp = dnlookup(dname, Cin, 1);
			rp->owner = dp;
			*l = rp;
			l = &rp->next;
			nt->ptr = 1;
		}
		nt = nt->line;
		if(nt == s.t)
			break;
	}

	/* search whole entry */
	for(nt = t; nt; nt = nt->entry)
		if(nt->ptr == 0 && cistrcmp(attr, nt->attr) == 0){
			rp = (*f)(t, nt);
			rp->db = 1;
			if(ttl)
				rp->ttl = ttl;
			rp->auth = auth;
			if(dp == nil)
				dp = dnlookup(dname, Cin, 1);
			rp->owner = dp;
			*l = rp;
			l = &rp->next;
		}
	ndbfree(t);

//	dnslog("dnlookup1(%s) -> %#p", name, list);
	return list;
}

/*
 *  make various types of resource records from a database entry
 */
static RR*
addrrr(Ndbtuple *entry, Ndbtuple *pair)
{
	RR *rp;
	uchar addr[IPaddrlen];

	USED(entry);
	parseip(addr, pair->val);
	if(isv4(addr))
		rp = rralloc(Ta);
	else
		rp = rralloc(Taaaa);
	rp->ip = dnlookup(pair->val, Cin, 1);
	return rp;
}
static RR*
nullrr(Ndbtuple *entry, Ndbtuple *pair)
{
	RR *rp;

	USED(entry);
	rp = rralloc(Tnull);
	rp->null->data = (uchar*)estrdup(pair->val);
	rp->null->dlen = strlen((char*)rp->null->data);
	return rp;
}
/*
 *  txt rr strings are at most 255 bytes long.  one
 *  can represent longer strings by multiple concatenated
 *  <= 255 byte ones.
 */
static RR*
txtrr(Ndbtuple *entry, Ndbtuple *pair)
{
	RR *rp;
	Txt *t, **l;
	int i, len, sofar;

	USED(entry);
	rp = rralloc(Ttxt);
	l = &rp->txt;
	rp->txt = nil;
	len = strlen(pair->val);
	sofar = 0;
	while(len > sofar){
		t = emalloc(sizeof(*t));
		t->next = nil;

		i = len-sofar;
		if(i > 255)
			i = 255;

		t->p = emalloc(i+1);
		memmove(t->p, pair->val+sofar, i);
		t->p[i] = 0;
		sofar += i;

		*l = t;
		l = &t->next;
	}
	return rp;
}
static RR*
cnamerr(Ndbtuple *entry, Ndbtuple *pair)
{
	RR *rp;

	USED(entry);
	rp = rralloc(Tcname);
	rp->host = dnlookup(pair->val, Cin, 1);
	return rp;
}
static RR*
mxrr(Ndbtuple *entry, Ndbtuple *pair)
{
	RR *rp;

	rp = rralloc(Tmx);
	rp->host = dnlookup(pair->val, Cin, 1);
	rp->pref = intval(entry, pair, "pref", 1);
	return rp;
}
static RR*
nsrr(Ndbtuple *entry, Ndbtuple *pair)
{
	RR *rp;
	Ndbtuple *t;

	rp = rralloc(Tns);
	rp->host = dnlookup(pair->val, Cin, 1);
	t = look(entry, pair, "soa");
	if(t && t->val[0] == 0)
		rp->local = 1;
	return rp;
}
static RR*
ptrrr(Ndbtuple *entry, Ndbtuple *pair)
{
	RR *rp;

	USED(entry);
	rp = rralloc(Tns);
	rp->ptr = dnlookup(pair->val, Cin, 1);
	return rp;
}
static RR*
soarr(Ndbtuple *entry, Ndbtuple *pair)
{
	RR *rp;
	Ndbtuple *ns, *mb, *t;
	char mailbox[Domlen];
	Ndb *ndb;
	char *p;

	rp = rralloc(Tsoa);
	rp->soa->serial = 1;
	for(ndb = db; ndb; ndb = ndb->next)
		if(ndb->mtime > rp->soa->serial)
			rp->soa->serial = ndb->mtime;

	rp->soa->retry  = intval(entry, pair, "retry", Hour);
	rp->soa->expire = intval(entry, pair, "expire", Day);
	rp->soa->minttl = intval(entry, pair, "ttl", Day);
	rp->soa->refresh = intval(entry, pair, "refresh", Day);
	rp->soa->serial = intval(entry, pair, "serial", rp->soa->serial);

	ns = look(entry, pair, "ns");
	if(ns == nil)
		ns = look(entry, pair, "dom");
	rp->host = dnlookup(ns->val, Cin, 1);

	/* accept all of:
	 *  mbox=person
	 *  mbox=person@machine.dom
	 *  mbox=person.machine.dom
	 */
	mb = look(entry, pair, "mbox");
	if(mb == nil)
		mb = look(entry, pair, "mb");
	if(mb)
		if(strchr(mb->val, '.')) {
			p = strchr(mb->val, '@');
			if(p != nil)
				*p = '.';
			rp->rmb = dnlookup(mb->val, Cin, 1);
		} else {
			snprint(mailbox, sizeof mailbox, "%s.%s",
				mb->val, ns->val);
			rp->rmb = dnlookup(mailbox, Cin, 1);
		}
	else {
		snprint(mailbox, sizeof mailbox, "postmaster.%s", ns->val);
		rp->rmb = dnlookup(mailbox, Cin, 1);
	}

	/*
	 *  hang dns slaves off of the soa.  this is
	 *  for managing the area.
	 */
	for(t = entry; t != nil; t = t->entry)
		if(strcmp(t->attr, "dnsslave") == 0)
			addserver(&rp->soa->slaves, t->val);

	return rp;
}

static RR*
srvrr(Ndbtuple *entry, Ndbtuple *pair)
{
	RR *rp;

	rp = rralloc(Tsrv);
	rp->host = dnlookup(pair->val, Cin, 1);
	rp->srv->pri = intval(entry, pair, "pri", 0);
	rp->srv->weight = intval(entry, pair, "weight", 0);
	/* TODO: translate service name to port # */
	rp->port = intval(entry, pair, "port", 0);
	return rp;
}

/*
 *  Look for a pair with the given attribute.  look first on the same line,
 *  then in the whole entry.
 */
static Ndbtuple*
look(Ndbtuple *entry, Ndbtuple *line, char *attr)
{
	Ndbtuple *nt;

	/* first look on same line (closer binding) */
	for(nt = line;;){
		if(cistrcmp(attr, nt->attr) == 0)
			return nt;
		nt = nt->line;
		if(nt == line)
			break;
	}
	/* search whole tuple */
	for(nt = entry; nt; nt = nt->entry)
		if(cistrcmp(attr, nt->attr) == 0)
			return nt;
	return 0;
}

static RR**
linkrr(RR *rp, DN *dp, RR **l)
{
	rp->owner = dp;
	rp->auth = 1;
	rp->db = 1;
	*l = rp;
	return &rp->next;
}

/* these are answered specially by the tcp version */
static RR*
doaxfr(Ndb *db, char *name)
{
	USED(db, name);
	return 0;
}


/*
 *  read the all the soa's from the database to determine area's.
 *  this is only used when we're not caching the database.
 */
static void
dbfile2area(Ndb *db)
{
	Ndbtuple *t;

	if(debug)
		dnslog("rereading %s", db->file);
	Bseek(&db->b, 0, 0);
	while(t = ndbparse(db))
		ndbfree(t);
}

/*
 *  read the database into the cache
 */
static void
dbpair2cache(DN *dp, Ndbtuple *entry, Ndbtuple *pair)
{
	RR *rp;
	static ulong ord;

	rp = 0;
	if(cistrcmp(pair->attr, "ip") == 0 ||
	   cistrcmp(pair->attr, "ipv6") == 0){
		dp->ordinal = ord++;
		rp = addrrr(entry, pair);
	} else 	if(cistrcmp(pair->attr, "ns") == 0)
		rp = nsrr(entry, pair);
	else if(cistrcmp(pair->attr, "soa") == 0) {
		rp = soarr(entry, pair);
		addarea(dp, rp, pair);
	} else if(cistrcmp(pair->attr, "mx") == 0)
		rp = mxrr(entry, pair);
	else if(cistrcmp(pair->attr, "srv") == 0)
		rp = srvrr(entry, pair);
	else if(cistrcmp(pair->attr, "cname") == 0)
		rp = cnamerr(entry, pair);
	else if(cistrcmp(pair->attr, "nullrr") == 0)
		rp = nullrr(entry, pair);
	else if(cistrcmp(pair->attr, "txtrr") == 0)
		rp = txtrr(entry, pair);
	if(rp == nil)
		return;

	rp->owner = dp;
	dnagenever(dp, 1);
	rp->db = 1;
	rp->ttl = intval(entry, pair, "ttl", rp->ttl);
	rrattach(rp, Notauthoritative);
}
static void
dbtuple2cache(Ndbtuple *t)
{
	Ndbtuple *et, *nt;
	DN *dp;

	for(et = t; et; et = et->entry)
		if(strcmp(et->attr, "dom") == 0){
			dp = dnlookup(et->val, Cin, 1);

			/* first same line */
			for(nt = et->line; nt != et; nt = nt->line){
				dbpair2cache(dp, t, nt);
				nt->ptr = 1;
			}

			/* then rest of entry */
			for(nt = t; nt; nt = nt->entry){
				if(nt->ptr == 0)
					dbpair2cache(dp, t, nt);
				nt->ptr = 0;
			}
		}
}
static void
dbfile2cache(Ndb *db)
{
	Ndbtuple *t;

	if(debug)
		dnslog("rereading %s", db->file);
	Bseek(&db->b, 0, 0);
	while(t = ndbparse(db)){
		dbtuple2cache(t);
		ndbfree(t);
	}
}

/* called with dblock held */
static void
loaddomsrvs(void)
{
	Ndbs s;

	if (!cfg.inside || !cfg.straddle || !cfg.serve)
		return;
	if (indoms) {
		ndbfree(indoms);
		ndbfree(innmsrvs);
		ndbfree(outnmsrvs);
		indoms = innmsrvs = outnmsrvs = nil;
	}
	if (db == nil)
		opendatabase();
	free(ndbgetvalue(db, &s, "sys", "inside-dom", "dom", &indoms));
	free(ndbgetvalue(db, &s, "sys", "inside-ns",  "ip",  &innmsrvs));
	free(ndbgetvalue(db, &s, "sys", "outside-ns", "ip",  &outnmsrvs));
	dnslog("[%d] ndb changed: reloaded inside-dom, inside-ns, outside-ns",
		getpid());
}

void
db2cache(int doit)
{
	ulong youngest;
	Ndb *ndb;
	Dir *d;
	static ulong lastcheck, lastyoungest;

	/* no faster than once every 2 minutes */
	if(now < lastcheck + 2*Min && !doit)
		return;

	refresh_areas(owned);

	lock(&dblock);

	if(opendatabase() < 0){
		unlock(&dblock);
		return;
	}

	/*
	 *  file may be changing as we are reading it, so loop till
	 *  mod times are consistent.
	 *
	 *  we don't use the times in the ndb records because they may
	 *  change outside of refreshing our cached knowledge.
	 */
	for(;;){
		lastcheck = now;
		youngest = 0;
		for(ndb = db; ndb; ndb = ndb->next)
			/* dirfstat avoids walking the mount table each time */
			if((d = dirfstat(Bfildes(&ndb->b))) != nil ||
			   (d = dirstat(ndb->file)) != nil){
				if(d->mtime > youngest)
					youngest = d->mtime;
				free(d);
			}
		if(!doit && youngest == lastyoungest)
			break;

		/* forget our area definition */
		freearea(&owned);
		freearea(&delegated);

		/* reopen all the files (to get oldest for time stamp) */
		for(ndb = db; ndb; ndb = ndb->next)
			ndbreopen(ndb);

		/* reload straddle-server configuration */
		loaddomsrvs();

		if(cfg.cachedb){
			/* mark all db records as timed out */
			dnagedb();

			/* read in new entries */
			for(ndb = db; ndb; ndb = ndb->next)
				dbfile2cache(ndb);

			/* mark as authoritative anything in our domain */
			dnauthdb();

			/* remove old entries */
			dnageall(1);
		} else
			/* read all the soa's to get database defaults */
			for(ndb = db; ndb; ndb = ndb->next)
				dbfile2area(ndb);

		doit = 0;
		lastyoungest = youngest;
		createptrs();
	}

	unlock(&dblock);
}

void
dnforceage(void)
{
	lock(&dblock);
	dnageall(1);
	unlock(&dblock);
}

extern uchar	ipaddr[IPaddrlen];	/* my ip address */

/*
 *  get all my xxx
 *  caller ndbfrees the result
 */
Ndbtuple*
lookupinfo(char *attr)
{
	char buf[64];
	char *a[2];
	Ndbtuple *t;

	snprint(buf, sizeof buf, "%I", ipaddr);
	a[0] = attr;

	lock(&dblock);
	if(opendatabase() < 0){
		unlock(&dblock);
		return nil;
	}
	t = ndbipinfo(db, "ip", buf, a, 1);
	unlock(&dblock);
	return t;
}

char *localservers =	  "local#dns#servers";
char *localserverprefix = "local#dns#server";

/*
 *  return non-zero if this is a bad delegation
 */
int
baddelegation(RR *rp, RR *nsrp, uchar *addr)
{
	Ndbtuple *nt;
	static int whined;
	static Ndbtuple *t;

	if(t == nil)
		t = lookupinfo("dom");
	if(t == nil)
		return 0;

	for(; rp; rp = rp->next){
		if(rp->type != Tns)
			continue;

		/* see if delegation is looping */
		if(nsrp)
		if(rp->owner != nsrp->owner)
		if(subsume(rp->owner->name, nsrp->owner->name) &&
		   strcmp(nsrp->owner->name, localservers) != 0){
			dnslog("delegation loop %R -> %R from %I",
				nsrp, rp, addr);
			return 1;
		}

		/* see if delegating to us what we don't own */
		for(nt = t; nt != nil; nt = nt->entry)
			if(rp->host && cistrcmp(rp->host->name, nt->val) == 0)
				break;
		if(nt != nil && !inmyarea(rp->owner->name)){
			if (!whined) {
				whined = 1;
				dnslog("bad delegation %R from %I; "
					"no further logging of them", rp, addr);
			}
			return 1;
		}
	}

	return 0;
}

int
myaddr(char *addr)
{
	char *name, *line, *sp;
	char buf[64];
	Biobuf *bp;

	snprint(buf, sizeof buf, "%I", ipaddr);
	if (strcmp(addr, buf) == 0) {
		dnslog("rejecting my ip %s as local dns server", addr);
		return 1;
	}

	name = smprint("%s/ipselftab", mntpt);
	bp = Bopen(name, OREAD);
	free(name);
	if (bp != nil) {
		while ((line = Brdline(bp, '\n')) != nil) {
			line[Blinelen(bp) - 1] = '\0';
			sp = strchr(line, ' ');
			if (sp) {
				*sp = '\0';
				if (strcmp(addr, line) == 0) {
					dnslog("rejecting my ip %s as local dns server",
						addr);
					return 1;
				}
			}
		}
		Bterm(bp);
	}
	return 0;
}

static char *locdns[20];
static QLock locdnslck;

static void
addlocaldnsserver(DN *dp, int class, char *ipaddr, int i)
{
	int n;
	DN *nsdp;
	RR *rp;
	char buf[32];
	uchar ip[IPaddrlen];

	/* reject our own ip addresses so we don't query ourselves via udp */
	if (myaddr(ipaddr))
		return;

	qlock(&locdnslck);
	for (n = 0; n < i && n < nelem(locdns) && locdns[n]; n++)
		if (strcmp(locdns[n], ipaddr) == 0) {
			dnslog("rejecting duplicate local dns server ip %s",
				ipaddr);
			qunlock(&locdnslck);
			return;
		}
	if (n < nelem(locdns))
		if (locdns[n] == nil || ++n < nelem(locdns))
			locdns[n] = strdup(ipaddr); /* remember 1st few local ns */
	qunlock(&locdnslck);

	/* ns record for name server, make up an impossible name */
	rp = rralloc(Tns);
	snprint(buf, sizeof buf, "%s%d", localserverprefix, i);
	nsdp = dnlookup(buf, class, 1);
	rp->host = nsdp;
	rp->owner = dp;			/* e.g., local#dns#servers */
	rp->local = 1;
	rp->db = 1;
//	rp->ttl = 10*Min;		/* seems too short */
	rp->ttl = (1UL<<31)-1;
	rrattach(rp, Authoritative);	/* will not attach rrs in my area */

	/* A or AAAA record */
	if (parseip(ip, ipaddr) >= 0 && isv4(ip))
		rp = rralloc(Ta);
	else
		rp = rralloc(Taaaa);
	rp->ip = dnlookup(ipaddr, class, 1);
	rp->owner = nsdp;
	rp->local = 1;
	rp->db = 1;
//	rp->ttl = 10*Min;		/* seems too short */
	rp->ttl = (1UL<<31)-1;
	rrattach(rp, Authoritative);	/* will not attach rrs in my area */

	dnslog("added local dns server %s at %s", buf, ipaddr);
}

/*
 *  return list of dns server addresses to use when
 *  acting just as a resolver.
 */
RR*
dnsservers(int class)
{
	int i, n;
	char *p;
	char *args[5];
	Ndbtuple *t, *nt;
	RR *nsrp;
	DN *dp;

	dp = dnlookup(localservers, class, 1);
	nsrp = rrlookup(dp, Tns, NOneg);
	if(nsrp != nil)
		return nsrp;

	p = getenv("DNSSERVER");		/* list of ip addresses */
	if(p != nil){
		n = tokenize(p, args, nelem(args));
		for(i = 0; i < n; i++)
			addlocaldnsserver(dp, class, args[i], i);
		free(p);
	} else {
		t = lookupinfo("@dns");		/* @dns=ip1 @dns=ip2 ... */
		if(t == nil)
			return nil;
		i = 0;
		for(nt = t; nt != nil; nt = nt->entry){
			addlocaldnsserver(dp, class, nt->val, i);
			i++;
		}
		ndbfree(t);
	}

	return rrlookup(dp, Tns, NOneg);
}

static void
addlocaldnsdomain(DN *dp, int class, char *domain)
{
	RR *rp;

	/* ptr record */
	rp = rralloc(Tptr);
	rp->ptr = dnlookup(domain, class, 1);
	rp->owner = dp;
	rp->db = 1;
	rp->ttl = 10*Min;
	rrattach(rp, Authoritative);
}

/*
 *  return list of domains to use when resolving names without '.'s
 */
RR*
domainlist(int class)
{
	Ndbtuple *t, *nt;
	RR *rp;
	DN *dp;

	dp = dnlookup("local#dns#domains", class, 1);
	rp = rrlookup(dp, Tptr, NOneg);
	if(rp != nil)
		return rp;

	t = lookupinfo("dnsdomain");
	if(t == nil)
		return nil;
	for(nt = t; nt != nil; nt = nt->entry)
		addlocaldnsdomain(dp, class, nt->val);
	ndbfree(t);

	return rrlookup(dp, Tptr, NOneg);
}

char *v4ptrdom = ".in-addr.arpa";
char *v6ptrdom = ".ip6.arpa";		/* ip6.int deprecated, rfc 3152 */

char *attribs[] = {
	"ipmask",
	0
};

/*
 *  create ptrs that are in our v4 areas
 */
static void
createv4ptrs(void)
{
	int len, dlen, n;
	char *dom;
	char buf[Domlen+1], ipa[48];
	char *f[40];
	uchar net[IPaddrlen], mask[IPaddrlen];
	Area *s;
	Ndbtuple *t, *nt;

	dlen = strlen(v4ptrdom);
	for(s = owned; s; s = s->next){
		dom = s->soarr->owner->name;
		len = strlen(dom);
		if((len <= dlen || cistrcmp(dom+len-dlen, v4ptrdom) != 0) &&
		    cistrcmp(dom, v4ptrdom+1) != 0)
			continue;

		/* get mask and net value */
		strncpy(buf, dom, sizeof buf);
		buf[sizeof buf-1] = 0;
		/* buf contains something like 178.204.in-addr.arpa (n==4) */
		n = getfields(buf, f, nelem(f), 0, ".");
		memset(mask, 0xff, IPaddrlen);
		ipmove(net, v4prefix);
		switch(n){
		case 3:			/* /8 */
			net[IPv4off] = atoi(f[0]);
			mask[IPv4off+1] = 0;
			mask[IPv4off+2] = 0;
			mask[IPv4off+3] = 0;
			break;
		case 4:			/* /16 */
			net[IPv4off] = atoi(f[1]);
			net[IPv4off+1] = atoi(f[0]);
			mask[IPv4off+2] = 0;
			mask[IPv4off+3] = 0;
			break;
		case 5:			/* /24 */
			net[IPv4off] = atoi(f[2]);
			net[IPv4off+1] = atoi(f[1]);
			net[IPv4off+2] = atoi(f[0]);
			mask[IPv4off+3] = 0;
			break;
		case 6:		/* rfc2317: classless in-addr.arpa delegation */
			net[IPv4off] = atoi(f[3]);
			net[IPv4off+1] = atoi(f[2]);
			net[IPv4off+2] = atoi(f[1]);
			net[IPv4off+3] = atoi(f[0]);
			sprint(ipa, "%I", net);
			t = ndbipinfo(db, "ip", ipa, attribs, 1);
			if(t == nil)	/* could be a reverse with no forward */
				continue;
			nt = look(t, t, "ipmask");
			if(nt == nil){		/* we're confused */
				ndbfree(t);
				continue;
			}
			parseipmask(mask, nt->val);
			ndbfree(t);
			n = 5;
			break;
		default:
			continue;
		}

		/*
		 * go through all domain entries looking for RR's
		 * in this network and create ptrs.
		 * +2 for ".in-addr.arpa".
		 */
		dnptr(net, mask, dom, Ta, 4+2-n, Ptrttl);
	}
}

/* convert bytes to nibbles, big-endian */
void
bytes2nibbles(uchar *nibbles, uchar *bytes, int nbytes)
{
	while (nbytes-- > 0) {
		*nibbles++ = *bytes >> Nibwidth;
		*nibbles++ = *bytes++ & Nibmask;
	}
}

void
nibbles2bytes(uchar *bytes, uchar *nibbles, int nnibs)
{
	for (; nnibs >= 2; nnibs -= 2) {
		*bytes++ = nibbles[0] << Nibwidth | (nibbles[1]&Nibmask);
		nibbles += 2;
	}
	if (nnibs > 0)
		*bytes = nibbles[0] << Nibwidth;
}

/*
 *  create ptrs that are in our v6 areas.  see rfc3596
 */
static void
createv6ptrs(void)
{
	int len, dlen, i, n, pfxnibs;
	char *dom;
	char buf[Domlen+1];
	char *f[40];
	uchar net[IPaddrlen], mask[IPaddrlen];
	uchar nibnet[IPaddrlen*2], nibmask[IPaddrlen*2];
	Area *s;

	dlen = strlen(v6ptrdom);
	for(s = owned; s; s = s->next){
		dom = s->soarr->owner->name;
		len = strlen(dom);
		if((len <= dlen || cistrcmp(dom+len-dlen, v6ptrdom) != 0) &&
		    cistrcmp(dom, v6ptrdom+1) != 0)
			continue;

		/* get mask and net value */
		strncpy(buf, dom, sizeof buf);
		buf[sizeof buf-1] = 0;
		/* buf contains something like 2.0.0.2.ip6.arpa (n==6) */
		n = getfields(buf, f, nelem(f), 0, ".");
		pfxnibs = n - 2;		/* 2 for .ip6.arpa */
		if (pfxnibs < 0 || pfxnibs > V6maxrevdomdepth)
			continue;

		memset(net, 0, IPaddrlen);
		memset(mask, 0xff, IPaddrlen);
		bytes2nibbles(nibnet, net, IPaddrlen);
		bytes2nibbles(nibmask, mask, IPaddrlen);

		/* copy prefix of f, in reverse order, to start of net. */
		for (i = 0; i < pfxnibs; i++)
			nibnet[i] = strtol(f[pfxnibs - 1 - i], nil, 16);
		/* zero nibbles of mask after prefix in net */
		memset(nibmask + pfxnibs, 0, V6maxrevdomdepth - pfxnibs);

		nibbles2bytes(net, nibnet, 2*IPaddrlen);
		nibbles2bytes(mask, nibmask, 2*IPaddrlen);

		/*
		 * go through all domain entries looking for RR's
		 * in this network and create ptrs.
		 */
		dnptr(net, mask, dom, Taaaa, V6maxrevdomdepth - pfxnibs, Ptrttl);
	}
}

/*
 *  create ptrs that are in our areas
 */
static void
createptrs(void)
{
	createv4ptrs();
	createv6ptrs();
}

/*
 * is this domain (or DOMAIN or Domain or dOMAIN)
 * internal to our organisation (behind our firewall)?
 * only inside straddling servers care, everybody else gets told `yes',
 * so they'll use mntpt for their queries.
 */
int
insideaddr(char *dom)
{
	int domlen, vallen, rv;
	Ndbtuple *t;

	if (!cfg.inside || !cfg.straddle || !cfg.serve)
		return 1;

	lock(&dblock);
	if (indoms == nil)
		loaddomsrvs();
	if (indoms == nil) {
		unlock(&dblock);
		return 1;	/* no "inside" sys, try inside nameservers */
	}

	rv = 0;
	domlen = strlen(dom);
	for (t = indoms; t != nil; t = t->entry) {
		if (strcmp(t->attr, "dom") != 0)
			continue;
		vallen = strlen(t->val);
		if (cistrcmp(dom, t->val) == 0 ||
		    domlen > vallen &&
		     cistrcmp(dom + domlen - vallen, t->val) == 0 &&
		     dom[domlen - vallen - 1] == '.') {
			rv = 1;
			break;
		}
	}
	unlock(&dblock);
	return rv;
}

int
insidens(uchar *ip)
{
	uchar ipa[IPaddrlen];
	Ndbtuple *t;

	for (t = innmsrvs; t != nil; t = t->entry)
		if (strcmp(t->attr, "ip") == 0) {
			parseip(ipa, t->val);
			if (memcmp(ipa, ip, sizeof ipa) == 0)
				return 1;
		}
	return 0;
}

uchar *
outsidens(int n)
{
	int i;
	Ndbtuple *t;
	static uchar ipa[IPaddrlen];

	i = 0;
	for (t = outnmsrvs; t != nil; t = t->entry)
		if (strcmp(t->attr, "ip") == 0 && i++ == n) {
			parseip(ipa, t->val);
			return ipa;
		}
	return nil;
}
