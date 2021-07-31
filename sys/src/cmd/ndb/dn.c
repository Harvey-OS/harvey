#include <u.h>
#include <libc.h>
#include <ip.h>
#include <lock.h>
#include "dns.h"

/*
 *  Hash table for domain names.  The hash is based only on the
 *  first element of the domain name.
 */
#define HTLEN (4*1024)
static DN	*ht[HTLEN];

/* time base */
static ulong	now;

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

/*
 *  set up a pipe to use as a lock
 */
void
dninit(void)
{
	fmtinstall('E', eipconv);
	fmtinstall('I', eipconv);
	fmtinstall('R', rrconv);
}

/*
 *  hash for a domain name
 */
static ulong
dnhash(char *name)
{
	ulong hash;
	uchar *val = (uchar*)name;

	for(hash = 0; *val && *val != '.'; val++)
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
	now = time(0);
	lock(&dnlock);
	for(dp = *l; dp; dp = dp->next) {
		if(dp->class == class && cistrcmp(dp->name, name) == 0){
			if(dp->reserved < now)
				dnage(dp);
			dp->reserved = time(0) + 5*Min;
			unlock(&dnlock);
			return dp;
		}
		l = &dp->next;
	}
	if(enter == 0){
		unlock(&dnlock);
		return 0;
	}
	dp = malloc(sizeof(DN));
	if(dp == 0){
		unlock(&dnlock);
		fatal("dnlookup");
	}
	dp->name = strdup(name);
	if(dp->name == 0){
		unlock(&dnlock);
		fatal("dnlookup");
	}
	dp->class = class;
	dp->rr = 0;
	dp->next = 0;
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

	fd = open(file, OWRITE);
	if(fd < 0)
		return;
	lock(&dnlock);
	for(i = 0; i < HTLEN; i++){
		for(dp = ht[i]; dp; dp = dp->next){
			fprint(fd, "%s\n", dp->name);
			for(rp = dp->rr; rp; rp = rp->next)
				fprint(fd, "	%R %c%c %s", rp, rp->auth?'A':'U',
					rp->db?'D':'N', ctime(rp->ttl));
		}
	}
	unlock(&dnlock);
	close(fd);
}

/*
 *  check the age of resource records, dump any that have timed out
 */
void
dnage(DN *dp)
{
	RR **l;
	RR *rp, *next;

	now = time(0);
	l = &dp->rr;
	for(rp = dp->rr; rp; rp = next){
		next = rp->next;
		if(rp->ttl < now && !rp->db){
			rrfree(rp);
			*l = next;
			continue;
		}
		l = &rp->next;
	}
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

	new->ttl += now;
	dp = new->owner;
	new->auth = auth;
	new->next = 0;

	/*
	 *  find first rr of the right type
	 */
	l = &dp->rr;
	for(rp = dp->rr; rp; rp = rp->next){
		if(rp->type == new->type)
			break;
		l = &rp->next;
	}

	/*
	 *  if not authoritative jump over all authoritative entries
	 */
	if(new->auth == 0)
		for(; rp; rp = rp->next){
			if(rp->type != new->type || rp->auth == 0)
				break;
			l = &rp->next;
		}

	/*
	 *  check for a duplicate, use longest ttl
	 */
	for(; rp; rp = rp->next){
		if(rp->type != new->type || rp->auth != new->auth)
			break;
		if(rp->arg0 == new->arg0 && rp->arg1 == new->arg1){
			if(new->ttl > rp->ttl)
				rp->ttl = new->ttl;
			rrfree(new);
			return;
		}
		l = &rp->next;
	}

	/*
	 *  add to chain
	 */
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

	rp = malloc(sizeof(RR));
	if(rp == 0)
		fatal("rralloc");
	memset(rp, 0, sizeof(RR));
	rp->type = type;
	if(type == Tsoa){
		rp->soa = malloc(sizeof(SOA));
		if(rp->soa == 0)
			fatal("rralloc");
	}
	rp->ttl = Day;
	return rp;
}

/*
 *  free a resource record and any related structs
 */
void
rrfree(RR *rp)
{
	if(rp->type == Tsoa)
		if(rp->soa)
			free(rp->soa);
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

/*
 *  lookup a resource record of a particular type and
 *  class attached to a domain name
 */
RR*
rrlookup(DN *dp, int type)
{
	RR *rp;

	for(rp = dp->rr; rp; rp = rp->next){
		if(tsame(type, rp->type))
			break;
	}
	return rp;
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
 *  add resource records to a list, duplicate them if they are not database
 *  RR's and hence from the cache since cache RR's are shared.
 */
RR*
rrcat(RR **start, RR *rp, int type)
{
	RR *next;
	RR *np;
	RR **last;
	SOA *soa;

	last = start;
	while(*last)
		last = &(*last)->next;

	for(;rp && tsame(type, rp->type); rp = next){
		next = rp->next;
		if(rp->db)
			np = rp;
		else {
			np = rralloc(rp->type);
			if(type == Tsoa){
				soa = np->soa;
				*soa = *(rp->soa);
				*np = *rp;
				np->soa = soa;
			} else
				*np = *rp;
		}
		np->next = 0;
		*last = np;
		last = &np->next;
	}

	return *start;
}

/*
 *  print conversion for rr records
 */
int
rrconv(void *v, Fconv *f)
{
	RR *rp;
	int n;
	char buf[3*Domlen];

	rp = *((RR**)v);
	if(rp == 0){
		strcpy(buf, "<null>");
		goto out;
	}

	n = snprint(buf, sizeof(buf), "%s %s", rp->owner->name, rrname(rp->type, buf));

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
		snprint(&buf[n], sizeof(buf)-n, "\t%d %s", rp->pref, rp->host->name);
		break;
	case Ta:
		snprint(&buf[n], sizeof(buf)-n, "\t%s", rp->ip->name);
		break;
	case Tptr:
		snprint(&buf[n], sizeof(buf)-n, "\t%s", rp->ptr->name);
		break;
	case Tsoa:
		snprint(&buf[n], sizeof(buf)-n, "\t%s %s %d %d %d %d %d", rp->host->name,
			rp->rmb->name, rp->soa->serial, rp->soa->refresh, rp->soa->retry,
			rp->soa->expire, rp->soa->minttl);
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
