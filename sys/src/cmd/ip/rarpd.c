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
#include <bio.h>
#include <ip.h>
#include <ndb.h>
#include "arp.h"

typedef struct Rarp Rarp;

struct Rarp {
	uint8_t edst[6];
	uint8_t esrc[6];
	uint8_t type[2];
	uint8_t hrd[2];
	uint8_t pro[2];
	uint8_t hln;
	uint8_t pln;
	uint8_t op[2];
	uint8_t sha[6];
	uint8_t spa[4];
	uint8_t tha[6];
	uint8_t tpa[4];
};

uint8_t myip[IPaddrlen];
uint8_t myether[6];
char rlog[] = "ipboot";
char *device = "ether0";
int debug;
Ndb *db;

char *lookup(char *, char *, char *, char *, int);

void
error(char *s)
{
	syslog(1, rlog, "error %s: %r", s);
	exits(s);
}

char net[32];

void
usage(void)
{
	fprint(2, "usage: %s [-e device] [-x netmtpt] [-f ndb-file] [-d]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int edata, ectl;
	uint8_t buf[2048];
	long n;
	Rarp *rp;
	char ebuf[16];
	char ipbuf[64];
	char file[128];
	int arp;
	char *p, *ndbfile;

	ndbfile = nil;
	setnetmtpt(net, sizeof(net), nil);
	ARGBEGIN
	{
	case 'e':
		p = ARGF();
		if(p == nil)
			usage();
		device = p;
		break;
	case 'd':
		debug = 1;
		break;
	case 'f':
		p = ARGF();
		if(p == nil)
			usage();
		ndbfile = p;
		break;
	case 'x':
		p = ARGF();
		if(p == nil)
			usage();
		setnetmtpt(net, sizeof(net), p);
		break;
	}
	ARGEND
	USED(argc);
	USED(argv);

	fmtinstall('E', eipfmt);
	fmtinstall('I', eipfmt);
	fmtinstall('V', eipfmt);

	db = ndbopen(ndbfile);
	if(db == 0)
		error("can't open the database");

	edata = dial(netmkaddr("0x8035", device, 0), 0, 0, &ectl);
	if(edata < 0)
		error("can't open ethernet");

	if(myipaddr(myip, net) < 0)
		error("can't get my ip address");
	sprint(ebuf, "%s/%s", net, device);
	if(myetheraddr(myether, ebuf) < 0)
		error("can't get my ether address");

	snprint(file, sizeof(file), "%s/arp", net);
	if((arp = open(file, ORDWR)) < 0)
		fprint(2, "rarpd: can't open %s\n", file);

	switch(rfork(RFNOTEG | RFPROC | RFFDG)) {
	case -1:
		error("fork");
	case 0:
		break;
	default:
		exits(0);
	}

	for(;;) {
		n = read(edata, buf, sizeof(buf));
		if(n <= 0)
			error("reading");
		if(n < sizeof(Rarp)) {
			syslog(debug, rlog, "bad packet size %ld", n);
			continue;
		}
		rp = (Rarp *)buf;
		if(rp->op[0] != 0 && rp->op[1] != 3) {
			syslog(debug, rlog, "bad op %d %d %E",
			       rp->op[1], rp->op[0], rp->esrc);
			continue;
		}

		if(debug)
			syslog(debug, rlog, "rcv se %E si %V te %E ti %V",
			       rp->sha, rp->spa, rp->tha, rp->tpa);

		sprint(ebuf, "%E", rp->tha);
		if(lookup("ether", ebuf, "ip", ipbuf, sizeof ipbuf) == nil) {
			syslog(debug, rlog, "client lookup failed: %s", ebuf);
			continue;
		}
		v4parseip(rp->tpa, ipbuf);

		memmove(rp->sha, myether, sizeof(rp->sha));
		v6tov4(rp->spa, myip);

		rp->op[0] = 0;
		rp->op[1] = 4;
		memmove(rp->edst, rp->esrc, sizeof(rp->edst));

		if(debug)
			syslog(debug, rlog, "send se %E si %V te %E ti %V",
			       rp->sha, rp->spa, rp->tha, rp->tpa);

		if(write(edata, buf, 60) != 60)
			error("write failed");

		if(arp < 0)
			continue;
		if(fprint(arp, "add %E %V", rp->esrc, rp->tpa) < 0)
			fprint(2, "can't write arp entry\n");
	}
}

char *
lookup(char *sattr, char *sval, char *tattr, char *tval, int len)
{
	static Ndb *db;
	char *attrs[1];
	Ndbtuple *t;

	if(db == nil)
		db = ndbopen(0);
	if(db == nil)
		return nil;

	if(sattr == nil)
		sattr = ipattr(sval);

	attrs[0] = tattr;
	t = ndbipinfo(db, sattr, sval, attrs, 1);
	if(t == nil)
		return nil;
	strncpy(tval, t->val, len);
	tval[len - 1] = 0;
	ndbfree(t);
	return tval;
}
