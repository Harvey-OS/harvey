#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dns.h"

int	udpannounce(void);
static RR*	getipaddr(DN*);
static void	recurse(DNSmsg*, Request*);
static void	local(DNSmsg*);
static void	reply(int, uchar*, DNSmsg*);
static void	hint(RR**, RR*);

extern char *logfile;

/*
 *  a process to act as a dns server for outside reqeusts
 */
void
dnserver(void)
{
	int fd, len;
	char *err;
	RR *tp;
	Request req;
	DNSmsg reqmsg;
	DNSmsg repmsg;
	uchar buf[Udphdrsize + Maxudp];
	char tname[32];

	/* fork sharing text, data, and bss with parent */
	switch(rfork(RFPROC|RFNOTEG|RFMEM|RFNOWAIT)){
	case -1:
		break;
	case 0:
		break;
	default:
		return;
	}

	while((fd = udpannounce()) < 0)
		sleep(5000);
	setjmp(req.mret);
	req.isslave = 0;

	/* loop on requests */
	for(;;){
		memset(&repmsg, 0, sizeof(repmsg));
		len = read(fd, buf, sizeof(buf));
		err = convM2DNS(&buf[Udphdrsize], len, &reqmsg);
		if(err){
			syslog(0, logfile, "server: input error: %s from %I", err, buf);
			continue;
		}
		if(reqmsg.qdcount < 1){
			syslog(0, logfile, "server: no questions from %I", buf);
		} else if(reqmsg.flags & Fresp){
			syslog(0, logfile, "server: reply not request from %I", buf);
		} else if(reqmsg.qd->owner->class != Cin){
			syslog(0, logfile, "server: class %d from %I", reqmsg.qd->owner->class,
				buf);
			memset(&repmsg, 0, sizeof(repmsg));
			repmsg.id = reqmsg.id;
			repmsg.flags = Runimplimented | Fresp | Fcanrec | Oquery;
			repmsg.qd = reqmsg.qd;
			reply(fd, buf, &repmsg);
		} else if((reqmsg.flags & Omask) != Oquery){
			syslog(0, logfile, "server: op %d from %I", reqmsg.flags & Omask, buf);
		} else {
			/* loop through each question */
			while(reqmsg.qd){
				if(debug)
					syslog(0, logfile, "serve %s %s",
						reqmsg.qd->owner->name,
						rrname(reqmsg.qd->type, tname));
				memset(&repmsg, 0, sizeof(repmsg));
				repmsg.id = reqmsg.id;
				repmsg.flags = Fresp | Fcanrec | Oquery;
				repmsg.qd = reqmsg.qd;
				reqmsg.qd = reqmsg.qd->next;
				repmsg.qd->next = 0;
	
				/*
				 *  get the answer if we can
				 */
				if(reqmsg.flags & Frecurse)
					recurse(&repmsg, &req);
				else
					local(&repmsg);
	
				/*
				 *  add ip addresses as hints
				 */
				for(tp = repmsg.ns; tp; tp = tp->next)
					hint(&repmsg.ar, tp);
				for(tp = repmsg.an; tp; tp = tp->next)
					hint(&repmsg.ar, tp);

				reply(fd, buf, &repmsg);

				rrfreelist(repmsg.qd);
				rrfreelist(repmsg.an);
				rrfreelist(repmsg.ns);
				rrfreelist(repmsg.ar);
			}
		}
		rrfreelist(reqmsg.qd);
		rrfreelist(reqmsg.an);
		rrfreelist(reqmsg.ns);
		rrfreelist(reqmsg.ar);

		if(req.isslave)
			_exits(0);
	}
}

/*
 *  satisfy a recursive request.  dnlookup will handle cnames.
 */
static void
recurse(DNSmsg *mp, Request *req)
{
	int type;
	char *name;
	RR *cname;

	name = mp->qd->owner->name;
	type = mp->qd->type;
	cname = 0;
	rrcat(&mp->an, dnresolve(name, Cin, type, req, &cname), type);
	if(cname)
		rrcat(&mp->an, cname, Tcname);
	if(mp->an && mp->an->auth)
		mp->flags |= Fauth;
}


/*
 *  satisfy a request with local info only
 */
static void
local(DNSmsg *mp)
{
	DN *dp, *nsdp;
	RR *nsrp;
	char *cp;
	int type;

	dp = mp->qd->owner;
	type = mp->qd->type;
	mp->flags |= Fauth;

	/* in-addr.arpa queries are special */
	if(type == Tptr){
		rrcat(&mp->an, dbinaddr(dp), type);
		if(mp->an)
			return;
	}

	/*
	 *  Quick grab, see if it's a 'relative to my domain' request.
	 *  I'm not sure this is a good idea but our x-terminals want it.
	 */
	if(strchr(dp->name, '.') == 0){
		nsrp = dblookup(dp->name, dp->class, type, 1);
		if(nsrp){
			rrcat(&mp->an, nsrp, type);
			return;
		}
	}

	/*
 	 *  walk up the domain name looking for
	 *  a name server for the domain.
	 */
	for(cp = dp->name; /* mp->an==0 && */ cp; cp = walkup(cp)){
		/* look for ns in cache and database */
		nsdp = dnlookup(cp, dp->class, 0);
		nsrp = 0;
		if(nsdp)
			nsrp = rrlookup(nsdp, Tns);
		if(nsrp == 0)
			nsrp = dblookup(cp, dp->class, Tns, 0);
		if(nsrp == 0)
			continue;

		if(nsrp->local){
			/* local domains resolved from this db */
			if(nsrp->db)	/* free database rr's */
				rrfreelist(nsrp);
			rrcat(&mp->an, dblookup(dp->name, Cin, type, 1), type);
		} else {
			/* just return the name of a name server to use */
			rrcat(&mp->ns, nsrp, Tns);
		}

		break;
	}
}

static void
reply(int fd, uchar *buf, DNSmsg *rep)
{
	int len;
	char tname[32];

	if(debug)
		syslog(0, logfile, "reply (%I/%d) %s %s an %R ns %R ar %R",
			buf, ((buf[4])<<8)+buf[5], rep->qd->owner->name,
			rrname(rep->qd->type, tname), rep->an, rep->ns, rep->ar);

	len = convDNS2M(rep, &buf[Udphdrsize], Maxudp);
	if(len <= 0)
		fatal("dnserver: converting reply");
	len += Udphdrsize;
	if(write(fd, buf, len) != len)
		fatal("dnserver: sending reply");
}

static void
hint(RR **last, RR *rp)
{
	switch(rp->type){
	case Tns:
	case Tmx:
	case Tmb:
	case Tmf:
	case Tmd:
		rrcat(last, dblookup(rp->host->name, Cin, Ta, 0), Ta);
		break;
	}
}

/*
 *  get ip addresses without looking off machine
 */
static RR*
getipaddr(DN *dp)
{
	RR *rp;

	/* first try the cache */
	rp = rrlookup(dp, Ta);
	if(rp)
		return rp;

	/* then try on disk db */
	return dblookup(dp->name, dp->class, Ta, 0);
}

/*
 *  announce on udp port and set message style interface
 */
static char *hmsg = "headers";

int
udpannounce(void)
{
	int data, ctl;
	char dir[64];
	char datafile[64+6];

	/* get a udp port */
	ctl = announce("udp!*!dns", dir);
	if(ctl < 0){
		warning("can't announce on udp port");
		return -1;
	}
	snprint(datafile, sizeof(datafile), "%s/data", dir);

	/* turn on header style interface */
	if(write(ctl, hmsg, strlen(hmsg)) , 0)
		fatal(hmsg);
	data = open(datafile, ORDWR);
	if(data < 0){
		close(ctl);
		warning("can't announce on udp port");
		return -1;
	}

	close(ctl);
	return data;
}
