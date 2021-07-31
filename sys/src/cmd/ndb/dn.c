#include <u.h>
#include <libc.h>
#include <ip.h>
#include <pool.h>
#include "dns.h"

/*
 *  Hash table for domain names.  The hash is based only on the
 *  first element of the domain name.
 */
DN	*ht[HTLEN];


static struct
{
	Lock;
	ulong	names;	/* names allocated */
	ulong	oldest;	/* longest we'll leave a name around */
	int	active;
	int	mutex;
	int	id;
} dnvars;

/* names of RR types */
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
[Tkey]		"key",
[Tcert]		"cert",
[Tsig]		"sig",
[Tixfr]		"ixfr",
[Taxfr]		"axfr",
[Tall]		"all",
		0,
};

/* names of response codes */
char *rname[] =
{
[Rok]			"ok",
[Rformat]		"format error",
[Rserver]		"server failure",
[Rname]			"bad name",
[Runimplimented]	"unimplemented",
[Rrefused]		"we don't like you",
};

/* names of op codes */
char *opname[] =
{
[Oquery]	"query",
[Oinverse]	"inverse",
[Ostatus]	"status",
};

Lock	dnlock;

static void* allocate(int);
static void checkallocation(void*, int);

#define CHECK(a) checkallocation(a, sizeof(*a))

/*
 *  set up a pipe to use as a lock
 */
void
dninit(void)
{
	fmtinstall('E', eipconv);
	fmtinstall('I', eipconv);
	fmtinstall('V', eipconv);
	fmtinstall('R', rrconv);
	fmtinstall('Q', rravconv);

	dnvars.oldest = maxage;
	dnvars.names = 0;
}

/*
 *  hash for a domain name
 */
static ulong
dnhash(char *name)
{
	ulong hash;
	uchar *val = (uchar*)name;

	for(hash = 0; *val; val++)
		hash = (hash*13) + tolower(*val)-'a';
	return hash % HTLEN;
}

/*
 *  lookup a symbol.  if enter is not zero and the name is
 *  not found, create it.
 */
DN*
dnlookup(char *name, int class, int enter)
{
	DN **l;
	DN *dp;

	l = &ht[dnhash(name)];
	lock(&dnlock);
	for(dp = *l; dp; dp = dp->next) {
		assert(dp->magic == DNmagic);
		if(dp->class == class && cistrcmp(dp->name, name) == 0){
			dp->referenced = now;
			unlock(&dnlock);
			return dp;
		}
		l = &dp->next;
	}
	if(enter == 0){
		unlock(&dnlock);
		return 0;
	}
	dnvars.names++;
	dp = allocate(sizeof(*dp));
	dp->magic = DNmagic;
	dp->name = strdup(name);
	assert(dp->name != 0);
	dp->class = class;
	dp->rr = 0;
	dp->next = 0;
	dp->referenced = now;
	*l = dp;
	unlock(&dnlock);

	return dp;
}

/*
 *  dump the cache
 */
void
dndump(char *file)
{
	DN *dp;
	int i, fd;
	RR *rp;

	fd = open(file, OWRITE|OTRUNC);
	if(fd < 0)
		return;
	lock(&dnlock);
	for(i = 0; i < HTLEN; i++){
		for(dp = ht[i]; dp; dp = dp->next){
			fprint(fd, "%s\n", dp->name);
			for(rp = dp->rr; rp; rp = rp->next)
				fprint(fd, "	%R %c%c %lud/%lud\n", rp, rp->auth?'A':'U',
					rp->db?'D':'N', rp->expire, rp->ttl);
		}
	}
	unlock(&dnlock);
	close(fd);
}

/*
 *  purge all records
 */
void
dnpurge(void)
{
	DN *dp;
	RR *rp;
	int i;

	lock(&dnlock);

	for(i = 0; i < HTLEN; i++)
		for(dp = ht[i]; dp; dp = dp->next){
			rp = dp->rr;
			dp->rr = nil;
			for(; rp != nil; rp = rp->next)
				rp->cached = 0;
			rrfreelist(dp->rr);
		}

	unlock(&dnlock);
}

/*
 *  check the age of resource records, free any that have timed out
 */
void
dnage(DN *dp)
{
	RR **l;
	RR *rp, *next;
	ulong diff;

	diff = now - dp->referenced;
	if(diff < Reserved)
		return;

	l = &dp->rr;
	for(rp = dp->rr; rp; rp = next){
		assert(rp->magic == RRmagic && rp->cached);
		next = rp->next;
		if(!rp->db)
		if(rp->expire < now || diff > dnvars.oldest){
			*l = next;
			rp->cached = 0;
			rrfree(rp);
			continue;
		}
		l = &rp->next;
	}
}

#define REF(x) if(x) x->refs++

/*
 *  our target is 4000 names cached, this should be larger on large servers
 */
#define TARGET 4000

/*
 *  periodicly sweep for old records and remove unreferenced domain names
 *
 *  only called when all other threads are locked out
 */
void
dnageall(int doit)
{
	DN *dp, **l;
	int i;
	RR *rp;
	static ulong nextage;

	if(dnvars.names < TARGET && now < nextage && !doit){
		dnvars.oldest = maxage;
		return;
	}

	if(dnvars.names > TARGET)
		dnvars.oldest /= 2;
	nextage = now + maxage;

	lock(&dnlock);

	/* time out all old entries (and set refs to 0) */
	for(i = 0; i < HTLEN; i++)
		for(dp = ht[i]; dp; dp = dp->next){
			dp->refs = 0;
			dnage(dp);
		}

	/* mark all referenced domain names */
	for(i = 0; i < HTLEN; i++)
		for(dp = ht[i]; dp; dp = dp->next)
			for(rp = dp->rr; rp; rp = rp->next){
				REF(rp->owner);
				if(rp->negative){
					REF(rp->negsoaowner);
					continue;
				}
				switch(rp->type){
				case Thinfo:
					REF(rp->cpu);
					REF(rp->os);
					break;
				case Ttxt:
					REF(rp->txt);
					break;
				case Tcname:
				case Tmb:
				case Tmd:
				case Tmf:
				case Tns:
					REF(rp->host);
					break;
				case Tmg:
				case Tmr:
					REF(rp->mb);
					break;
				case Tminfo:
					REF(rp->rmb);
					REF(rp->mb);
					break;
				case Trp:
					REF(rp->rmb);
					REF(rp->txt);
					break;
				case Tmx:
					REF(rp->host);
					break;
				case Ta:
					REF(rp->ip);
					break;
				case Tptr:
					REF(rp->ptr);
					break;
				case Tsoa:
					REF(rp->host);
					REF(rp->rmb);
					break;
				}
			}

	/* sweep and remove unreferenced domain names */
	for(i = 0; i < HTLEN; i++){
		l = &ht[i];
		for(dp = *l; dp; dp = *l){
			if(dp->rr == 0 && dp->refs == 0){
				*l = dp->next;
				if(dp->name)
					free(dp->name);
				dnvars.names--;
				dncheck(dp, 0);
				free(dp);
				continue;
			}
			l = &dp->next;
		}
	}

	unlock(&dnlock);
}

/*
 *  timeout all database records (used when rereading db)
 */
void
dnagedb(void)
{
	DN *dp;
	int i;
	RR *rp;
	static ulong nextage;

	lock(&dnlock);

	/* time out all database entries */
	for(i = 0; i < HTLEN; i++)
		for(dp = ht[i]; dp; dp = dp->next)
			for(rp = dp->rr; rp; rp = rp->next)
				if(rp->db)
					rp->expire = 0;

	unlock(&dnlock);
}

/*
 *  mark all local db records about my area as authoritative, time out any others
 */
void
dnauthdb(void)
{
	DN *dp;
	int i;
	Area *area;
	RR *rp;
	static ulong nextage;

	lock(&dnlock);

	/* time out all database entries */
	for(i = 0; i < HTLEN; i++)
		for(dp = ht[i]; dp; dp = dp->next){
			area = inmyarea(dp->name);
			for(rp = dp->rr; rp; rp = rp->next)
				if(rp->db){
					if(area){
						if(rp->ttl < area->soarr->soa->minttl)
							rp->ttl = area->soarr->soa->minttl;
						rp->auth = 1;
					}
					if(rp->expire == 0){
						rp->db = 0;
						dp->referenced = now - Reserved - 1;
					}
				}
		}

	unlock(&dnlock);
}

/*
 *  keep track of other processes to know if we can
 *  garbage collect.  block while garbage collecting.
 */
int
getactivity(Request *req)
{
	int rv;

	lock(&dnvars);
	while(dnvars.mutex){
		unlock(&dnvars);
		sleep(200);
		lock(&dnvars);
	}
	rv = ++dnvars.active;
	now = time(0);
	req->id = ++dnvars.id;
	unlock(&dnvars);

	return rv;
}
void
putactivity(void)
{
	static ulong lastclean;

	lock(&dnvars);
	dnvars.active--;
	assert(dnvars.active >= 0); /* "dnvars.active %d", dnvars.active */;

	/*
	 *  clean out old entries and check for new db periodicly
	 */
	if(dnvars.mutex || (needrefresh == 0 && dnvars.active > 0)){
		unlock(&dnvars);
		return;
	}

	/* wait till we're alone */
	dnvars.mutex = 1;
	while(dnvars.active > 0){
		unlock(&dnvars);
		sleep(100);
		lock(&dnvars);
	}
	unlock(&dnvars);

	db2cache(needrefresh);
	dnageall(0);

	/* let others back in */
	lastclean = now;
	needrefresh = 0;
	dnvars.mutex = 0;
}

/*
 *  Attach a single resource record to a domain name.
 *	- Avoid duplicates with already present RR's
 *	- Chain all RR's of the same type adjacent to one another
 *	- chain authoritative RR's ahead of non-authoritative ones
 */
static void
rrattach1(RR *new, int auth)
{
	RR **l;
	RR *rp;
	DN *dp;

	assert(new->magic == RRmagic && !new->cached);

	if(!new->db)
		new->expire = new->ttl;
	else
		new->expire = now + Year;
	dp = new->owner;
	assert(dp->magic == DNmagic);
	new->auth |= auth;
	new->next = 0;
	CHECK(new);

	/*
	 *  find first rr of the right type, similar types
	 *  are grouped mostly for debugging
	 */
	l = &dp->rr;
	for(rp = *l; rp; rp = *l){
		assert(rp->magic == RRmagic && rp->cached);
		if(rp->type == new->type)
			break;
		l = &rp->next;
	}

	/*
	 *  negative entries replace positive entries
	 *  positive entries replace negative entries
	 *  newer entries replace older entries with the same fields
	 */
	for(rp = *l; rp; rp = *l){
		CHECK(rp);
		assert(rp->magic == RRmagic && rp->cached);
		if(rp->type != new->type)
			break;

		if(rp->db == new->db && rp->auth == new->auth){
			/* negative drives out positive and vice versa */
			if(rp->negative != new->negative){
				*l = rp->next;
				rp->cached = 0;
				rrfree(rp);
				continue;
			}

			/* all things equal, pick the newer one */
			if(rp->arg0 == new->arg0 && rp->arg1 == new->arg1){
				/* new drives out old */
				if(new->ttl > rp->ttl || new->expire > rp->expire){
					*l = rp->next;
					rp->cached = 0;
					rrfree(rp);
					continue;
				} else {
					rrfree(new);
					return;
				}
			}
		}
		l = &rp->next;
	}

	/*
	 *  add to chain
	 */
	new->cached = 1;
	new->next = *l;
	*l = new;
}

/*
 *  Attach a list of resource records to a domain name.
 *	- Avoid duplicates with already present RR's
 *	- Chain all RR's of the same type adjacent to one another
 *	- chain authoritative RR's ahead of non-authoritative ones
 *	- remove any expired RR's
 */
void
rrattach(RR *rp, int auth)
{
	RR *next;

	lock(&dnlock);
	for(; rp; rp = next){
		next = rp->next;
		rp->next = 0;

		/* avoid any outside spoofing */
		if(cachedb && !rp->db && inmyarea(rp->owner->name))
			rrfree(rp);
		else
			rrattach1(rp, auth);
	}
	unlock(&dnlock);
}

/*
 *  allocate a resource record of a given type
 */
RR*
rralloc(int type)
{
	RR *rp;

	rp = allocate(sizeof(*rp));
	rp->magic = RRmagic;
	rp->pc = getcallerpc(&type);
	rp->type = type;
	switch(type){
	case Tsoa:
		rp->soa = allocate(sizeof(*rp->soa));
		break;
	case Tkey:
		rp->key = allocate(sizeof(*rp->key));
		break;
	case Tcert:
		rp->cert = allocate(sizeof(*rp->cert));
		break;
	case Tsig:
		rp->sig = allocate(sizeof(*rp->sig));
		break;
	}
	rp->ttl = 0;
	rp->expire = 0;
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

	assert(!rp->cached);
	CHECK(rp);

	dp = rp->owner;
	if(dp){
		assert(dp->magic == DNmagic);
		for(nrp = dp->rr; nrp; nrp = nrp->next)
			assert(nrp != rp); /* "rrfree of live rr" */;
	}

	switch(rp->type){
	case Tsoa:
		if(rp->soa){
			CHECK(rp->soa);
			free(rp->soa);
		}
		break;
	case Tkey:
		if(rp->key){
			if(rp->key->data){
				CHECK(rp->key->data);
				free(rp->key->data);
			}
			CHECK(rp->key);
			free(rp->key);
		}
		break;
	case Tcert:
		if(rp->cert){
			if(rp->cert->data){
				CHECK(rp->cert->data);
				free(rp->cert->data);
			}
			CHECK(rp->cert);
			free(rp->cert);
		}
		break;
	case Tsig:
		if(rp->sig){
			if(rp->sig->data){
				CHECK(rp->sig->data);
				free(rp->sig->data);
			}
			CHECK(rp->sig);
			free(rp->sig);
		}
		break;
	}

	free(rp);
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

extern RR**
rrcopy(RR *rp, RR **last)
{
	RR *nrp;
	SOA *soa;

	CHECK(rp);
	nrp = rralloc(rp->type);
	soa = nrp->soa;
	*nrp = *rp;
	if(rp->type == Tsoa){
		nrp->soa = soa;
		*nrp->soa = *rp->soa;
	}
	nrp->cached = 0;
	nrp->next = 0;
	*last = nrp;
	CHECK(nrp);
	return &nrp->next;
}

/*
 *  lookup a resource record of a particular type and
 *  class attached to a domain name.  Return copies.
 *
 *  Priority ordering is:
 *	db authoritative
 *	not timed out network authoritative
 *	not timed out network unauthoritative
 *	unauthoritative db
 *
 *  if flag NOneg is set, don't return negative cached entries.
 *  return nothing instead.
 */
RR*
rrlookup(DN *dp, int type, int flag)
{
	RR *rp, *first, **last;

	assert(dp->magic == DNmagic);

	first = 0;
	last = &first;
	lock(&dnlock);

	/* try for an authoritative db entry */
	for(rp = dp->rr; rp; rp = rp->next){
		CHECK(rp);
		assert(rp->magic == RRmagic && rp->cached);
		if(rp->db)
		if(rp->auth)
		if(tsame(type, rp->type))
			last = rrcopy(rp, last);
	}
	if(first)
		goto out;

	/* try for an living authoritative network entry */
	for(rp = dp->rr; rp; rp = rp->next){
		if(!rp->db)
		if(rp->auth)
		if(rp->ttl + 60 > now)
		if(tsame(type, rp->type)){
			if(flag == NOneg && rp->negative)
				goto out;
			last = rrcopy(rp, last);
		}
	}
	if(first)
		goto out;

	/* try for an living unauthoritative network entry */
	for(rp = dp->rr; rp; rp = rp->next){
		if(!rp->db)
		if(rp->ttl + 60 > now)
		if(tsame(type, rp->type)){
			if(flag == NOneg && rp->negative)
				goto out;
			last = rrcopy(rp, last);
		}
	}
	if(first)
		goto out;

	/* try for an unauthoritative db entry */
	for(rp = dp->rr; rp; rp = rp->next){
		if(rp->db)
		if(tsame(type, rp->type))
			last = rrcopy(rp, last);
	}
	if(first)
		goto out;

	/* otherwise, settle for anything we got (except for negative caches)  */
	for(rp = dp->rr; rp; rp = rp->next){
		if(tsame(type, rp->type)){
			if(rp->negative)
				goto out;
			last = rrcopy(rp, last);
		}
	}

out:
	unlock(&dnlock);
	unique(first);
	return first;
}

/*
 *  convert an ascii RR type name to its integer representation
 */
int
rrtype(char *atype)
{
	int i;

	for(i = 0; i <= Tall; i++)
		if(rrtname[i] && strcmp(rrtname[i], atype) == 0)
			return i;

	// make any a synonym for all
	if(strcmp(atype, "any") == 0)
		return Tall;
	return atoi(atype);
}

/*
 *  convert an integer RR type to it's ascii name
 */
char*
rrname(int type, char *buf)
{
	char *t;

	t = 0;
	if(type <= Tall)
		t = rrtname[type];
	if(t==0){
		sprint(buf, "%d", type);
		t = buf;
	}
	return t;
}

/*
 *  compare 2 types
 */
int
tsame(int t1, int t2)
{
	return t1 == t2 || t1 == Tall;
}

/*
 *  Add resource records to a list, duplicate them if they are cached
 *  RR's since these are shared.
 */
RR*
rrcat(RR **start, RR *rp)
{
	RR **last;

	last = start;
	while(*last != 0)
		last = &(*last)->next;

	*last = rp;
	return *start;
}

/*
 *  remove negative cache rr's from an rr list
 */
RR*
rrremneg(RR **l)
{
	RR **nl, *rp;
	RR *first;

	first = nil;
	nl = &first;
	while(*l != nil){
		rp = *l;
		if(rp->negative){
			*l = rp->next;
			*nl = rp;
			nl = &rp->next;
			*nl = nil;
		} else
			l = &rp->next;
	}

	return first;
}

/*
 *  remove rr's of a particular type from an rr list
 */
RR*
rrremtype(RR **l, int type)
{
	RR **nl, *rp;
	RR *first;

	first = nil;
	nl = &first;
	while(*l != nil){
		rp = *l;
		if(rp->type == type){
			*l = rp->next;
			*nl = rp;
			nl = &rp->next;
			*nl = nil;
		} else
			l = &(*l)->next;
	}

	return first;
}

/*
 *  print conversion for rr records
 */
int
rrconv(va_list *arg, Fconv *f)
{
	RR *rp;
	int n;
	char buf[3*Domlen];

	rp = va_arg(*arg, RR*);
	if(rp == 0){
		strcpy(buf, "<null>");
		goto out;
	}

	n = snprint(buf, sizeof(buf), "%s %s", rp->owner->name,
		rrname(rp->type, buf));

	if(rp->negative){
		snprint(&buf[n], sizeof(buf)-n, "\tnegative - rcode %d", rp->negrcode);
		goto out;
	}

	switch(rp->type){
	case Thinfo:
		snprint(&buf[n], sizeof(buf)-n, "\t%s %s", rp->cpu->name, rp->os->name);
		break;
	case Tcname:
	case Tmb:
	case Tmd:
	case Tmf:
	case Tns:
		snprint(&buf[n], sizeof(buf)-n, "\t%s", rp->host->name);
		break;
	case Tmg:
	case Tmr:
		snprint(&buf[n], sizeof(buf)-n, "\t%s", rp->mb->name);
		break;
	case Tminfo:
		snprint(&buf[n], sizeof(buf)-n, "\t%s %s", rp->mb->name, rp->rmb->name);
		break;
	case Tmx:
		snprint(&buf[n], sizeof(buf)-n, "\t%lud %s", rp->pref, rp->host->name);
		break;
	case Ta:
		snprint(&buf[n], sizeof(buf)-n, "\t%s", rp->ip->name);
		break;
	case Tptr:
		snprint(&buf[n], sizeof(buf)-n, "\t%s", rp->ptr->name);
		break;
	case Tsoa:
		snprint(&buf[n], sizeof(buf)-n, "\t%s %s %lud %lud %lud %lud %lud", rp->host->name,
			rp->rmb->name, rp->soa->serial, rp->soa->refresh, rp->soa->retry,
			rp->soa->expire, rp->soa->minttl);
		break;
	case Ttxt:
		snprint(&buf[n], sizeof(buf)-n, "\t%s", rp->txt->name);
		break;
	case Trp:
		snprint(&buf[n], sizeof(buf)-n, "\t%s %s", rp->rmb->name, rp->txt->name);
		break;
	case Tkey:
		snprint(&buf[n], sizeof(buf)-n, "\t%d %d %d", rp->key->flags, rp->key->proto,
			rp->key->alg);
		break;
	case Tsig:
		snprint(&buf[n], sizeof(buf)-n, "\t%d %d %d %lud %lud %lud %d %s",
			rp->sig->type, rp->sig->alg, rp->sig->labels, rp->sig->ttl,
			rp->sig->exp, rp->sig->incep, rp->sig->tag, rp->sig->signer->name);
		break;
	case Tcert:
		snprint(&buf[n], sizeof(buf)-n, "\t%d %d %d",
			rp->sig->type, rp->sig->tag, rp->sig->alg);
		break;
	default:
		break;
	}
out:
	strconv(buf, f);
	return sizeof(RR*);
}

/*
 *  print conversion for rr records in attribute value form
 */
int
rravconv(va_list *arg, Fconv *f)
{
	RR *rp;
	int n;
	char buf[3*Domlen];

	rp = va_arg(*arg, RR*);
	if(rp == 0){
		strcpy(buf, "<null>");
		goto out;
	}

	if(rp->type == Tptr)
		n = snprint(buf, sizeof(buf), "ptr=%s", rp->owner->name);
	else
		n = snprint(buf, sizeof(buf), "dom=%s", rp->owner->name);

	switch(rp->type){
	case Thinfo:
		snprint(&buf[n], sizeof(buf)-n, " cpu=%s os=%s", rp->cpu->name, rp->os->name);
		break;
	case Tcname:
		snprint(&buf[n], sizeof(buf)-n, " cname=%s", rp->host->name);
		break;
	case Tmb:
	case Tmd:
	case Tmf:
		snprint(&buf[n], sizeof(buf)-n, " mbox=%s", rp->host->name);
		break;
	case Tns:
		snprint(&buf[n], sizeof(buf)-n, " ns=%s", rp->host->name);
		break;
	case Tmg:
	case Tmr:
		snprint(&buf[n], sizeof(buf)-n, " mbox=%s", rp->mb->name);
		break;
	case Tminfo:
		snprint(&buf[n], sizeof(buf)-n, " mbox=%s mbox=%s", rp->mb->name, rp->rmb->name);
		break;
	case Tmx:
		snprint(&buf[n], sizeof(buf)-n, " pref=%lud mx=%s", rp->pref, rp->host->name);
		break;
	case Ta:
		snprint(&buf[n], sizeof(buf)-n, " ip=%s", rp->ip->name);
		break;
	case Tptr:
		snprint(&buf[n], sizeof(buf)-n, " dom=%s", rp->ptr->name);
		break;
	case Tsoa:
		snprint(&buf[n], sizeof(buf)-n, " ns=%s mbox=%s serial=%lud refresh=%lud retry=%lud expire=%lud ttl=%lud",
			rp->host->name, rp->rmb->name, rp->soa->serial,
			rp->soa->refresh, rp->soa->retry,
			rp->soa->expire, rp->soa->minttl);
		break;
	case Ttxt:
		snprint(&buf[n], sizeof(buf)-n, " txt=%s", rp->txt->name);
		break;
	case Trp:
		snprint(&buf[n], sizeof(buf)-n, " rp=%s txt=%s", rp->rmb->name, rp->txt->name);
		break;
	case Tkey:
		snprint(&buf[n], sizeof(buf)-n, " flags=%d proto=%d alg=%d",
			rp->key->flags, rp->key->proto, rp->key->alg);
		break;
	case Tsig:
		snprint(&buf[n], sizeof(buf)-n, " type=%d alg=%d labels=%d ttl=%lud exp=%lud incep=%lud tag=%d signer=%s",
			rp->sig->type, rp->sig->alg, rp->sig->labels, rp->sig->ttl,
			rp->sig->exp, rp->sig->incep, rp->sig->tag, rp->sig->signer->name);
		break;
	case Tcert:
		snprint(&buf[n], sizeof(buf)-n, " type=%d tag=%d alg=%d",
			rp->sig->type, rp->sig->tag, rp->sig->alg);
		break;
	default:
		break;
	}
out:
	strconv(buf, f);
	return sizeof(RR*);

}

/*
 *  case insensitive strcmp
 */
int
cistrcmp(char *s1, char *s2)
{
	unsigned c1, c2;

	for(;;) {
		c1 = tolower(*s1++);
		c2 = tolower(*s2++);
		if(c1 != c2) {
			if(c1 > c2)
				return 1;
			return -1;
		}
		if(c1 == 0)
			return 0;
	}
	return 0;	/* not reached */
}

void
warning(char *fmt, ...)
{
	int n;
	char dnserr[128];
	va_list arg;

	va_start(arg, fmt);
	n = doprint(dnserr, dnserr+sizeof(dnserr), fmt, arg) - dnserr;
	va_end(arg);
	dnserr[n] = 0;
	syslog(1, "dns", dnserr);
}

/*
 *  create a slave process to handle a request to avoid one request blocking
 *  another
 */
void
slave(Request *req)
{
	static int slaveid;

	if(req->isslave)
		return;		/* we're already a slave process */

	/* limit parallelism */
	if(getactivity(req) > Maxactive){
		putactivity();
		return;
	}

	switch(rfork(RFPROC|RFNOTEG|RFMEM|RFNOWAIT)){
	case -1:
		putactivity();
		break;
	case 0:
		req->isslave = 1;
		break;
	default:
		longjmp(req->mret, 1);
	}
}

/*
 *  chasing down double free's
 */
void
dncheck(void *p, int dolock)
{
	int i;
	DN *dp;
	RR *rp;

	if(p != nil){
		dp = p;
		assert(dp->magic == DNmagic);
	}

	if(!testing)
		return;

	if(dolock)
		lock(&dnlock);
	for(i = 0; i < HTLEN; i++)
		for(dp = ht[i]; dp; dp = dp->next){
			assert(dp != p);
			assert(dp->magic == DNmagic);
			for(rp = dp->rr; rp; rp = rp->next){
				assert(rp->magic == RRmagic);
				assert(rp->cached);
				assert(rp->owner == dp);
			}
		}
	if(dolock)
		unlock(&dnlock);
}

static int
rrequiv(RR *r1, RR *r2)
{
	return r1->owner == r2->owner
		&& r1->type == r2->type
		&& r1->arg0 == r2->arg0
		&& r1->arg1 == r2->arg1;
}

void
unique(RR *rp)
{
	RR **l, *nrp;

	for(; rp; rp = rp->next){
		l = &rp->next;
		for(nrp = *l; nrp; nrp = *l){
			if(rrequiv(rp, nrp)){
				*l = nrp->next;
				rrfree(nrp);
			} else
				l = &nrp->next;
		}
	}
}

/*
 *  true if second domain is subsumed by the first
 */
int
subsume(char *higher, char *lower)
{
	int hn, ln;

	ln = strlen(lower);
	hn = strlen(higher);
	if(ln < hn)
		return 0;

	if(cistrcmp(lower + ln - hn, higher) != 0)
		return 0;

	if(ln > hn && hn != 0 && lower[ln - hn - 1] != '.')
		return 0;

	return 1;
}

/*
 *  randomize the order we return items to provide some
 *  load balancing for servers
 */
RR*
randomize(RR *rp)
{
	RR *first, *last, *x;
	ulong n;

	if(rp == nil || rp->next == nil)
		return rp;

	/* just randomize addresses and mx's */
	for(x = rp; x; x = x->next)
		if(x->type != Ta && x->type != Tmx && x->type != Tns)
			return rp;

	n = rand();
	last = first = nil;
	while(rp != nil){
		/* unchain */
		x = rp;
		rp = x->next;
		x->next = nil;

		if(n&1){
			/* add to tail */
			if(last == nil)
				first = x;
			else
				last->next = x;
			last = x;
		} else {
			/* add to head */
			if(last == nil)
				last = x;
			x->next = first;
			first = x;
		}

		/* reroll the dice */
		n >>= 1;
	}
	return first;
}

uchar allmagic[4] = { 0xb, 0xa, 0xb, 0xe };

static void*
allocate(int len)
{
	uchar *p;

	p = mallocz(len+4, 1);
	assert(p != nil);
	memmove(p+len, allmagic, 4);
	return (void*)p;
}

static void
checkallocation(void *x, int len)
{
	uchar *p;

	p = x;
	if(memcmp(&p[len], allmagic, 4) != 0)
		sysfatal("allocation overrun");
}
