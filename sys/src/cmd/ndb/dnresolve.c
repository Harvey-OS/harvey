#include <u.h>
#include <libc.h>
#include "dns.h"
#include "ip.h"

enum
{
	Maxdest=	32,	/* maximum destinations for a request message */
	Maxtrans=	3,	/* maximum transmissions to a server */
};

typedef struct Dest	Dest;
struct Dest
{
	uchar	a[4];	/* ip address */
	DN	*s;	/* name server */
	int	nx;	/* number of transmissions */
};

static ulong reqno;	/* request id */

static int	netquery(DN*, int, RR*, Request*);
static RR*	dnresolve1(char*, int, int, Request*);

/*
 *  lookup 'type' info for domain name 'name'.  If it doesn't exist, try
 *  looking it up as a canonical name.
 */
RR*
dnresolve(char *name, int class, int type, Request *req, RR **cn)
{
	RR *rp;
	DN *dp;

	/* try the name directly */
	rp = dnresolve1(name, class, type, req);
	if(rp)
		return rp;

	/* try it as a canonical name */
	rp = dnresolve1(name, class, Tcname, req);
	if(rp == 0)
		return 0;
	if(rp && cn)
		*cn = rp;
	dp = rp->host;
	return dnresolve1(dp->name, class, type, req);
}

static RR*
dnresolve1(char *name, int class, int type, Request *req)
{
	DN *dp, *nsdp;
	RR *rp, *nsrp;
	char *cp;

	/* only class Cin implemented so far */
	if(class != Cin)
		return 0;

	dp = dnlookup(name, class, 1);

	/* first try the cache */
	rp = rrlookup(dp, type);
	if(rp)
		return rp;

	/* in-addr.arpa queries are special */
	if(type == Tptr){
		rp = dbinaddr(dp);
		if(rp)
			return rp;
	}

	/*
 	 *  walk up the domain name looking for
	 *  a name server for the domain.
	 */
	for(cp = name; cp; cp = walkup(cp)){
		/* look for ns in cache and database */
		nsdp = dnlookup(cp, class, 0);
		nsrp = 0;
		if(nsdp)
			nsrp = rrlookup(nsdp, Tns);
		if(nsrp == 0)
			nsrp = dblookup(cp, class, Tns, 0);

		if(nsrp){
			/* local domains resolved from this db */
			if(nsrp->local){
				if(nsrp->db)	/* free database rr's */
					rrfreelist(nsrp);
				return dblookup(name, class, type, 1);
			}

			/* try the name servers */
			if(netquery(dp, type, nsrp, req)){
				/* we got an answer */
				if(nsrp->db)	/* free database rr's */
					rrfreelist(nsrp);
				return rrlookup(dp, type);
			}
		}
	}

	/* noone answered */
	return 0;
}

/*
 *  walk a domain name one element to the right.  return a pointer to that element.
 *  in other words, return a pointer to the parent domain name.
 */
char*
walkup(char *name)
{
	char *cp;

	cp = strchr(name, '.');
	if(cp)
		return cp+1;
	else if(*name)
		return "";
	else
		return 0;
}

/*
 *  Get a udpport for requests and replies.  Put the port
 *  into "headers" mode.
 */
static char *hmsg = "headers";

static int
udpport(void)
{
	int fd, ctl;

	/* get a udp port */
	fd = dial("udp!0.0.0.0!0", 0, 0, &ctl);
	if(fd < 0){
		warning("can't get udp port");
		return -1;
	}

	/* turn on header style interface */
	if(write(ctl, hmsg, strlen(hmsg)) , 0){
		warning(hmsg);
		return -1;
	}

	close(ctl);
	return fd;
}

static int
mkreq(DN *dp, int type, uchar *buf, int reqno)
{
	DNSmsg m;
	int len;

	/* stuff port number into output buffer */
	buf[4] = 0;
	buf[5] = 53;

	/* make request and convert it to output format */
	memset(&m, 0, sizeof(m));
	m.flags = 0;
	m.id = reqno;
	m.qd = rralloc(type);
	m.qd->owner = dp;
	m.qd->type = type;
	len = convDNS2M(&m, &buf[Udphdrlen], Maxudp);
	if(len < 0)
		fatal("can't convert");
	return len;
}

/*
 *  read replies to a request.  ignore any of the wrong type.
 */
static int
readreq(int fd, DN *dp, int type, int req, uchar *ibuf, DNSmsg *mp)
{
	char *err;
	int len;

	for(;;){
		len = read(fd, ibuf, Udphdrlen+Maxudp);
		len -= Udphdrlen;
		if(len < 0)
			return -1;	/* timed out */
		
		/* convert into internal format  */
		err = convM2DNS(&ibuf[Udphdrlen], len, mp);
		if(err){
			syslog(0, "dns", "input err %s", err);
			continue;
		}

		/* answering the right question? */
		if(mp->id != req){
			syslog(0, "dns", "id %d instead of %d", mp->id, req);
			continue;
		}
		if(mp->qd == 0){
			syslog(0, "dns", "no question RR");
			continue;
		}
		if(mp->qd->owner != dp){
			syslog(0, "dns", "owner %s instead of %s", mp->qd->owner->name,
				dp->name);
			continue;
		}
		if(mp->qd->type != type){
			syslog(0, "dns", "type %d instead of %d", mp->qd->type, type);
			continue;
		}
		return 0;
	}

	return 0;	/* never reached */
}

/*
 *  query name servers.  If the name server returns a pointer to another
 *  name server, recurse.
 */
static void
ding(void *x, char *msg)
{
	USED(x);
	if(strcmp(msg, "alarm") == 0)
		noted(NCONT);
	else
		noted(NDFLT);
}
static int
netquery(DN *dp, int type, RR *nsrp, Request *reqp)
{
	int fd, i, j, len;
	ulong req;
	RR *rp;
	Dest *p, *l;
	DN *ndp;
	Dest dest[Maxdest];
	DNSmsg m;
	uchar obuf[Maxudp+Udphdrlen];
	uchar ibuf[Maxudp+Udphdrlen];

	slave(reqp);

	/* get the addresses */
	l = dest;
	for(; nsrp && nsrp->type == Tns; nsrp = nsrp->next){
		rp = rrlookup(nsrp->host, Ta);
		if(rp == 0)
			rp = dblookup(nsrp->host->name, Cin, Ta, 0);
		for(; rp && rp->type == Ta; rp = rp->next){
			if(l >= &dest[Maxdest])
				break;
			parseip(l->a, rp->ip->name);
			l->nx = 0;
			l->s = nsrp->host;
			l++;
		}
	}

	/* pack request into a message */
	req = reqno++;
	len = mkreq(dp, type, obuf, req);

	/*
	 *  transmit requests and wait for answers.
	 *  at most 3 attempts to each address.
	 *  each cycle send one more message than the previous.
	 */
	fd = udpport();
	if(fd < 0)
		return 0;
	notify(ding);
	for(i = 1;; i++){
		/* send to i destinations */
		p = dest;
		for(j = 0; j < i; j++){
			/* skip destinations we've finished with */
			for(; p < l; p++)
				if(p->nx < Maxtrans)
					break;
			if(p >= l)
				break;

			p->nx++;
			memmove(obuf, p->a, sizeof(p->a));
			if(write(fd, obuf, len + Udphdrlen) < 0)
				warning("sending udp msg %r");
			p++;
		}
		if(j == 0)
			break;		/* no destinations left */

		/* wait a fixed time for replies */
		alarm(1000);
		for(;;){
			if(readreq(fd, dp, type, req, ibuf, &m) < 0)
				break;		/* timed out */

			/* remove all addrs of responding server from list */
			for(p = dest; p < l; p++)
				if(memcmp(p->a, ibuf, sizeof(p->a)) == 0){
					ndp = p->s;
					for(p = dest; p < l; p++)
						if(p->s == ndp)
							p->nx = Maxtrans;
					break;
				}

			/* incorporate answers */
			if(m.an)
				rrattach(m.an, m.flags & Fauth);
			if(m.ar)
				rrattach(m.ar, 0);

			/*
			 *  Any reply from an authoritative server,
			 *  or a positive reply terminates the search
			 */
			if(m.an || (m.flags & Fauth)){
				alarm(0);
				close(fd);
				return 1;
			}

			/*
			 *  if we've been given better name servers
			 *  recurse
			 */
			if(m.ns){
				alarm(0);
				close(fd);
				ndp = m.ns->owner;
				rrattach(m.ns, 0);
				return netquery(dp, type, rrlookup(ndp, Tns), reqp);
			}
		}
		alarm(0);
	}
	alarm(0);
	close(fd);
	return 0;
}
