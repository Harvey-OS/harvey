#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <lock.h>
#include "dns.h"

static Ndb *db;

static RR*	dblookup1(char*, int, int);
static RR*	addrrr(Ndbtuple*, Ndbtuple*);
static RR*	nsrr(Ndbtuple*, Ndbtuple*);
static RR*	mxrr(Ndbtuple*, Ndbtuple*);
static RR*	soarr(Ndbtuple*, Ndbtuple*);
static Ndbtuple* look(Ndbtuple*, Ndbtuple*, char*);

static Lock	dblock;

static int	implemented[Tall] =
{
	[Ta]	1,
	[Tns]	1,
	[Tsoa]	1,
	[Tmx]	1,
};

/*
 *  lookup an RR in the network database, look for matches
 *  against both the domain name and the wildcarded domain name.
 *
 *  the lock makes sure only one process can be accessing the data
 *  base at a time.  This is important since there's a lot of
 *  shared state there.
 *
 *  e.g. for x.research.att.com, first look for a match against
 *       the x.research.att.com.  If nothing matches, try *.research.att.com.
 */
RR*
dblookup(char *name, int class, int type, int auth)
{
	RR *rp, *tp;
	char buf[256];
	char *wild, *cp;
	DN *dp;
	static int parallel;
	static int parfd[2];
	static char token[1];

	/* so far only internet lookups are implemented */
	if(class != Cin)
		return 0;

	if(type == Tall){
		rp = 0;
		for (type = Ta; type <= Tall; type++)
			if(implemented[type])
				rrcat(&rp, dblookup(name, class, type, auth), type);
		return rp;
	}

	lock(&dblock);
	rp = 0;
	if(db == 0){
		db = ndbopen(dbfile);
		if(db == 0)
			goto out;
	}

	/* first try the given name */
	rp = dblookup1(name, type, auth);
	if(rp)
		goto out;

	/* try lower case version */
	for(cp = name; *cp; cp++)
		*cp = tolower(*cp);
	rp = dblookup1(name, type, auth);
	if(rp)
		goto out;

	/* now try the given name with the first element replaced by '*' */
	wild = strchr(name, '.');
	if(wild == 0)
		goto out;

	snprint(buf, sizeof(buf), "*%s", wild);
	rp = dblookup1(buf, type, auth);
	dp = dnlookup(name, class, 1);
	for(tp = rp; tp; tp = tp->next)
		tp->owner = dp;
out:
	unlock(&dblock);
	return rp;
}

/*
 *  lookup an RR in the network database
 */
static RR*
dblookup1(char *name, int type, int auth)
{
	Ndbtuple *t, *nt;
	RR *rp, *list, **l;
	Ndbs s;
	char val[Ndbvlen];
	char *attr;
	DN *dp;
	RR *(*f)(Ndbtuple*, Ndbtuple*);

	dp = 0;
	switch(type){
	case Ta:
		attr = "ip";
		f = addrrr;
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
	default:
		return 0;
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
	if(t == 0)
		return 0;

	/*
	 *  The database has 2 levels of precedence; line and entry.
	 *  Pairs on the same line bind tighter than pairs in the
	 *  same entry, so we search the line first.
	 */
	list = 0;
	l = &list;
	for(nt = s.t;; ){
		if(cistrcmp(attr, nt->attr) == 0){
			rp = (*f)(t, nt);
			rp->auth = auth;
			rp->db = 1;
			if(dp == 0)
				dp = dnlookup(name, Cin, 1);
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
			rp->auth = auth;
			if(dp == 0)
				dp = dnlookup(name, Cin, 1);
			rp->owner = dp;
			*l = rp;
			l = &rp->next;
		}
	ndbfree(t);

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

	rp = rralloc(Tns);
	rp->host = dnlookup(pair->val, Cin, 1);
	if(look(entry, pair, "soa"))
		rp->local = 1;
	return rp;

}

static RR*
soarr(Ndbtuple *entry, Ndbtuple *pair)
{
	RR *rp;
	Ndbtuple *ns, *mb;
	char mailbox[Domlen];

	USED(entry);
	USED(pair);
	rp = rralloc(Tsoa);
	rp->local = 1;
	rp->soa->serial = db->mtime;
	rp->soa->refresh = Week;
	rp->soa->retry = Hour;
	rp->soa->expire = Day;
	rp->soa->minttl = Day;
	ns = look(entry, pair, "ns");
	if(ns == 0)
		ns = look(entry, pair, "dom");
	rp->host = dnlookup(ns->val, Cin, 1);
	mb = look(entry, pair, "mb");
	if(mb){
		if(strchr(mb->val, '.'))
			rp->rmb = dnlookup(mb->val, Cin, 1);
		else {
			strcpy(mailbox, mb->val);
			strcat(mailbox, ns->val);
			rp->rmb = dnlookup(mailbox, Cin, 1);
		}
	} else {
		strcpy(mailbox, "postmaster.");
		strcat(mailbox, ns->val);
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
dbinaddr(DN *dp)
{
	int len;
	char ip[Domlen];
	char dom[Domlen];
	char *np, *p;
	Ndbtuple *t;
	RR *rp;
	Ndbs s;

	rp = 0;
	lock(&dblock);
	if(db == 0){
		db = ndbopen(dbfile);
		if(db == 0)
			goto out;
	}

	len = strlen(dp->name);
	if(len <= IALEN)
		return 0;
	p = dp->name + len - IALEN;
	if(cistrcmp(p, ia) != 0)
		goto out;

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

	/* create an RR record for a ptr */
	rp = rralloc(Tptr);
	rp->db = 1;
	rp->owner = dp;
	rp->ptr = dnlookup(dom, Cin, 1);
out:
	unlock(&dblock);
	return rp;
}
