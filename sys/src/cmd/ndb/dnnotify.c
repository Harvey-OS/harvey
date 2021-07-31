#include <u.h>
#include <libc.h>
#include <ip.h>
#include <bio.h>
#include <ndb.h>
#include "dns.h"

/* get a notification from another system of a changed zone */
void
dnnotify(DNSmsg *reqp, DNSmsg *repp, Request *)
{
	RR *tp;
	Area *a;

	/* move one question from reqp to repp */
	memset(repp, 0, sizeof(*repp));
	tp = reqp->qd;
	reqp->qd = tp->next;
	tp->next = 0;
	repp->qd = tp;
	repp->id = reqp->id;
	repp->flags = Fresp  | Onotify | Fauth;

	/* anything to do? */
	if(zonerefreshprogram == nil)
		return;

	/* make sure its the right type */
	if(repp->qd->type != Tsoa)
		return;

syslog(0, logfile, "notification for %s", repp->qd->owner->name);

	/* is it something we care about? */
	a = inmyarea(repp->qd->owner->name);
	if(a == nil)
		return;

syslog(0, logfile, "serial old %lud new %lud", a->soarr->soa->serial, repp->qd->soa->serial);

	/* do nothing if it didn't change */
	if(a->soarr->soa->serial== repp->qd->soa->serial)
		return;

	a->needrefresh = 1;
}

static void
ding(void*, char *msg)
{
	if(strstr(msg, "alarm"))
		noted(NCONT);
	else
		noted(NDFLT);
}

/* notify a slave that an area has changed. */
static void
send_notify(char *slave, RR *soa, Request *req)
{
	int i, len, n, reqno, status, fd;
	uchar obuf[Maxudp+OUdphdrsize];
	uchar ibuf[Maxudp+OUdphdrsize];
	RR *rp;
	OUdphdr *up = (OUdphdr*)obuf;
	char *err;
	DNSmsg repmsg;

	/* create the request */
	reqno = rand();
	n = mkreq(soa->owner, Cin, obuf, Fauth | Onotify, reqno);

	/* get an address */
	if(strcmp(ipattr(slave), "ip") == 0) {
		parseip(up->raddr, slave);
	} else {
		rp = dnresolve(slave, Cin, Ta, req, nil, 0, 1, 1, &status);
		if(rp == nil)
			return;
		parseip(up->raddr, rp->ip->name);
		rrfree(rp);
	}

	fd = udpport();
	if(fd < 0)
		return;

	notify(ding);

	/* send 3 times or until we get anything back */
	n += OUdphdrsize;
	for(i = 0; i < 3; i++){
syslog(0, logfile, "sending %d byte notify to %s/%I.%d about %s", n, slave, up->raddr, nhgets(up->rport), soa->owner->name);
		if(write(fd, obuf, n) != n)
			break;
		alarm(2*1000);
		len = read(fd, ibuf, sizeof ibuf);
		alarm(0);
		if(len <= OUdphdrsize)
			continue;
		err = convM2DNS(&ibuf[OUdphdrsize], len, &repmsg);
		if(err != nil)
			continue;
		if(repmsg.id == reqno && (repmsg.flags & Omask) == Onotify)
			break;
	}

	close(fd);
}

/* send notifies for any updated areas */
static void
notify_areas(Area *a, Request *req)
{
	Server *s;

	for(; a != nil; a = a->next){
		if(!a->neednotify)
			continue;

		/* send notifies to all slaves */
		for(s = a->soarr->soa->slaves; s != nil; s = s->next)
			send_notify(s->name, a->soarr, req);
		a->neednotify = 0;
	}
}

/*
 *  process to notify other servers of changes
 *  (also reads in new databases)
 */
void
notifyproc(void)
{
	Request req;
	static int already;

	if(already)
		return;

	switch(rfork(RFPROC|RFNOTEG|RFMEM|RFNOWAIT)){
	case -1:
		return;
	case 0:
		break;
	default:
		return;
	}

	req.isslave = 1;	/* son't fork off subprocesses */

	for(;;){
		getactivity(&req);
		notify_areas(owned, &req);
		putactivity();
		sleep(60*1000);
	}
}
