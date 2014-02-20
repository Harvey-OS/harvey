#include <u.h>
#include <libc.h>
#include <ip.h>
#include <pool.h>
#include <ctype.h>
#include "dns.h"

/*
 *  this comment used to say `our target is 4000 names cached, this should
 *  be larger on large servers'.  dns at Bell Labs starts off with
 *  about 1780 names.
 *
 *  aging corrupts the cache, so raise the trigger to avoid it.
 */
enum {
	Deftarget	= 1<<30,	/* effectively disable aging */
	Minage		= 1<<30,
	Defagefreq	= 1<<30,	/* age names this often (seconds) */

	/* these settings will trigger frequent aging */
//	Deftarget	= 4000,
//	Minage		=  5*60,
//	Defagefreq	= 15*60,	/* age names this often (seconds) */

	Restartmins	= 0,
//	Restartmins	= 600,
};

/*
 *  Hash table for domain names.  The hash is based only on the
 *  first element of the domain name.
 */
DN *ht[HTLEN];

static struct {
	Lock;
	ulong	names;		/* names allocated */
	ulong	oldest;		/* longest we'll leave a name around */
	int	active;
	int	mutex;
	ushort	id;		/* same size as in packet */
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

/* names of response codes */
char *rname[Rmask+1] =
{
[Rok]			"ok",
[Rformat]		"format error",
[Rserver]		"server failure",
[Rname]			"bad name",
[Runimplimented]	"unimplemented",
[Rrefused]		"we don't like you",
[Ryxdomain]		"name should not exist",
[Ryxrrset]		"rr set should not exist",
[Rnxrrset]		"rr set should exist",
[Rnotauth]		"not authorative",
[Rnotzone]		"not in zone",
[Rbadvers]		"bad opt version",
/* [Rbadsig]		"bad signature", */
[Rbadkey]		"bad key",
[Rbadtime]		"bad signature time",
[Rbadmode]		"bad mode",
[Rbadname]		"duplicate key name",
[Rbadalg]		"bad algorithm",
};
unsigned nrname = nelem(rname);

/* names of op codes */
char *opname[] =
{
[Oquery]	"query",
[Oinverse]	"inverse query (retired)",
[Ostatus]	"status",
[Oupdate]	"update",
};

ulong target = Deftarget;
ulong start;
Lock	dnlock;

static ulong agefreq = Defagefreq;

static int rrequiv(RR *r1, RR *r2);
static int sencodefmt(Fmt*);

static void
ding(void*, char *msg)
{
	if(strstr(msg, "alarm") != nil) {
		stats.alarms++;
		noted(NCONT);		/* resume with system call error */
	} else
		noted(NDFLT);		/* die */
}

void
dninit(void)
{
	fmtinstall('E', eipfmt);
	fmtinstall('I', eipfmt);
	fmtinstall('V', eipfmt);
	fmtinstall('R', rrfmt);
	fmtinstall('Q', rravfmt);
	fmtinstall('H', sencodefmt);

	dnvars.oldest = maxage;
	dnvars.names = 0;
	dnvars.id = truerand();	/* don't start with same id every time */

	notify(ding);
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
		hash = hash*13 + tolower(*val)-'a';
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

	if(!enter){
		unlock(&dnlock);
		return 0;
	}
	dnvars.names++;
	dp = emalloc(sizeof(*dp));
	dp->magic = DNmagic;
	dp->name = estrdup(name);
	assert(dp->name != nil);
	dp->class = class;
	dp->rr = 0;
	dp->referenced = now;
	/* add new DN to tail of the hash list.  *l points to last next ptr. */
	dp->next = nil;
	*l = dp;
	unlock(&dnlock);

	return dp;
}

static int
rrsame(RR *rr1, RR *rr2)
{
	return rr1 == rr2 || rr2 && rrequiv(rr1, rr2) &&
		rr1->db == rr2->db && rr1->auth == rr2->auth;
}

static int
rronlist(RR *rp, RR *lp)
{
	for(; lp; lp = lp->next)
		if (rrsame(lp, rp))
			return 1;
	return 0;
}

/*
 * dump the stats
 */
void
dnstats(char *file)
{
	int i, fd;

	fd = create(file, OWRITE, 0666);
	if(fd < 0)
		return;

	qlock(&stats);
	fprint(fd, "# system %s\n", sysname());
	fprint(fd, "# slave procs high-water mark\t%lud\n", stats.slavehiwat);
	fprint(fd, "# queries received by 9p\t%lud\n", stats.qrecvd9p);
	fprint(fd, "# queries received by udp\t%lud\n", stats.qrecvdudp);
	fprint(fd, "# queries answered from memory\t%lud\n", stats.answinmem);
	fprint(fd, "# queries sent by udp\t%lud\n", stats.qsent);
	for (i = 0; i < nelem(stats.under10ths); i++)
		if (stats.under10ths[i] || i == nelem(stats.under10ths) - 1)
			fprint(fd, "# responses arriving within %.1f s.\t%lud\n",
				(double)(i+1)/10, stats.under10ths[i]);
	fprint(fd, "\n# queries sent & timed-out\t%lud\n", stats.tmout);
	fprint(fd, "# cname queries timed-out\t%lud\n", stats.tmoutcname);
	fprint(fd, "# ipv6  queries timed-out\t%lud\n", stats.tmoutv6);
	fprint(fd, "\n# negative answers received\t%lud\n", stats.negans);
	fprint(fd, "# negative answers w Rserver set\t%lud\n", stats.negserver);
	fprint(fd, "# negative answers w bad delegation\t%lud\n",
		stats.negbaddeleg);
	fprint(fd, "# negative answers w bad delegation & no answers\t%lud\n",
		stats.negbdnoans);
	fprint(fd, "# negative answers w no Rname set\t%lud\n", stats.negnorname);
	fprint(fd, "# negative answers cached\t%lud\n", stats.negcached);
	qunlock(&stats);

	lock(&dnlock);
	fprint(fd, "\n# domain names %lud target %lud\n", dnvars.names, target);
	unlock(&dnlock);
	close(fd);
}

/*
 *  dump the cache
 */
void
dndump(char *file)
{
	int i, fd;
	DN *dp;
	RR *rp;

	fd = create(file, OWRITE, 0666);
	if(fd < 0)
		return;

	lock(&dnlock);
	for(i = 0; i < HTLEN; i++)
		for(dp = ht[i]; dp; dp = dp->next){
			fprint(fd, "%s\n", dp->name);
			for(rp = dp->rr; rp; rp = rp->next) {
				fprint(fd, "\t%R %c%c %lud/%lud\n",
					rp, rp->auth? 'A': 'U',
					rp->db? 'D': 'N', rp->expire, rp->ttl);
				if (rronlist(rp, rp->next))
					fprint(fd, "*** duplicate:\n");
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
	RR *rp, *srp;
	int i;

	lock(&dnlock);

	for(i = 0; i < HTLEN; i++)
		for(dp = ht[i]; dp; dp = dp->next){
			srp = rp = dp->rr;
			dp->rr = nil;
			for(; rp != nil; rp = rp->next)
				rp->cached = 0;
			rrfreelist(srp);
		}

	unlock(&dnlock);
}

/*
 *  delete head of *l and free the old head.
 *  call with dnlock held.
 */
static void
rrdelhead(RR **l)
{
	RR *rp;

	if (canlock(&dnlock))
		abort();	/* rrdelhead called with dnlock not held */
	rp = *l;
	if(rp == nil)
		return;
	*l = rp->next;		/* unlink head */
	rp->cached = 0;		/* avoid blowing an assertion in rrfree */
	rrfree(rp);
}

/*
 *  check the age of resource records, free any that have timed out.
 *  call with dnlock held.
 */
void
dnage(DN *dp)
{
	RR **l;
	RR *rp, *next;
	ulong diff;

	if (canlock(&dnlock))
		abort();	/* dnage called with dnlock not held */
	diff = now - dp->referenced;
	if(diff < Reserved || dp->keep)
		return;

	l = &dp->rr;
	for(rp = dp->rr; rp; rp = next){
		assert(rp->magic == RRmagic && rp->cached);
		next = rp->next;
		if(!rp->db && (rp->expire < now || diff > dnvars.oldest))
			rrdelhead(l); /* rp == *l before; *l == rp->next after */
		else
			l = &rp->next;
	}
}

#define MARK(dp)	{ if (dp) (dp)->keep = 1; }

/* mark a domain name and those in its RRs as never to be aged */
void
dnagenever(DN *dp, int dolock)
{
	RR *rp;

	if (dolock)
		lock(&dnlock);

	/* mark all referenced domain names */
	MARK(dp);
	for(rp = dp->rr; rp; rp = rp->next){
		MARK(rp->owner);
		if(rp->negative){
			MARK(rp->negsoaowner);
			continue;
		}
		switch(rp->type){
		case Thinfo:
			MARK(rp->cpu);
			MARK(rp->os);
			break;
		case Ttxt:
			break;
		case Tcname:
		case Tmb:
		case Tmd:
		case Tmf:
		case Tns:
		case Tmx:
		case Tsrv:
			MARK(rp->host);
			break;
		case Tmg:
		case Tmr:
			MARK(rp->mb);
			break;
		case Tminfo:
			MARK(rp->rmb);
			MARK(rp->mb);
			break;
		case Trp:
			MARK(rp->rmb);
			MARK(rp->rp);
			break;
		case Ta:
		case Taaaa:
			MARK(rp->ip);
			break;
		case Tptr:
			MARK(rp->ptr);
			break;
		case Tsoa:
			MARK(rp->host);
			MARK(rp->rmb);
			break;
		}
	}

	if (dolock)
		unlock(&dnlock);
}

/* mark all current domain names as never to be aged */
void
dnageallnever(void)
{
	int i;
	DN *dp;

	lock(&dnlock);

	/* mark all referenced domain names */
	for(i = 0; i < HTLEN; i++)
		for(dp = ht[i]; dp; dp = dp->next)
			dnagenever(dp, 0);

	unlock(&dnlock);

	dnslog("%ld initial domain names; target is %ld", dnvars.names, target);
	if(dnvars.names >= target)
		dnslog("more initial domain names (%ld) than target (%ld)",
			dnvars.names, target);
}

#define REF(dp)	{ if (dp) (dp)->refs++; }

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

	if(dnvars.names < target || (now < nextage && !doit)){
		dnvars.oldest = maxage;
		return;
	}

	if(dnvars.names >= target) {
		dnslog("more names (%lud) than target (%lud)", dnvars.names,
			target);
		dnvars.oldest /= 2;
		if (dnvars.oldest < Minage)
			dnvars.oldest = Minage;		/* don't be silly */
	}
	if (agefreq > dnvars.oldest / 2)
		nextage = now + dnvars.oldest / 2;
	else
		nextage = now + agefreq;

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
					break;
				case Tcname:
				case Tmb:
				case Tmd:
				case Tmf:
				case Tns:
				case Tmx:
				case Tsrv:
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
					REF(rp->rp);
					break;
				case Ta:
				case Taaaa:
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
			if(dp->rr == 0 && dp->refs == 0 && !dp->keep){
				assert(dp->magic == DNmagic);
				*l = dp->next;

				if(dp->name)
					free(dp->name);
				dp->magic = ~dp->magic;
				dnvars.names--;
				memset(dp, 0, sizeof *dp); /* cause trouble */
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

	lock(&dnlock);

	/* time out all database entries */
	for(i = 0; i < HTLEN; i++)
		for(dp = ht[i]; dp; dp = dp->next) {
			dp->keep = 0;
			for(rp = dp->rr; rp; rp = rp->next)
				if(rp->db)
					rp->expire = 0;
		}

	unlock(&dnlock);
}

/*
 *  mark all local db records about my area as authoritative,
 *  time out any others
 */
void
dnauthdb(void)
{
	int i;
	ulong minttl;
	Area *area;
	DN *dp;
	RR *rp;

	lock(&dnlock);

	/* time out all database entries */
	for(i = 0; i < HTLEN; i++)
		for(dp = ht[i]; dp; dp = dp->next){
			area = inmyarea(dp->name);
			for(rp = dp->rr; rp; rp = rp->next)
				if(rp->db){
					if(area){
						minttl = area->soarr->soa->minttl;
						if(rp->ttl < minttl)
							rp->ttl = minttl;
						rp->auth = 1;
					}
					if(rp->expire == 0){
						rp->db = 0;
						dp->referenced = now-Reserved-1;
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
getactivity(Request *req, int recursive)
{
	int rv;

	if(traceactivity)
		dnslog("get: %d active by pid %d from %p",
			dnvars.active, getpid(), getcallerpc(&req));
	lock(&dnvars);
	/*
	 * can't block here if we're already holding one
	 * of the dnvars.active (recursive).  will deadlock.
	 */
	while(!recursive && dnvars.mutex){
		unlock(&dnvars);
		sleep(100);			/* tune; was 200 */
		lock(&dnvars);
	}
	rv = ++dnvars.active;
	now = time(nil);
	nowns = nsec();
	req->id = ++dnvars.id;
	unlock(&dnvars);

	return rv;
}
void
putactivity(int recursive)
{
	static ulong lastclean;

	if(traceactivity)
		dnslog("put: %d active by pid %d",
			dnvars.active, getpid());
	lock(&dnvars);
	dnvars.active--;
	assert(dnvars.active >= 0); /* "dnvars.active %d", dnvars.active */

	/*
	 *  clean out old entries and check for new db periodicly
	 *  can't block here if being called to let go a "recursive" lock
	 *  or we'll deadlock waiting for ourselves to give up the dnvars.active.
	 */
	if (recursive || dnvars.mutex ||
	    (needrefresh == 0 && dnvars.active > 0)){
		unlock(&dnvars);
		return;
	}

	/* wait till we're alone */
	dnvars.mutex = 1;
	while(dnvars.active > 0){
		unlock(&dnvars);
		sleep(100);		/* tune; was 100 */
		lock(&dnvars);
	}
	unlock(&dnvars);

	db2cache(needrefresh);

	/* if we've been running for long enough, restart */
	if(start == 0)
		start = time(nil);
	if(Restartmins > 0 && time(nil) - start > Restartmins*60){
		dnslog("killing all dns procs for timed restart");
		postnote(PNGROUP, getpid(), "die");
		dnvars.mutex = 0;
		exits("restart");
	}

	dnageall(0);

	/* let others back in */
	lastclean = now;
	needrefresh = 0;
	dnvars.mutex = 0;
}

int
rrlistlen(RR *rp)
{
	int n;

	n = 0;
	for(; rp; rp = rp->next)
		++n;
	return n;
}

/*
 *  Attach a single resource record to a domain name (new->owner).
 *	- Avoid duplicates with already present RR's
 *	- Chain all RR's of the same type adjacent to one another
 *	- chain authoritative RR's ahead of non-authoritative ones
 *	- remove any expired RR's
 *  If new is a stale duplicate, rrfree it.
 *  Must be called with dnlock held.
 */
static void
rrattach1(RR *new, int auth)
{
	RR **l;
	RR *rp;
	DN *dp;

	assert(new->magic == RRmagic && !new->cached);

//	dnslog("rrattach1: %s", new->owner->name);
	if(!new->db) {
		/*
		 * try not to let responses expire before we
		 * can use them to complete this query, by extending
		 * past (or nearly past) expiration time.
		 */
		new->expire = new->ttl > now + Min? new->ttl: now + 10*Min;
	} else
		new->expire = now + Year;
	dp = new->owner;
	assert(dp->magic == DNmagic);
	new->auth |= auth;
	new->next = 0;

	/*
	 *  find first rr of the right type
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
	 *
	 *  look farther ahead than just the next entry when looking
	 *  for duplicates; RRs of a given type can have different rdata
	 *  fields (e.g. multiple NS servers).
	 */
	while ((rp = *l) != nil){
		assert(rp->magic == RRmagic && rp->cached);
		if(rp->type != new->type)
			break;

		if(rp->db == new->db && rp->auth == new->auth){
			/* negative drives out positive and vice versa */
			if(rp->negative != new->negative) {
				/* rp == *l before; *l == rp->next after */
				rrdelhead(l);
				continue;	
			}
			/* all things equal, pick the newer one */
			else if(rp->arg0 == new->arg0 && rp->arg1 == new->arg1){
				/* new drives out old */
				if (new->ttl <= rp->ttl &&
				    new->expire <= rp->expire) {
					rrfree(new);
					return;
				}
				/* rp == *l before; *l == rp->next after */
				rrdelhead(l);
				continue;
			}
			/*
			 *  Hack for pointer records.  This makes sure
			 *  the ordering in the list reflects the ordering
			 *  received or read from the database
			 */
			else if(rp->type == Tptr &&
			    !rp->negative && !new->negative &&
			    rp->ptr->ordinal > new->ptr->ordinal)
				break;
		}
		l = &rp->next;
	}

	if (rronlist(new, rp)) {
		/* should not happen; duplicates were processed above */
		dnslog("adding duplicate %R to list of %R; aborting", new, rp);
		abort();
	}
	/*
	 *  add to chain
	 */
	new->cached = 1;
	new->next = rp;
	*l = new;
}

/*
 *  Attach a list of resource records to a domain name.
 *  May rrfree any stale duplicate RRs; dismembers the list.
 *  Upon return, every RR in the list will have been rrfree-d
 *  or attached to its domain name.
 *  See rrattach1 for properties preserved.
 */
void
rrattach(RR *rp, int auth)
{
	RR *next, *tp;
	DN *dp;

	lock(&dnlock);
	for(; rp; rp = next){
		next = rp->next;
		rp->next = nil;
		dp = rp->owner;

//		dnslog("rrattach: %s", rp->owner->name);
		/* avoid any outside spoofing; leave keepers alone */
		if(cfg.cachedb && !rp->db && inmyarea(rp->owner->name)
//		    || dp->keep			/* TODO: make this work */
		    )
			rrfree(rp);
		else {
			/* ameliorate the memory leak (someday delete this) */
			if (0 && rrlistlen(dp->rr) > 50 && !dp->keep) {
				dnslog("rrattach(%s): rr list too long; "
					"freeing it", dp->name);
				tp = dp->rr;
				dp->rr = nil;
				rrfreelist(tp);
			} else
				USED(dp);
			rrattach1(rp, auth);
		}
	}
	unlock(&dnlock);
}

/* should be called with dnlock held */
RR**
rrcopy(RR *rp, RR **last)
{
	Cert *cert;
	Key *key;
	Null *null;
	RR *nrp;
	SOA *soa;
	Sig *sig;
	Txt *t, *nt, **l;

	if (canlock(&dnlock))
		abort();	/* rrcopy called with dnlock not held */
	nrp = rralloc(rp->type);
	setmalloctag(nrp, getcallerpc(&rp));
	switch(rp->type){
	case Ttxt:
		*nrp = *rp;
		l = &nrp->txt;
		*l = nil;
		for(t = rp->txt; t != nil; t = t->next){
			nt = emalloc(sizeof(*nt));
			nt->p = estrdup(t->p);
			nt->next = nil;
			*l = nt;
			l = &nt->next;
		}
		break;
	case Tsoa:
		soa = nrp->soa;
		*nrp = *rp;
		nrp->soa = soa;
		*nrp->soa = *rp->soa;
		nrp->soa->slaves = copyserverlist(rp->soa->slaves);
		break;
	case Tsrv:
		*nrp = *rp;
		nrp->srv = emalloc(sizeof *nrp->srv);
		*nrp->srv = *rp->srv;
		break;
	case Tkey:
		key = nrp->key;
		*nrp = *rp;
		nrp->key = key;
		*key = *rp->key;
		key->data = emalloc(key->dlen);
		memmove(key->data, rp->key->data, rp->key->dlen);
		break;
	case Tsig:
		sig = nrp->sig;
		*nrp = *rp;
		nrp->sig = sig;
		*sig = *rp->sig;
		sig->data = emalloc(sig->dlen);
		memmove(sig->data, rp->sig->data, rp->sig->dlen);
		break;
	case Tcert:
		cert = nrp->cert;
		*nrp = *rp;
		nrp->cert = cert;
		*cert = *rp->cert;
		cert->data = emalloc(cert->dlen);
		memmove(cert->data, rp->cert->data, rp->cert->dlen);
		break;
	case Tnull:
		null = nrp->null;
		*nrp = *rp;
		nrp->null = null;
		*null = *rp->null;
		null->data = emalloc(null->dlen);
		memmove(null->data, rp->null->data, rp->null->dlen);
		break;
	default:
		*nrp = *rp;
		break;
	}
	nrp->cached = 0;
	nrp->next = 0;
	*last = nrp;
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
		assert(rp->magic == RRmagic);
		assert(rp->cached);
		if(rp->db)
		if(rp->auth)
		if(tsame(type, rp->type)) {
			last = rrcopy(rp, last);
			// setmalloctag(*last, getcallerpc(&dp));
		}
	}
	if(first)
		goto out;

	/* try for a living authoritative network entry */
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

	/* try for a living unauthoritative network entry */
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

	/* otherwise, settle for anything we got (except for negative caches) */
	for(rp = dp->rr; rp; rp = rp->next)
		if(tsame(type, rp->type)){
			if(rp->negative)
				goto out;
			last = rrcopy(rp, last);
		}

out:
	unique(first);
	unlock(&dnlock);
//	dnslog("rrlookup(%s) -> %#p\t# in-core only", dp->name, first);
//	if (first)
//		setmalloctag(first, getcallerpc(&dp));
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

	/* make any a synonym for all */
	if(strcmp(atype, "any") == 0)
		return Tall;
	else if(isascii(atype[0]) && isdigit(atype[0]))
		return atoi(atype);
	else
		return -1;
}

/*
 *  return 0 if not a supported rr type
 */
int
rrsupported(int type)
{
	if(type < 0 || type >Tall)
		return 0;
	return rrtname[type] != nil;
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
 *  RR's since these are shared.  should be called with dnlock held
 *  to avoid racing down the start chain.
 */
RR*
rrcat(RR **start, RR *rp)
{
	RR *olp, *nlp;
	RR **last;

	if (canlock(&dnlock))
		abort();	/* rrcat called with dnlock not held */
	/* check for duplicates */
	for (olp = *start; 0 && olp; olp = olp->next)
		for (nlp = rp; nlp; nlp = nlp->next)
			if (rrsame(nlp, olp))
				dnslog("rrcat: duplicate RR: %R", nlp);
	USED(olp);

	last = start;
	while(*last != nil)
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

	if (canlock(&dnlock))
		abort();	/* rrremneg called with dnlock not held */
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
	RR *first, *rp;
	RR **nl;

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

static char *
dnname(DN *dn)
{
	return dn? dn->name: "<null>";
}

/*
 *  print conversion for rr records
 */
int
rrfmt(Fmt *f)
{
	int rv;
	char *strp;
	char buf[Domlen];
	Fmt fstr;
	RR *rp;
	Server *s;
	SOA *soa;
	Srv *srv;
	Txt *t;

	fmtstrinit(&fstr);

	rp = va_arg(f->args, RR*);
	if(rp == nil){
		fmtprint(&fstr, "<null>");
		goto out;
	}

	fmtprint(&fstr, "%s %s", dnname(rp->owner),
		rrname(rp->type, buf, sizeof buf));

	if(rp->negative){
		fmtprint(&fstr, "\tnegative - rcode %d", rp->negrcode);
		goto out;
	}

	switch(rp->type){
	case Thinfo:
		fmtprint(&fstr, "\t%s %s", dnname(rp->cpu), dnname(rp->os));
		break;
	case Tcname:
	case Tmb:
	case Tmd:
	case Tmf:
	case Tns:
		fmtprint(&fstr, "\t%s", dnname(rp->host));
		break;
	case Tmg:
	case Tmr:
		fmtprint(&fstr, "\t%s", dnname(rp->mb));
		break;
	case Tminfo:
		fmtprint(&fstr, "\t%s %s", dnname(rp->mb), dnname(rp->rmb));
		break;
	case Tmx:
		fmtprint(&fstr, "\t%lud %s", rp->pref, dnname(rp->host));
		break;
	case Ta:
	case Taaaa:
		fmtprint(&fstr, "\t%s", dnname(rp->ip));
		break;
	case Tptr:
//		fmtprint(&fstr, "\t%s(%lud)", dnname(rp->ptr),
//			rp->ptr? rp->ptr->ordinal: "<null>");
		fmtprint(&fstr, "\t%s", dnname(rp->ptr));
		break;
	case Tsoa:
		soa = rp->soa;
		fmtprint(&fstr, "\t%s %s %lud %lud %lud %lud %lud",
			dnname(rp->host), dnname(rp->rmb),
			(soa? soa->serial: 0),
			(soa? soa->refresh: 0), (soa? soa->retry: 0),
			(soa? soa->expire: 0), (soa? soa->minttl: 0));
		if (soa)
			for(s = soa->slaves; s != nil; s = s->next)
				fmtprint(&fstr, " %s", s->name);
		break;
	case Tsrv:
		srv = rp->srv;
		fmtprint(&fstr, "\t%ud %ud %ud %s",
			(srv? srv->pri: 0), (srv? srv->weight: 0),
			rp->port, dnname(rp->host));
		break;
	case Tnull:
		if (rp->null == nil)
			fmtprint(&fstr, "\t<null>");
		else
			fmtprint(&fstr, "\t%.*H", rp->null->dlen,
				rp->null->data);
		break;
	case Ttxt:
		fmtprint(&fstr, "\t");
		for(t = rp->txt; t != nil; t = t->next)
			fmtprint(&fstr, "%s", t->p);
		break;
	case Trp:
		fmtprint(&fstr, "\t%s %s", dnname(rp->rmb), dnname(rp->rp));
		break;
	case Tkey:
		if (rp->key == nil)
			fmtprint(&fstr, "\t<null> <null> <null>");
		else
			fmtprint(&fstr, "\t%d %d %d", rp->key->flags,
				rp->key->proto, rp->key->alg);
		break;
	case Tsig:
		if (rp->sig == nil)
			fmtprint(&fstr,
		   "\t<null> <null> <null> <null> <null> <null> <null> <null>");
		else
			fmtprint(&fstr, "\t%d %d %d %lud %lud %lud %d %s",
				rp->sig->type, rp->sig->alg, rp->sig->labels,
				rp->sig->ttl, rp->sig->exp, rp->sig->incep,
				rp->sig->tag, dnname(rp->sig->signer));
		break;
	case Tcert:
		if (rp->cert == nil)
			fmtprint(&fstr, "\t<null> <null> <null>");
		else
			fmtprint(&fstr, "\t%d %d %d",
				rp->cert->type, rp->cert->tag, rp->cert->alg);
		break;
	}
out:
	strp = fmtstrflush(&fstr);
	rv = fmtstrcpy(f, strp);
	free(strp);
	return rv;
}

/*
 *  print conversion for rr records in attribute value form
 */
int
rravfmt(Fmt *f)
{
	int rv, quote;
	char *strp;
	Fmt fstr;
	RR *rp;
	Server *s;
	SOA *soa;
	Srv *srv;
	Txt *t;

	fmtstrinit(&fstr);

	rp = va_arg(f->args, RR*);
	if(rp == nil){
		fmtprint(&fstr, "<null>");
		goto out;
	}

	if(rp->type == Tptr)
		fmtprint(&fstr, "ptr=%s", dnname(rp->owner));
	else
		fmtprint(&fstr, "dom=%s", dnname(rp->owner));

	switch(rp->type){
	case Thinfo:
		fmtprint(&fstr, " cpu=%s os=%s",
			dnname(rp->cpu), dnname(rp->os));
		break;
	case Tcname:
		fmtprint(&fstr, " cname=%s", dnname(rp->host));
		break;
	case Tmb:
	case Tmd:
	case Tmf:
		fmtprint(&fstr, " mbox=%s", dnname(rp->host));
		break;
	case Tns:
		fmtprint(&fstr,  " ns=%s", dnname(rp->host));
		break;
	case Tmg:
	case Tmr:
		fmtprint(&fstr, " mbox=%s", dnname(rp->mb));
		break;
	case Tminfo:
		fmtprint(&fstr, " mbox=%s mbox=%s",
			dnname(rp->mb), dnname(rp->rmb));
		break;
	case Tmx:
		fmtprint(&fstr, " pref=%lud mx=%s", rp->pref, dnname(rp->host));
		break;
	case Ta:
	case Taaaa:
		fmtprint(&fstr, " ip=%s", dnname(rp->ip));
		break;
	case Tptr:
		fmtprint(&fstr, " dom=%s", dnname(rp->ptr));
		break;
	case Tsoa:
		soa = rp->soa;
		fmtprint(&fstr,
" ns=%s mbox=%s serial=%lud refresh=%lud retry=%lud expire=%lud ttl=%lud",
			dnname(rp->host), dnname(rp->rmb),
			(soa? soa->serial: 0),
			(soa? soa->refresh: 0), (soa? soa->retry: 0),
			(soa? soa->expire: 0), (soa? soa->minttl: 0));
		for(s = soa->slaves; s != nil; s = s->next)
			fmtprint(&fstr, " dnsslave=%s", s->name);
		break;
	case Tsrv:
		srv = rp->srv;
		fmtprint(&fstr, " pri=%ud weight=%ud port=%ud target=%s",
			(srv? srv->pri: 0), (srv? srv->weight: 0),
			rp->port, dnname(rp->host));
		break;
	case Tnull:
		if (rp->null == nil)
			fmtprint(&fstr, " null=<null>");
		else
			fmtprint(&fstr, " null=%.*H", rp->null->dlen,
				rp->null->data);
		break;
	case Ttxt:
		fmtprint(&fstr, " txt=");
		quote = 0;
		for(t = rp->txt; t != nil; t = t->next)
			if(strchr(t->p, ' '))
				quote = 1;
		if(quote)
			fmtprint(&fstr, "\"");
		for(t = rp->txt; t != nil; t = t->next)
			fmtprint(&fstr, "%s", t->p);
		if(quote)
			fmtprint(&fstr, "\"");
		break;
	case Trp:
		fmtprint(&fstr, " rp=%s txt=%s",
			dnname(rp->rmb), dnname(rp->rp));
		break;
	case Tkey:
		if (rp->key == nil)
			fmtprint(&fstr, " flags=<null> proto=<null> alg=<null>");
		else
			fmtprint(&fstr, " flags=%d proto=%d alg=%d",
				rp->key->flags, rp->key->proto, rp->key->alg);
		break;
	case Tsig:
		if (rp->sig == nil)
			fmtprint(&fstr,
" type=<null> alg=<null> labels=<null> ttl=<null> exp=<null> incep=<null> tag=<null> signer=<null>");
		else
			fmtprint(&fstr,
" type=%d alg=%d labels=%d ttl=%lud exp=%lud incep=%lud tag=%d signer=%s",
				rp->sig->type, rp->sig->alg, rp->sig->labels,
				rp->sig->ttl, rp->sig->exp, rp->sig->incep,
				rp->sig->tag, dnname(rp->sig->signer));
		break;
	case Tcert:
		if (rp->cert == nil)
			fmtprint(&fstr, " type=<null> tag=<null> alg=<null>");
		else
			fmtprint(&fstr, " type=%d tag=%d alg=%d",
				rp->cert->type, rp->cert->tag, rp->cert->alg);
		break;
	}
out:
	strp = fmtstrflush(&fstr);
	rv = fmtstrcpy(f, strp);
	free(strp);
	return rv;
}

void
warning(char *fmt, ...)
{
	char dnserr[256];
	va_list arg;

	va_start(arg, fmt);
	vseprint(dnserr, dnserr+sizeof(dnserr), fmt, arg);
	va_end(arg);
	syslog(1, logfile, dnserr);		/* on console too */
}

void
dnslog(char *fmt, ...)
{
	char dnserr[256];
	va_list arg;

	va_start(arg, fmt);
	vseprint(dnserr, dnserr+sizeof(dnserr), fmt, arg);
	va_end(arg);
	syslog(0, logfile, dnserr);
}

/*
 * based on libthread's threadsetname, but drags in less library code.
 * actually just sets the arguments displayed.
 */
void
procsetname(char *fmt, ...)
{
	int fd;
	char *cmdname;
	char buf[128];
	va_list arg;

	va_start(arg, fmt);
	cmdname = vsmprint(fmt, arg);
	va_end(arg);
	if (cmdname == nil)
		return;
	snprint(buf, sizeof buf, "#p/%d/args", getpid());
	if((fd = open(buf, OWRITE)) >= 0){
		write(fd, cmdname, strlen(cmdname)+1);
		close(fd);
	}
	free(cmdname);
}

/*
 *  create a slave process to handle a request to avoid one request blocking
 *  another
 */
void
slave(Request *req)
{
	int ppid, procs;

	if(req->isslave)
		return;		/* we're already a slave process */

	/*
	 * These calls to putactivity cannot block.
	 * After getactivity(), the current process is counted
	 * twice in dnvars.active (one will pass to the child).
	 * If putactivity tries to wait for dnvars.active == 0,
	 * it will never happen.
	 */

	/* limit parallelism */
	procs = getactivity(req, 1);
	if (procs > stats.slavehiwat)
		stats.slavehiwat = procs;
	if(procs > Maxactive){
		if(traceactivity)
			dnslog("[%d] too much activity", getpid());
		putactivity(1);
		return;
	}

	/*
	 * parent returns to main loop, child does the work.
	 * don't change note group.
	 */
	ppid = getpid();
	switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
	case -1:
		putactivity(1);
		break;
	case 0:
		procsetname("request slave of pid %d", ppid);
 		if(traceactivity)
			dnslog("[%d] take activity from %d", getpid(), ppid);
		req->isslave = 1;	/* why not `= getpid()'? */
		break;
	default:
		/*
		 * this relies on rfork producing separate, initially-identical
		 * stacks, thus giving us two copies of `req', one in each
		 * process.
		 */
		alarm(0);
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
	poolcheck(mainmem);
	for(i = 0; i < HTLEN; i++)
		for(dp = ht[i]; dp; dp = dp->next){
			assert(dp != p);
			assert(dp->magic == DNmagic);
			for(rp = dp->rr; rp; rp = rp->next){
				assert(rp->magic == RRmagic);
				assert(rp->cached);
				assert(rp->owner == dp);
				/* also check for duplicate rrs */
				if (dolock && rronlist(rp, rp->next)) {
					dnslog("%R duplicates its next chain "
						"(%R); aborting", rp, rp->next);
					abort();
				}
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

/* called with dnlock held */
void
unique(RR *rp)
{
	RR **l, *nrp;

	for(; rp; rp = rp->next){
		l = &rp->next;
		for(nrp = *l; nrp; nrp = *l)
			if(rrequiv(rp, nrp)){
				*l = nrp->next;
				rrfree(nrp);
			} else
				l = &nrp->next;
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
	if (ln < hn || cistrcmp(lower + ln - hn, higher) != 0 ||
	    ln > hn && hn != 0 && lower[ln - hn - 1] != '.')
		return 0;
	return 1;
}

/*
 *  randomize the order we return items to provide some
 *  load balancing for servers.
 *
 *  only randomize the first class of entries
 */
RR*
randomize(RR *rp)
{
	RR *first, *last, *x, *base;
	ulong n;

	if(rp == nil || rp->next == nil)
		return rp;

	/* just randomize addresses, mx's and ns's */
	for(x = rp; x; x = x->next)
		if(x->type != Ta && x->type != Taaaa &&
		    x->type != Tmx && x->type != Tns)
			return rp;

	base = rp;

	n = rand();
	last = first = nil;
	while(rp != nil){
		/* stop randomizing if we've moved past our class */
		if(base->auth != rp->auth || base->db != rp->db){
			last->next = rp;
			break;
		}

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

static int
sencodefmt(Fmt *f)
{
	int i, len, ilen, rv;
	char *out, *buf;
	uchar *b;
	char obuf[64];		/* rsc optimization */

	if(!(f->flags&FmtPrec) || f->prec < 1)
		goto error;

	b = va_arg(f->args, uchar*);
	if(b == nil)
		goto error;

	/* if it's a printable, go for it */
	len = f->prec;
	for(i = 0; i < len; i++)
		if(!isprint(b[i]))
			break;
	if(i == len){
		if(len >= sizeof obuf)
			len = sizeof(obuf)-1;
		memmove(obuf, b, len);
		obuf[len] = 0;
		fmtstrcpy(f, obuf);
		return 0;
	}

	ilen = f->prec;
	f->prec = 0;
	f->flags &= ~FmtPrec;
	switch(f->r){
	case '<':
		len = (8*ilen+4)/5 + 3;
		break;
	case '[':
		len = (8*ilen+5)/6 + 4;
		break;
	case 'H':
		len = 2*ilen + 1;
		break;
	default:
		goto error;
	}

	if(len > sizeof(obuf)){
		buf = malloc(len);
		if(buf == nil)
			goto error;
	} else
		buf = obuf;

	/* convert */
	out = buf;
	switch(f->r){
	case '<':
		rv = enc32(out, len, b, ilen);
		break;
	case '[':
		rv = enc64(out, len, b, ilen);
		break;
	case 'H':
		rv = enc16(out, len, b, ilen);
		break;
	default:
		rv = -1;
		break;
	}
	if(rv < 0)
		goto error;

	fmtstrcpy(f, buf);
	if(buf != obuf)
		free(buf);
	return 0;

error:
	return fmtstrcpy(f, "<encodefmt>");
}

void*
emalloc(int size)
{
	char *x;

	x = mallocz(size, 1);
	if(x == nil)
		abort();
	setmalloctag(x, getcallerpc(&size));
	return x;
}

char*
estrdup(char *s)
{
	int size;
	char *p;

	size = strlen(s)+1;
	p = mallocz(size, 0);
	if(p == nil)
		abort();
	memmove(p, s, size);
	setmalloctag(p, getcallerpc(&s));
	return p;
}

/*
 *  create a pointer record
 */
static RR*
mkptr(DN *dp, char *ptr, ulong ttl)
{
	DN *ipdp;
	RR *rp;

	ipdp = dnlookup(ptr, Cin, 1);

	rp = rralloc(Tptr);
	rp->ptr = dp;
	rp->owner = ipdp;
	rp->db = 1;
	if(ttl)
		rp->ttl = ttl;
	return rp;
}

void	bytes2nibbles(uchar *nibbles, uchar *bytes, int nbytes);

/*
 *  look for all ip addresses in this network and make
 *  pointer records for them.
 */
void
dnptr(uchar *net, uchar *mask, char *dom, int forwtype, int subdoms, int ttl)
{
	int i, j, len;
	char *p, *e;
	char ptr[Domlen];
	uchar *ipp;
	uchar ip[IPaddrlen], nnet[IPaddrlen];
	uchar nibip[IPaddrlen*2];
	DN *dp;
	RR *rp, *nrp, *first, **l;

	l = &first;
	first = nil;
	for(i = 0; i < HTLEN; i++)
		for(dp = ht[i]; dp; dp = dp->next)
			for(rp = dp->rr; rp; rp = rp->next){
				if(rp->type != forwtype || rp->negative)
					continue;
				parseip(ip, rp->ip->name);
				maskip(ip, mask, nnet);
				if(ipcmp(net, nnet) != 0)
					continue;

				ipp = ip;
				len = IPaddrlen;
				if (forwtype == Taaaa) {
					bytes2nibbles(nibip, ip, IPaddrlen);
					ipp = nibip;
					len = 2*IPaddrlen;
				}

				p = ptr;
				e = ptr+sizeof(ptr);
				for(j = len - 1; j >= len - subdoms; j--)
					p = seprint(p, e, (forwtype == Ta?
						"%d.": "%x."), ipp[j]);
				seprint(p, e, "%s", dom);

				nrp = mkptr(dp, ptr, ttl);
				*l = nrp;
				l = &nrp->next;
			}

	for(rp = first; rp != nil; rp = nrp){
		nrp = rp->next;
		rp->next = nil;
		rrattach(rp, Authoritative);
	}
}

void
addserver(Server **l, char *name)
{
	Server *s;

	while(*l)
		l = &(*l)->next;
	s = malloc(sizeof(Server)+strlen(name)+1);
	if(s == nil)
		return;
	s->name = (char*)(s+1);
	strcpy(s->name, name);
	s->next = nil;
	*l = s;
}

Server*
copyserverlist(Server *s)
{
	Server *ns;

	for(ns = nil; s != nil; s = s->next)
		addserver(&ns, s->name);
	return ns;
}


/* from here down is copied to ip/snoopy/dns.c periodically to update it */

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
	if (rp->type != type)
		dnslog("rralloc: bogus type %d", type);
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

	assert(rp->magic == RRmagic);
	assert(!rp->cached);

	/* our callers often hold dnlock.  it's needed to examine dp safely. */
	dp = rp->owner;
	if(dp){
		/* if someone else holds dnlock, skip the sanity check. */
		if (canlock(&dnlock)) {
			assert(dp->magic == DNmagic);
			for(nrp = dp->rr; nrp; nrp = nrp->next)
				assert(nrp != rp);   /* "rrfree of live rr" */
			unlock(&dnlock);
		}
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
