#include "all.h"

#include "../ip/ip.h"

typedef struct Sntppkt {
	uchar	d[Easize];		/* ether header */
	uchar	s[Easize];
	uchar	type[2];

	uchar	vihl;			/* ip header */
	uchar	tos;
	uchar	length[2];
	uchar	id[2];
	uchar	frag[2];
	uchar	ttl;
	uchar	proto;
	uchar	cksum[2];
	uchar	src[Pasize];
	uchar	dst[Pasize];

	uchar	udpsrc[2];		/* Src port */
	uchar	udpdst[2];		/* Dst port */
	uchar	udplen[2];		/* Packet length */
	uchar	udpsum[2];		/* Checksum including header */
	uchar	mode;			/* li:2, vn:3, mode:3 */
	uchar	stratum;		/* level of local clock */
	signed char poll;		/* log2(max interval between polls) */
	signed char precision;		/* log2(clock precision) -6 => mains,
					   -18 => us */
	uchar rootdelay[4];		/* round trip delay to reference
				   	   16.16 fraction */
	uchar dispersion[4];		/* max error to reference */
	uchar clockid[4];		/* reference clock identifier */
	uchar reftime[8];		/* local time when clock set */
	uchar orgtime[8];		/* local time when client sent request */
	uchar rcvtime[8];		/* time request arrived */
	uchar xmttime[8];		/* time reply sent */
} Sntppkt;

enum {
	Sntpsize = 4 + 3 * 4 + 4 * 8,
	Version = 1,
	Stratum = 0,
	Poll = 0,
	LI = 0,
	Symmetric = 2,
	ClientMode = 3,
	ServerMode = 4,
	Epoch = 86400 * (365 * 70 + 17), /* 1900 to 1970 in seconds */
};

#define DEBUG if(cons.flags&sntp.flag)print

static struct {
	Lock;
	int flag;
	int gotreply;
	int kicked;
	Rendez r;
	Rendez doze;
} sntp;

static int
done(void*)
{
	return sntp.gotreply != 0;
}

static int
kicked(void*)
{
	return sntp.kicked != 0;
}

void
sntprecv(Msgbuf *mb, Ifc *ifc)
{
	Udppkt *uh;
	Sntppkt *sh;
	int v, li, m, now;

	USED(ifc);
	uh = (Udppkt *)mb->data;
	DEBUG("sntp: receive from %I\n", uh->src);
	if (memcmp(uh->src, sntpip, 4) != 0) {
		DEBUG("sntp: wrong IP address\n");
		goto overandout;
	}
	if (nhgets(uh->udplen) < Sntpsize) {
		DEBUG("sntp: packet too small\n");
		goto overandout;
	}
	sh = (Sntppkt *)mb->data;
	v = (sh->mode >> 3) & 7;
	li = (sh->mode >> 6);
	m = sh->mode & 7;
	/*
	 * if reply from right place and contains time set gotreply
	 * and wakeup r
	 */
	DEBUG("sntp: LI %d Version %d Mode %d\n", li, v, m);
	if (sh->stratum == 1) {
		char buf[5];
		memmove(buf, sh->clockid, 4);
		buf[4] = 0;
		DEBUG("sntp: Stratum 1 (%s)\n", buf);
	}
	else {
		DEBUG("sntp: Stratum %d\n", sh->stratum);
	}
	DEBUG("Poll %d Precision %d\n", sh->poll, sh->precision);
	DEBUG("RootDelay %ld Dispersion %ld\n",
	    nhgetl(sh->rootdelay), nhgetl(sh->dispersion));
	if (v == 0 || v > 3) {
		DEBUG("sntp: unsupported version\n");
		goto overandout;
	}
	if (m >= 6 || m == ClientMode) {
		DEBUG("sntp: wrong mode\n");
		goto overandout;
	}
	now = nhgetl(sh->xmttime) - Epoch;
	if (li == 3 || now == 0 || sh->stratum == 0) {
		/* unsynchronized */
		print("sntp: time server not synchronized\n");
		goto overandout;
	}
	settime(now);
	setrtc(now);
	print("sntp: %d\n", now);
	sntp.gotreply = 1;
	wakeup(&sntp.r);
overandout:
	mbfree(mb);
}

void
sntpsend(void)
{
	ushort sum;
	Msgbuf *mb;
	Sntppkt *s;
	uchar tmp[Pasize];
	Ifc *ifc;

	for(ifc = enets; ifc; ifc = ifc->next) {
		if(isvalidip(ifc->ipa))
			break;
	}
	if(ifc == nil)
		return;

	/* compose a UDP sntp request */
	DEBUG("sntp: sending\n");
	mb = mballoc(Ensize+Ipsize+Udpsize+Sntpsize, 0, Mbsntp);
	s = (Sntppkt *)mb->data;
	/* IP fields */	
	memmove(s->src, ifc->ipa, Pasize);
	memmove(s->dst, sntpip, Pasize);
	s->proto = Udpproto;
	s->ttl = 0;
	/* Udp fields */
	hnputs(s->udpsrc, SNTP_LOCAL);
	hnputs(s->udpdst, SNTP);
	hnputs(s->udplen, Sntpsize + Udpsize);
	/* Sntp fields */
	memset(mb->data + Ensize+Ipsize+Udpsize, 0, Sntpsize);
	s->mode = 010 | ClientMode;
	s->poll = 6;
	hnputl(s->orgtime, rtctime() + Epoch);	/* leave 0 fraction */
	/* Compute the UDP sum - form psuedo header */
	hnputs(s->cksum, Udpsize + Sntpsize);
	hnputs(s->udpsum, 0);
	sum = ptclcsum((uchar *)mb->data + Ensize + Ipsize - Udpphsize,
	    Udpsize + Udpphsize + Sntpsize);
	hnputs(s->udpsum, sum);
	/*
	  * now try to send it - cribbed from icmp.c
	  * send to interface 0
	  */
	memmove(tmp, s->dst, Pasize);
	if((nhgetl(ifc->ipa)&ifc->mask) != (nhgetl(s->dst)&ifc->mask))
		iproute(tmp, s->dst, ifc->netgate);
	ipsend1(mb, ifc, tmp);
}

#define TRIES 3
#define INTERVAL (60 * 60 * 1000)
#define TIMO 1000

void
sntptask(void)
{
	DEBUG("sntp: running\n");
	tsleep(&sntp.doze, kicked, 0, 2 * 60 * 1000);
	for (;;) {
		sntp.kicked = 0;
		DEBUG("sntp: poll time!\n");
		if (isvalidip(sntpip)) {
			int i;
			sntp.gotreply = 0;
			for (i = 0; i < TRIES; i++)
			{
				sntpsend();
				tsleep(&sntp.r, done, 0, TIMO);
				if (sntp.gotreply)
					break;
			}
			/* clock has been set */
		}
		tsleep(&sntp.doze, kicked, 0, INTERVAL);
	}
}

void
cmd_sntp(int argc, char *argv[])
{
	int i;

	if(argc <= 1) {
		print("sntp kick -- check time now\n");
		return;
	}
	for(i=1; i<argc; i++) {
		if(strcmp(argv[i], "kick") == 0) {
			sntp.kicked = 1;
			wakeup(&sntp.doze);
			continue;
		}
	}
}

void
sntpinit(void)
{
	cmd_install("sntp", "subcommand -- sntp protocol", cmd_sntp);
	sntp.flag = flag_install("sntp", "-- verbose");
	userinit(sntptask, 0, "sntp");
}
