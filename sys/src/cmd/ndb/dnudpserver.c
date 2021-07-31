#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dns.h"

static int	udpannounce(char*);
static void	reply(int, uchar*, DNSmsg*, Request*);

extern char *logfile;

static void
ding(void *x, char *msg)
{
	USED(x);
	if(strcmp(msg, "alarm") == 0)
		noted(NCONT);
	else
		noted(NDFLT);
}

typedef struct Inprogress Inprogress;
struct Inprogress
{
	int	inuse;
	OUdphdr	uh;
	DN	*owner;
	int	type;
	int	id;
};
Inprogress inprog[Maxactive+2];

/*
 *  record client id and ignore retransmissions.
 *  we're still single thread at this point.
 */
static Inprogress*
clientrxmit(DNSmsg *req, uchar *buf)
{
	Inprogress *p, *empty;
	OUdphdr *uh;

	uh = (OUdphdr *)buf;
	empty = 0;
	for(p = inprog; p < &inprog[Maxactive]; p++){
		if(p->inuse == 0){
			if(empty == 0)
				empty = p;
			continue;
		}
		if(req->id == p->id)
		if(req->qd->owner == p->owner)
		if(req->qd->type == p->type)
		if(memcmp(uh, &p->uh, OUdphdrsize) == 0)
			return 0;
	}
	if(empty == 0)
		return 0;	/* shouldn't happen - see slave() and definition of Maxactive */

	empty->id = req->id;
	empty->owner = req->qd->owner;
	empty->type = req->qd->type;
	memmove(&empty->uh, uh, OUdphdrsize);
	empty->inuse = 1;
	return empty;
}

/*
 *  a process to act as a dns server for outside reqeusts
 */
void
dnudpserver(char *mntpt)
{
	int fd, len, op;
	Request req;
	DNSmsg reqmsg, repmsg;
	uchar buf[OUdphdrsize + Maxudp + 1024];
	char *err;
	Inprogress *p;
	char tname[32];
	OUdphdr *uh;

	/* fork sharing text, data, and bss with parent */
	switch(rfork(RFPROC|RFNOTEG|RFMEM|RFNOWAIT)){
	case -1:
		break;
	case 0:
		break;
	default:
		return;
	}

	fd = -1;
	notify(ding);
restart:
	if(fd >= 0)
		close(fd);
	while((fd = udpannounce(mntpt)) < 0)
		sleep(5000);
	if(setjmp(req.mret))
		putactivity();
	req.isslave = 0;

	/* loop on requests */
	for(;; putactivity()){
		memset(&repmsg, 0, sizeof(repmsg));
		memset(&reqmsg, 0, sizeof(reqmsg));
		alarm(60*1000);
		len = read(fd, buf, sizeof(buf));
		alarm(0);
		if(len <= OUdphdrsize)
			goto restart;
		uh = (OUdphdr*)buf;
		len -= OUdphdrsize;
		getactivity(&req);
		req.aborttime = now + 30;	/* don't spend more than 30 seconds */
		err = convM2DNS(&buf[OUdphdrsize], len, &reqmsg);
		if(err){
			syslog(0, logfile, "server: input error: %s from %I", err, buf);
			continue;
		}
		if(reqmsg.qdcount < 1){
			syslog(0, logfile, "server: no questions from %I", buf);
			goto freereq;
		}
		if(reqmsg.flags & Fresp){
			syslog(0, logfile, "server: reply not request from %I", buf);
			goto freereq;
		}
		op = reqmsg.flags & Omask;
		if(op != Oquery && op != Onotify){
			syslog(0, logfile, "server: op %d from %I", reqmsg.flags & Omask, buf);
			goto freereq;
		}

		if(debug || (trace && subsume(trace, reqmsg.qd->owner->name))){
			syslog(0, logfile, "%d: serve (%I/%d) %d %s %s",
				req.id, buf, ((uh->rport[0])<<8)+uh->rport[1],
				reqmsg.id,
				reqmsg.qd->owner->name,
				rrname(reqmsg.qd->type, tname, sizeof tname));
		}

		p = clientrxmit(&reqmsg, buf);
		if(p == 0){
			if(debug)
				syslog(0, logfile, "%d: duplicate", req.id);
			goto freereq;
		}

		/* loop through each question */
		while(reqmsg.qd){
			memset(&repmsg, 0, sizeof(repmsg));
			switch(op){
			case Oquery:
				dnserver(&reqmsg, &repmsg, &req);
				break;
			case Onotify:
				dnnotify(&reqmsg, &repmsg, &req);
				break;
			}
			reply(fd, buf, &repmsg, &req);
			rrfreelist(repmsg.qd);
			rrfreelist(repmsg.an);
			rrfreelist(repmsg.ns);
			rrfreelist(repmsg.ar);
		}

		p->inuse = 0;

freereq:
		rrfreelist(reqmsg.qd);
		rrfreelist(reqmsg.an);
		rrfreelist(reqmsg.ns);
		rrfreelist(reqmsg.ar);

		if(req.isslave){
			putactivity();
			_exits(0);
		}

	}
}

/*
 *  announce on udp port and set message style interface
 */
static char *hmsg = "headers";
static char *ohmsg = "oldheaders";

static int
udpannounce(char *mntpt)
{
	int data, ctl;
	char dir[64];
	char datafile[64+6];

	/* get a udp port */
	sprint(datafile, "%s/udp!*!dns", mntpt);
	ctl = announce(datafile, dir);
	if(ctl < 0){
		warning("can't announce on dns udp port");
		return -1;
	}
	snprint(datafile, sizeof(datafile), "%s/data", dir);

	/* turn on header style interface */
	if(write(ctl, hmsg, strlen(hmsg)) , 0)
		abort(); /* hmsg */;
	write(ctl, ohmsg, strlen(ohmsg));
	data = open(datafile, ORDWR);
	if(data < 0){
		close(ctl);
		warning("can't announce on dns udp port");
		return -1;
	}

	close(ctl);
	return data;
}

static void
reply(int fd, uchar *buf, DNSmsg *rep, Request *reqp)
{
	int len;
	char tname[32];
	RR *rp;

	if(debug || (trace && subsume(trace, rep->qd->owner->name)))
		syslog(0, logfile, "%d: reply (%I/%d) %d %s %s an %R ns %R ar %R",
			reqp->id, buf, ((buf[4])<<8)+buf[5],
			rep->id, rep->qd->owner->name,
			rrname(rep->qd->type, tname, sizeof tname), rep->an, rep->ns, rep->ar);

	len = convDNS2M(rep, &buf[OUdphdrsize], Maxudp);
	if(len <= 0){
		syslog(0, logfile, "error converting reply: %s %d", rep->qd->owner->name,
			rep->qd->type);
		for(rp = rep->an; rp; rp = rp->next)
			syslog(0, logfile, "an %R", rp);
		for(rp = rep->ns; rp; rp = rp->next)
			syslog(0, logfile, "ns %R", rp);
		for(rp = rep->ar; rp; rp = rp->next)
			syslog(0, logfile, "ar %R", rp);
		return;
	}
	len += OUdphdrsize;
	if(write(fd, buf, len) != len)
		syslog(0, logfile, "error sending reply: %r");
}
