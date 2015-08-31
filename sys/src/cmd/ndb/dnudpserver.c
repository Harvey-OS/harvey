/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dns.h"

enum {
	Logqueries = 0,
};

static int	udpannounce(char*);
static void	reply(int, uint8_t*, DNSmsg*, Request*);

typedef struct Inprogress Inprogress;
struct Inprogress
{
	int	inuse;
	Udphdr	uh;
	DN	*owner;
	uint16_t	type;
	int	id;
};
Inprogress inprog[Maxactive+2];

typedef struct Forwtarg Forwtarg;
struct Forwtarg {
	char	*host;
	uint8_t	addr[IPaddrlen];
	int	fd;
	uint32_t	lastdial;
};
Forwtarg forwtarg[10];
int currtarg;

static char *hmsg = "headers";

/*
 *  record client id and ignore retransmissions.
 *  we're still single thread at this point.
 */
static Inprogress*
clientrxmit(DNSmsg *req, uint8_t *buf)
{
	Inprogress *p, *empty;
	Udphdr *uh;

	uh = (Udphdr *)buf;
	empty = nil;
	for(p = inprog; p < &inprog[Maxactive]; p++){
		if(p->inuse == 0){
			if(empty == nil)
				empty = p;
			continue;
		}
		if(req->id == p->id && req->qd != nil &&
		    req->qd->owner == p->owner && req->qd->type == p->type &&
		    memcmp(uh, &p->uh, Udphdrsize) == 0)
			return nil;
	}
	if(empty == nil)
		return nil; /* shouldn't happen: see slave() & Maxactive def'n */
	if(req->qd == nil) {
		dnslog("clientrxmit: nil req->qd");
		return nil;
	}
	empty->id = req->id;
	empty->owner = req->qd->owner;
	empty->type = req->qd->type;
	if (empty->type != req->qd->type)
		dnslog("clientrxmit: bogus req->qd->type %d", req->qd->type);
	memmove(&empty->uh, uh, Udphdrsize);
	empty->inuse = 1;
	return empty;
}

int
addforwtarg(char *host)
{
	Forwtarg *tp;

	if (currtarg >= nelem(forwtarg)) {
		dnslog("too many forwarding targets");
		return -1;
	}
	tp = forwtarg + currtarg;
	if (parseip(tp->addr, host) < 0) {
		dnslog("can't parse ip %s", host);
		return -1;
	}
	tp->lastdial = time(nil);
	tp->fd = udpport(mntpt);
	if (tp->fd < 0)
		return -1;

	free(tp->host);
	tp->host = estrdup(host);
	currtarg++;
	return 0;
}

/*
 * fast forwarding of incoming queries to other dns servers.
 * intended primarily for debugging.
 */
static void
redistrib(uint8_t *buf, int len)
{
	Forwtarg *tp;
	Udphdr *uh;
	static uint8_t outpkt[1500];

	assert(len <= sizeof outpkt);
	memmove(outpkt, buf, len);
	uh = (Udphdr *)outpkt;
	for (tp = forwtarg; tp < forwtarg + currtarg; tp++)
		if (tp->fd > 0) {
			memmove(outpkt, tp->addr, sizeof tp->addr);
			hnputs(uh->rport, Dnsport);
			if (write(tp->fd, outpkt, len) != len) {
				close(tp->fd);
				tp->fd = -1;
			}
		} else if (tp->host && time(nil) - tp->lastdial > 60) {
			tp->lastdial = time(nil);
			tp->fd = udpport(mntpt);
		}
}

/*
 *  a process to act as a dns server for outside reqeusts
 */
void
dnudpserver(char *mntpt)
{
	int fd, len, op, rcode;
	char *err;
	char tname[32];
	uint8_t buf[Udphdrsize + Maxpayload];
	DNSmsg reqmsg, repmsg;
	Inprogress *p;
	Request req;
	Udphdr *uh;

	/*
	 * fork sharing text, data, and bss with parent.
	 * stay in the same note group.
	 */
	switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
	case -1:
		break;
	case 0:
		break;
	default:
		return;
	}

	fd = -1;
restart:
	procsetname("udp server announcing");
	if(fd >= 0)
		close(fd);
	while((fd = udpannounce(mntpt)) < 0)
		sleep(5000);

//	procsetname("udp server");
	memset(&req, 0, sizeof req);
	if(setjmp(req.mret))
		putactivity(0);
	req.isslave = 0;
	req.id = 0;
	req.aborttime = 0;

	/* loop on requests */
	for(;; putactivity(0)){
		procsetname("served %d udp; %d alarms",
			stats.qrecvdudp, stats.alarms);
		memset(&repmsg, 0, sizeof repmsg);
		memset(&reqmsg, 0, sizeof reqmsg);

		alarm(60*1000);
		len = read(fd, buf, sizeof buf);
		alarm(0);
		if(len <= Udphdrsize)
			goto restart;

		redistrib(buf, len);

		uh = (Udphdr*)buf;
		len -= Udphdrsize;

		// dnslog("read received UDP from %I to %I",
		//	((Udphdr*)buf)->raddr, ((Udphdr*)buf)->laddr);
		getactivity(&req, 0);
		req.aborttime = timems() + Maxreqtm;
//		req.from = smprint("%I", ((Udphdr*)buf)->raddr);
		req.from = smprint("%I", buf);
		rcode = 0;
		stats.qrecvdudp++;

		err = convM2DNS(&buf[Udphdrsize], len, &reqmsg, &rcode);
		if(err){
			/* first bytes in buf are source IP addr */
			dnslog("server: input error: %s from %I", err, buf);
			free(err);
			goto freereq;
		}
		if (rcode == 0)
			if(reqmsg.qdcount < 1){
				dnslog("server: no questions from %I", buf);
				goto freereq;
			} else if(reqmsg.flags & Fresp){
				dnslog("server: reply not request from %I", buf);
				goto freereq;
			}
		op = reqmsg.flags & Omask;
		if(op != Oquery && op != Onotify){
			dnslog("server: op %d from %I", reqmsg.flags & Omask,
				buf);
			goto freereq;
		}

		if(debug || (trace && subsume(trace, reqmsg.qd->owner->name)))
			dnslog("%d: serve (%I/%d) %d %s %s",
				req.id, buf, uh->rport[0]<<8 | uh->rport[1],
				reqmsg.id, reqmsg.qd->owner->name,
				rrname(reqmsg.qd->type, tname, sizeof tname));

		p = clientrxmit(&reqmsg, buf);
		if(p == nil){
			if(debug)
				dnslog("%d: duplicate", req.id);
			goto freereq;
		}

		if (Logqueries) {
			RR *rr;

			for (rr = reqmsg.qd; rr; rr = rr->next)
				syslog(0, "dnsq", "id %d: (%I/%d) %d %s %s",
					req.id, buf, uh->rport[0]<<8 |
					uh->rport[1], reqmsg.id,
					reqmsg.qd->owner->name,
					rrname(reqmsg.qd->type, tname,
					sizeof tname));
		}
		/* loop through each question */
		while(reqmsg.qd){
			memset(&repmsg, 0, sizeof repmsg);
			switch(op){
			case Oquery:
				dnserver(&reqmsg, &repmsg, &req, buf, rcode);
				break;
			case Onotify:
				dnnotify(&reqmsg, &repmsg, &req);
				break;
			}
			/* send reply on fd to address in buf's udp hdr */
			reply(fd, buf, &repmsg, &req);
			freeanswers(&repmsg);
		}

		p->inuse = 0;
freereq:
		free(req.from);
		req.from = nil;
		freeanswers(&reqmsg);
		if(req.isslave){
			putactivity(0);
			_exits(0);
		}
	}
}

/*
 *  announce on well-known dns udp port and set message style interface
 */
static int
udpannounce(char *mntpt)
{
	int data, ctl;
	char dir[64], datafile[64+6];
	static int whined;

	/* get a udp port */
	sprint(datafile, "%s/udp!*!dns", mntpt);
	ctl = announce(datafile, dir);
	if(ctl < 0){
		if(!whined++)
			warning("can't announce on %s", datafile);
		return -1;
	}

	/* turn on header style interface */
	if(write(ctl, hmsg, strlen(hmsg)) != strlen(hmsg))
		abort();			/* hmsg */

	snprint(datafile, sizeof(datafile), "%s/data", dir);
	data = open(datafile, ORDWR);
	if(data < 0){
		close(ctl);
		if(!whined++)
			warning("can't open %s to announce on dns udp port",
				datafile);
		return -1;
	}

	close(ctl);
	return data;
}

static void
reply(int fd, uint8_t *buf, DNSmsg *rep, Request *reqp)
{
	int len;
	char tname[32];

	if(debug || (trace && subsume(trace, rep->qd->owner->name)))
		dnslog("%d: reply (%I/%d) %d %s %s qd %R an %R ns %R ar %R",
			reqp->id, buf, buf[4]<<8 | buf[5],
			rep->id, rep->qd->owner->name,
			rrname(rep->qd->type, tname, sizeof tname),
			rep->qd, rep->an, rep->ns, rep->ar);

	len = convDNS2M(rep, &buf[Udphdrsize], Maxdnspayload);
	len += Udphdrsize;
	if(write(fd, buf, len) != len)
		dnslog("error sending reply: %r");
}
