#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <ip.h>
#include "dns.h"

static Ndb *db;

Area *owned;
Area *delegated;

static RR*	dblookup1(char*, int, int, int);
static RR*	addrrr(Ndbtuple*, Ndbtuple*);
static RR*	nsrr(Ndbtuple*, Ndbtuple*);
static RR*	cnamerr(Ndbtuple*, Ndbtuple*);
static RR*	mxrr(Ndbtuple*, Ndbtuple*);
static RR*	soarr(Ndbtuple*, Ndbtuple*);
static RR*	ptrrr(Ndbtuple*, Ndbtuple*);
static Ndbtuple* look(Ndbtuple*, Ndbtuple*, char*);
static RR*	doaxfr(Ndb*, char*);
static RR*	nullrr(Ndbtuple *entry, Ndbtuple *pair);
static RR*	txtrr(Ndbtuple *entry, Ndbtuple *pair);
static Lock	dblock;

static int	implemented[Tall] =
{
	[Ta]		1,
	[Tns]		1,
	[Tsoa]		1,
	[Tmx]		1,
	[Tptr]		1,
	[Tcname]	1,
	[Tnull]		1,
	[Ttxt]		1,
};

int
opendatabase(void)
{
	char buf[256];
	Ndb *xdb;

	if(db == nil){
		snprint(buf, sizeof(buf), "%s/ndb", mntpt);
		xdb = ndbopen(dbfile);
		if(xdb != nil)
			xdb->nohash = 1;
		db = ndbcat(ndbopen(buf), xdb);
	}
	if(db == nil)
		return -1;
	else
		return 0;
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
 *       the x.research.bell-labs.com.  If nothing matches, try *.research.bell-labs.com.
 */
RR*
dblookup(char *name, int class, int type, int auth, int ttl)
{
	RR *rp, *tp;
	char buf[256];
	char *wild, *cp;
	DN *dp, *ndp;
	int err;
	static int parallel;
	static int parfd[2];
	static char token[1];

	/* so far only internet lookups are implemented */
	if(class != Cin)
		return 0;

	err = Rname;

	if(type == Tall){
		rp = 0;
		for (type = Ta; type < Tall; type++)
			if(implemented[type])
				rrcat(&rp, dblookup(name, class, type, auth, ttl));
		return rp;
	}

	lock(&dblock);
	dp = dnlookup(name, class, 1);
	if(opendatabase() < 0)
		goto out;
	if(dp->rr)
		err = 0;

	/* first try the given name */
	rp = 0;
	if(cachedb)
		rp = rrlookup(dp, type, NOneg);
	else
		rp = dblookup1(name, type, auth, ttl);
	if(rp)
		goto out;

	/* try lower case version */
	for(cp = name; *cp; cp++)
		*cp = tolower(*cp);
	if(cachedb)
		rp = rrlookup(dp, type, NOneg);
	else
		rp = dblookup1(name, type, auth, ttl);
	if(rp)
		goto out;

	/* walk the domain name trying the wildcard '*' at each position */
	for(wild = strchr(name, '.'); wild; wild = strchr(wild+1, '.')){
		snprint(buf, sizeof(buf), "*%s", wild);
		ndp = dnlookup(buf, class, 1);
		if(ndp->rr)
			err = 0;
		if(cachedb)
			rp = rrlookup(ndp, type, NOneg);
		else
			rp = dblookup1(buf, type, auth, ttl);
		if(rp)
			break;
	}
out:
	/* add owner to uncached records */
	if(rp){
		for(tp = rp; tp; tp = tp->next)
			tp->owner = dp;
	} else
		dp->nonexistent = err;

	unlock(&dblock);
	return rp;
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
	char val[Ndbvlen], dname[Ndbvlen];
	char *attr;
	DN *dp;
	RR *(*f)(Ndbtuple*, Ndbtuple*);
	int found, x;

	dp = 0;
	switch(type){
	case Tptr:
		attr = "ptr";
		f = ptrrr;
		break;
	case Ta:
		attr = "ip";
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
		return nil;
	}

	/*
	 *  find a matching entry in the database
	 */
	t = ndbgetval(db, &s, "dom", name, attr, val);

	/*
	 *  hack for local names
	 */
	if(t == 0 && strchr(name, '.') == 0)
		t = ndbgetval(db, &s, "sys", name, attr, val);
	if(t == 0){
		if(type == Tptr)
			return dbinaddr(dnlookup(name, Cin, 1), ttl);
		else
			return nil;
	}

	/* search whole entry for default domain name */
	strncpy(dname, name, Ndbvlen);
	for(nt = t; nt; nt = nt->entry)
		if(strcmp(nt->attr, "dom") == 0){
			strcpy(dname, nt->val);
			break;
		}

	/* ttl is maximum of soa minttl and entry's ttl ala rfc883 */
	nt = look(t, s.t, "ttl");
	if(nt){
		x = atoi(nt->val);
		if(x > ttl)
			ttl = x;
	}

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
			strcpy(dname, nt->val);
			found = 1;
		}
		if(cistrcmp(attr, nt->attr) == 0){
			rp = (*f)(t, nt);
			rp->auth = auth;
			rp->db = 1;
			if(ttl)
				rp->ttl = ttl;
			if(dp == 0)
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
			if(dp == 0)
				dp = dnlookup(dname, Cin, 1);
			rp->owner = dp;
			*l = rp;
			l = &rp->next;
		}
	ndbfree(t);

	if(list == nil && type == Tptr)
		return dbinaddr(dnlookup(name, Cin, 1), ttl);

	return list;
}

/*
 *  make various types of resource records from a database entry
 */
static RR*
addrrr(Ndbtuple *entry, Ndbtuple *pair)
{
	RR *rp;

	USED(entry);
	rp = rralloc(Ta);
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
static RR*
txtrr(Ndbtuple *entry, Ndbtuple *pair)
{
	RR *rp;

	USED(entry);
	rp = rralloc(Ttxt);
	rp->txt = dnlookup(pair->val, Cin, 1);
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
	RR * rp;

	rp = rralloc(Tmx);
	rp->host = dnlookup(pair->val, Cin, 1);
	pair = look(entry, pair, "pref");
	if(pair)
		rp->pref = atoi(pair->val);
	else
		rp->pref = 1;
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
	rp->soa->refresh = Day;
	rp->soa->retry = Hour;
	rp->soa->expire = Day;
	rp->soa->minttl = Day;
	t = look(entry, pair, "ttl");
	if(t)
		rp->soa->minttl = atoi(t->val);
	t = look(entry, pair, "refresh");
	if(t)
		rp->soa->refresh = atoi(t->val);
	t = look(entry, pair, "serial");
	if(t)
		rp->soa->serial = strtoul(t->val, 0, 10);

	ns = look(entry, pair, "ns");
	if(ns == 0)
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
	if(mb){
		if(strchr(mb->val, '.')) {
			p = strchr(mb->val, '@');
			if(p != nil)
				*p = '.';
			rp->rmb = dnlookup(mb->val, Cin, 1);
		} else {
			snprint(mailbox, sizeof(mailbox), "%s.%s",
				mb->val, ns->val);
			rp->rmb = dnlookup(mailbox, Cin, 1);
		}
	} else {
		snprint(mailbox, sizeof(mailbox), "postmaster.%s",
			ns->val);
		rp->rmb = dnlookup(mailbox, Cin, 1);
	}
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

static char *ia = ".in-addr.arpa";
#define IALEN 13

/*
 *  answer in-addr.arpa queries by making up a ptr record if the address
 *  is in our database
 */
RR*
dbinaddr(DN *dp, int ttl)
{
	int len;
	char ip[Domlen];
	char dom[Domlen];
	char *np, *p;
	Ndbtuple *t, *nt;
	RR *rp;
	Ndbs s;
	long x;

	if(opendatabase() < 0)
		return 0;

	len = strlen(dp->name);
	if(len <= IALEN)
		return 0;
	p = dp->name + len - IALEN;
	if(cistrcmp(p, ia) != 0)
		return 0;

	/* flip ip components into sensible order */
	np = ip;
	len = 0;
	while(p >= dp->name){
		len++;
		p--;
		if(*p == '.'){
			memmove(np, p+1, len);
			np += len;
			len = 0;
		}
	}
	memmove(np, p+1, len-1);
	np += len-1;
	*np = 0;

	/* look in local database */
	t = ndbgetval(db, &s, "ip", ip, "dom", dom);
	if(t == 0)
		return 0;
	ndbfree(t);

	/* ttl is maximum of soa minttl and entry's ttl ala rfc883 */
	nt = look(t, s.t, "ttl");
	if(nt){
		x = atoi(nt->val);
		if(x > ttl)
			ttl = x;
	}

	/* default ttl is one day */
	if(ttl < 0)
		ttl = DEFTTL;

	/* create an RR record for a ptr */
	rp = rralloc(Tptr);
	rp->db = 1;
	rp->owner = dp;
	rp->ptr = dnlookup(dom, Cin, 1);
	rp->ttl = ttl;
	return rp;
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

void
addptr(DN *dp, Ndbtuple *entry, Ndbtuple *pair)
{
	RR *rp;
	int len;
	Ndbtuple *t;
	char *p, *np;
	DN *ipdp;
	char ip[Domlen];

	len = strlen(pair->val);
	if(len >= Domlen - IALEN)
		return;
	p = pair->val + len;

	/* flip ip components into reverse order */
	np = ip;
	len = 0;
	while(p >= pair->val){
		len++;
		p--;
		if(*p == '.'){
			memmove(np, p+1, len-1);
			np[len-1] = '.';
			np += len;
			len = 0;
		}
	}
	memmove(np, p+1, len-1);
	np += len-1;
	*np = 0;
	strcat(np, ".in-addr.arpa");
	ipdp = dnlookup(ip, Cin, 1);

	rp = rralloc(Tptr);
	rp->ptr = dp;

	rp->owner = ipdp;
	rp->db = 1;
	t = look(entry, pair, "ttl");
	if(t)
		rp->ttl = atoi(t->val);
	rrattach(rp, 0);
}

/*
 *  our area is the part of the domain tree that
 *  we serve
 */
static void
addarea(DN *dp, RR *rp, Ndbtuple *t)
{
	Area **l, *s;

	if(t->val[0])
		l = &delegated;
	else
		l = &owned;

	/*
	 *  The area contains a copy of the soa rr that created it.
	 *  The owner of the the soa rr should stick around as long
	 *  as the area does.
	 */
	s = emalloc(sizeof(*s));
	s->len = strlen(dp->name);
	rrcopy(rp, &s->soarr);
	s->soarr->owner = dp;

	s->next = *l;
	*l = s;
}

static void
freearea(Area **l)
{
	Area *s;

	while(s = *l){
		*l = s->next;
		rrfree(s->soarr);
		free(s);
	}
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
		syslog(0, logfile, "rereading %s", db->file);
	Bseek(&db->b, 0, 0);
	while(t = ndbparse(db)){
		ndbfree(t);
	}
}

/*
 *  read the database into the cache
 */
static void
dbpair2cache(DN *dp, Ndbtuple *entry, Ndbtuple *pair)
{
	RR *rp;
	Ndbtuple *t;

	rp = 0;
	if(cistrcmp(pair->attr, "ip") == 0){
		rp = addrrr(entry, pair);
		addptr(dp, entry, pair);
	} else if(cistrcmp(pair->attr, "ns") == 0){
		rp = nsrr(entry, pair);
	} else if(cistrcmp(pair->attr, "soa") == 0){
		rp = soarr(entry, pair);
		addarea(dp, rp, pair);
	} else if(cistrcmp(pair->attr, "mx") == 0){
		rp = mxrr(entry, pair);
	} else if(cistrcmp(pair->attr, "cname") == 0){
		rp = cnamerr(entry, pair);
	} else if(cistrcmp(pair->attr, "nullrr") == 0){
		rp = nullrr(entry, pair);
	} else if(cistrcmp(pair->attr, "txtrr") == 0){
		rp = txtrr(entry, pair);
	}

	if(rp == 0)
		return;

	rp->owner = dp;
	rp->db = 1;
	t = look(entry, pair, "ttl");
	if(t)
		rp->ttl = atoi(t->val);
	rrattach(rp, 0);
}
static void
dbtuple2cache(Ndbtuple *t)
{
	Ndbtuple *et, *nt;
	DN *dp;

	for(et = t; et; et = et->entry){
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
}
static void
dbfile2cache(Ndb *db)
{
	Ndbtuple *t;

	if(debug)
		syslog(0, logfile, "rereading %s", db->file);
	Bseek(&db->b, 0, 0);
	while(t = ndbparse(db)){
		dbtuple2cache(t);
		ndbfree(t);
	}
}
void
db2cache(int doit)
{
	Ndb *ndb;
	Dir *d;
	ulong youngest, temp;
	static ulong lastcheck;
	static ulong lastyoungest;

	if(now < lastcheck + 2*Min && !doit)
		return;

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
		for(ndb = db; ndb; ndb = ndb->next){
			/* the dirfstat avoids walking the mount table each time */
			if((d = dirfstat(Bfildes(&ndb->b))) != nil ||
			   (d = dirstat(ndb->file)) != nil){
				temp = d->mtime;		/* ulong vs int crap */
				if(temp > youngest)
					youngest = temp;
				free(d);
			}
		}
		if(!doit && youngest == lastyoungest){
			unlock(&dblock);
			return;
		}
	
		/* forget our area definition */
		freearea(&owned);
		freearea(&delegated);
	
		/* reopen all the files (to get oldest for time stamp) */
		for(ndb = db; ndb; ndb = ndb->next)
			ndbreopen(ndb);

		if(cachedb){
			/* mark all db records as timed out */
			dnagedb();
	
			/* read in new entries */
			for(ndb = db; ndb; ndb = ndb->next)
				dbfile2cache(ndb);
	
			/* mark as authentic anything in our domain */
			dnauthdb();
	
			/* remove old entries */
			dnageall(1);
		} else {
			/* read all the soa's to get database defaults */
			for(ndb = db; ndb; ndb = ndb->next)
				dbfile2area(ndb);
		}

		doit = 0;
		lastyoungest = youngest;
	}

	unlock(&dblock);
}

/*
 *  true if a name is in our area
 */
Area*
inmyarea(char *name)
{
	int len;
	Area *s, *d;

	len = strlen(name);
	for(s = owned; s; s = s->next){
		if(s->len > len)
			continue;
		if(cistrcmp(s->soarr->owner->name, name + len - s->len) == 0)
			if(len == s->len || name[len - s->len - 1] == '.')
				break;
	}
	if(s == 0)
		return 0;

	for(d = delegated; d; d = d->next){
		if(d->len > len)
			continue;
		if(cistrcmp(d->soarr->owner->name, name + len - d->len) == 0)
			if(len == d->len || name[len - d->len - 1] == '.')
				return 0;
	}

	return s;
}

extern uchar	ipaddr[IPaddrlen];

/*
 *  get all my xxx
 */
Ndbtuple*
lookupinfo(char *attr)
{
	char buf[64];
	char *a[2];
	static Ndbtuple *t;

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

char *localservers = "local#dns#servers";
char *localserverprefix = "local#dns#server";

/*
 *  return non-zero is this is a bad delegation
 */
int
baddelegation(RR *rp, RR *nsrp, uchar *addr)
{
	Ndbtuple *nt;
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
			syslog(0, logfile, "delegation loop %R -> %R from %I", nsrp, rp, addr);
			return 1;
		}

		/* see if delegating to us what we don't own */
		for(nt = t; nt != nil; nt = nt->entry)
			if(rp->host && cistrcmp(rp->host->name, nt->val) == 0)
				break;
		if(nt != nil && !inmyarea(rp->owner->name)){
			syslog(0, logfile, "bad delegation %R from %I", rp, addr);
			return 1;
		}
	}

	return 0;
}

static void
addlocaldnsserver(DN *dp, int class, char *ipaddr, int i)
{
	DN *nsdp;
	RR *rp;
	char buf[32];

	/* ns record for name server, make up an impossible name */
	rp = rralloc(Tns);
	snprint(buf, sizeof(buf), "%s%d", localserverprefix, i);
	nsdp = dnlookup(buf, class, 1);
	rp->host = nsdp;
	rp->owner = dp;
	rp->local = 1;
	rp->db = 1;
	rp->ttl = 10*Min;
	rrattach(rp, 1);

	/* A record */
	rp = rralloc(Ta);
	rp->ip = dnlookup(ipaddr, class, 1);
	rp->owner = nsdp;
	rp->local = 1;
	rp->db = 1;
	rp->ttl = 10*Min;
	rrattach(rp, 1);
}

/*
 *  return list of dns server addresses to use when
 *  acting just as a resolver.
 */
RR*
dnsservers(int class)
{
	Ndbtuple *t, *nt;
	RR *nsrp;
	DN *dp;
	char *p;
	int i, n;
	char *buf, *args[5];

	dp = dnlookup(localservers, class, 1);
	nsrp = rrlookup(dp, Tns, NOneg);
	if(nsrp != nil)
		return nsrp;

	p = getenv("DNSSERVER");
	if(p != nil){
		buf = estrdup(p);
		n = tokenize(buf, args, nelem(args));
		for(i = 0; i < n; i++)
			addlocaldnsserver(dp, class, args[i], i);
		free(buf);
	} else {
		t = lookupinfo("@dns");
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

	/* A record */
	rp = rralloc(Tptr);
	rp->ptr = dnlookup(domain, class, 1);
	rp->owner = dp;
	rp->db = 1;
	rp->ttl = 10*Min;
	rrattach(rp, 1);
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
