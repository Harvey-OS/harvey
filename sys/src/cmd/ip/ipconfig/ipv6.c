/*
 * ipconfig for IPv6
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
typedef struct Block Block;
typedef struct Fs Fs;
#include "/sys/src/9/ip/ipv6.h"
#include "ipconfig.h"
#include "../icmp.h"

#pragma varargck argpos ralog 1

#define RALOG "v6routeradv"

#define NetS(x) (((uchar*)x)[0]<< 8 | ((uchar*)x)[1])
#define NetL(x) (((uchar*)x)[0]<<24 | ((uchar*)x)[1]<<16 | \
		 ((uchar*)x)[2]<< 8 | ((uchar*)x)[3])

enum {
	ICMP6LEN=	4,
};

typedef struct Hdr Hdr;
struct Hdr			/* ICMP v4 & v6 header */
{
	uchar	type;
	uchar	code;
	uchar	cksum[2];	/* Checksum */
	uchar	data[1];
};

char *icmpmsg6[Maxtype6+1] =
{
[EchoReply]		"EchoReply",
[UnreachableV6]		"UnreachableV6",
[PacketTooBigV6]	"PacketTooBigV6",
[TimeExceedV6]		"TimeExceedV6",
[Redirect]		"Redirect",
[EchoRequest]		"EchoRequest",
[TimeExceed]		"TimeExceed",
[InParmProblem]		"InParmProblem",
[Timestamp]		"Timestamp",
[TimestampReply]	"TimestampReply",
[InfoRequest]		"InfoRequest",
[InfoReply]		"InfoReply",
[AddrMaskRequest]	"AddrMaskRequest",
[AddrMaskReply]		"AddrMaskReply",
[EchoRequestV6]		"EchoRequestV6",
[EchoReplyV6]		"EchoReplyV6",
[RouterSolicit]		"RouterSolicit",
[RouterAdvert]		"RouterAdvert",
[NbrSolicit]		"NbrSolicit",
[NbrAdvert]		"NbrAdvert",
[RedirectV6]		"RedirectV6",
};

static char *icmp6opts[] =
{
[0]		"unknown opt",
[SRC_LLADDR]	"sll_addr",
[TARGET_LLADDR]	"tll_addr",
[PREFIX_INFO]	"pref_opt",
[REDIR_HEADER]	"redirect",
[MTU_OPTION]	"mtu_opt",
};

uchar v6allroutersL[IPaddrlen] = {
	0xff, 0x02, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0x02
};

uchar v6allnodesL[IPaddrlen] = {
	0xff, 0x02, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0x01
};

uchar v6Unspecified[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};

uchar v6glunicast[IPaddrlen] = {
	0x08, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};

uchar v6linklocal[IPaddrlen] = {
	0xfe, 0x80, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};

uchar v6solpfx[IPaddrlen] = {
	0xff, 0x02, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 1,
	/* last 3 bytes filled with low-order bytes of addr being solicited */
	0xff, 0, 0, 0,
};

uchar v6defmask[IPaddrlen] = {
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0, 0, 0, 0,
	0, 0, 0, 0
};

enum
{
	Vadd,
	Vremove,
	Vunbind,
	Vaddpref6,
	Vra6,
};

static void
ralog(char *fmt, ...)
{
	char msg[512];
	va_list arg;

	va_start(arg, fmt);
	vseprint(msg, msg+sizeof msg, fmt, arg);
	va_end(arg);
	syslog(debug, RALOG, msg);
}

extern void
ea2lla(uchar *lla, uchar *ea)
{
	assert(IPaddrlen == 16);
	memset(lla, 0, IPaddrlen);
	lla[0]  = 0xFE;
	lla[1]  = 0x80;
	lla[8]  = ea[0] | 0x2;
	lla[9]  = ea[1];
	lla[10] = ea[2];
	lla[11] = 0xFF;
	lla[12] = 0xFE;
	lla[13] = ea[3];
	lla[14] = ea[4];
	lla[15] = ea[5];
}

extern void
ipv62smcast(uchar *smcast, uchar *a)
{
	assert(IPaddrlen == 16);
	memset(smcast, 0, IPaddrlen);
	smcast[0]  = 0xFF;
	smcast[1]  = 0x02;
	smcast[11] = 0x1;
	smcast[12] = 0xFF;
	smcast[13] = a[13];
	smcast[14] = a[14];
	smcast[15] = a[15];
}

void
v6paraminit(void)
{
	conf.sendra = conf.recvra = 0;
	conf.mflag = 0;
	conf.oflag = 0;
	conf.maxraint = MAX_INIT_RTR_ADVERT_INTVL;
	conf.minraint = MAX_INIT_RTR_ADVERT_INTVL / 4;
	conf.linkmtu = 1500;
	conf.reachtime = REACHABLE_TIME;
	conf.rxmitra = RETRANS_TIMER;
	conf.ttl = MAXTTL;

	conf.routerlt = 0;
	conf.force = 0;

	conf.prefixlen = 64;
	conf.onlink = 0;
	conf.autoflag = 0;
	conf.validlt = conf.preflt = ~0L;
}

static char*
opt_seprint(uchar *ps, uchar *pe, char *sps, char *spe)
{
	int otype, osz, pktsz;
	uchar *a;
	char *p = sps, *e = spe;

	a = ps;
	for (pktsz = pe - ps; pktsz > 0; pktsz -= osz) {
		otype = a[0];
		osz = a[1] * 8;

		switch (otype) {
		default:
			return seprint(p, e, " option=%s ", icmp6opts[otype]);
		case SRC_LLADDR:
		case TARGET_LLADDR:
			if (pktsz < osz || osz != 8)
				return seprint(p, e, " option=%s bad size=%d",
					icmp6opts[otype], osz);
			p = seprint(p, e, " option=%s maddr=%E",
				icmp6opts[otype], a+2);
			break;
		case PREFIX_INFO:
			if (pktsz < osz || osz != 32)
				return seprint(p, e, " option=%s: bad size=%d",
					icmp6opts[otype], osz);

			p = seprint(p, e, " option=%s pref=%I preflen=%3.3d"
				" lflag=%1.1d aflag=%1.1d unused1=%1.1d"
				" validlt=%ud preflt=%ud unused2=%1.1d",
				icmp6opts[otype], a+16, (int)(*(a+2)),
				(*(a+3) & (1 << 7)) != 0,
				(*(a+3) & (1 << 6)) != 0,
				(*(a+3) & 63) != 0,
				NetL(a+4), NetL(a+8), NetL(a+12)!=0);
			break;
		}
		a += osz;
	}
	return p;
}

static void
pkt2str(uchar *ps, uchar *pe, char *sps, char *spe)
{
	int pktlen;
	char *tn, *p, *e;
	uchar *a;
	Hdr *h;

	h = (Hdr*)ps;
	a = ps + 4;
	p = sps;
	e = spe;

	pktlen = pe - ps;
	if(pktlen < ICMP6LEN) {
		seprint(sps, spe, "short pkt");
		return;
	}

	tn = icmpmsg6[h->type];
	if(tn == nil)
		p = seprint(p, e, "t=%ud c=%d ck=%4.4ux", h->type,
			h->code, (ushort)NetS(h->cksum));
	else
		p = seprint(p, e, "t=%s c=%d ck=%4.4ux", tn,
			h->code, (ushort)NetS(h->cksum));

	switch(h->type){
	case RouterSolicit:
		ps += 8;
		p = seprint(p, e, " unused=%1.1d ", NetL(a)!=0);
		opt_seprint(ps, pe, p, e);
		break;
	case RouterAdvert:
		ps += 16;
		p = seprint(p, e, " hoplim=%3.3d mflag=%1.1d oflag=%1.1d"
			" unused=%1.1d routerlt=%d reachtime=%d rxmtimer=%d",
			a[0],
			(*(a+1) & (1 << 7)) != 0,
			(*(a+1) & (1 << 6)) != 0,
			(*(a+1) & 63) != 0,
			NetS(a+2), NetL(a+4), NetL(a+8));
		opt_seprint(ps, pe, p, e);
		break;
	default:
		seprint(p, e, " unexpected icmp6 pkt type");
		break;
	}
}

static void
catch(void *a, char *msg)
{
	USED(a);
	if(strstr(msg, "alarm"))
		noted(NCONT);
	else
		noted(NDFLT);
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

int
dialicmp(uchar *dst, int dport, int *ctlfd)
{
	int fd, cfd, n, m;
	char cmsg[100], name[128], connind[40];
	char hdrs[] = "headers";

	snprint(name, sizeof name, "%s/icmpv6/clone", conf.mpoint);
	cfd = open(name, ORDWR);
	if(cfd < 0)
		sysfatal("dialicmp: can't open %s: %r", name);

	n = snprint(cmsg, sizeof cmsg, "connect %I!%d!r %d", dst, dport, dport);
	m = write(cfd, cmsg, n);
	if (m < n)
		sysfatal("dialicmp: can't write %s to %s: %r", cmsg, name);

	seek(cfd, 0, 0);
	n = read(cfd, connind, sizeof connind);
	if (n < 0)
		connind[0] = 0;
	else if (n < sizeof connind)
		connind[n] = 0;
	else
		connind[sizeof connind - 1] = 0;

	snprint(name, sizeof name, "%s/icmpv6/%s/data", conf.mpoint, connind);
	fd = open(name, ORDWR);
	if(fd < 0)
		sysfatal("dialicmp: can't open %s: %r", name);

	n = sizeof hdrs - 1;
	if(write(cfd, hdrs, n) < n)
		sysfatal("dialicmp: can't write `%s' to %s: %r", hdrs, name);
	*ctlfd = cfd;
	return fd;
}

/* add ipv6 addr to an interface */
int
ip6cfg(int autoconf)
{
	int dupfound = 0, n;
	char *p;
	char buf[256];
	uchar ethaddr[6];
	Biobuf *bp;

	if (autoconf) {			/* create link-local addr */
		if (myetheraddr(ethaddr, conf.dev) < 0)
			sysfatal("myetheraddr w/ %s failed: %r", conf.dev);
		ea2lla(conf.laddr, ethaddr);
	}

	if (dupl_disc)
		n = sprint(buf, "try");
	else
		n = sprint(buf, "add");

	n += snprint(buf+n, sizeof buf-n, " %I", conf.laddr);
	if(!validip(conf.mask))
		ipmove(conf.mask, v6defmask);
	n += snprint(buf+n, sizeof buf-n, " %M", conf.mask);
	if(validip(conf.raddr)){
		n += snprint(buf+n, sizeof buf-n, " %I", conf.raddr);
		if(conf.mtu != 0)
			n += snprint(buf+n, sizeof buf-n, " %d", conf.mtu);
	}

	if(write(conf.cfd, buf, n) < 0){
		fprint(2, "%s: write(%s): %r\n", argv0, buf);
		return -1;
	}

	if (!dupl_disc)
		return 0;

	sleep(3000);

	/* read arp table, look for addr duplication */
	snprint(buf, sizeof buf, "%s/arp", conf.mpoint);
	bp = Bopen(buf, OREAD);
	if (bp == 0) {
		fprint(2, "%s: couldn't open %s/arp: %r\n", argv0, conf.mpoint);
		return -1;
	}

	snprint(buf, sizeof buf, "%I", conf.laddr);
	while(p = Brdline(bp, '\n')){
		p[Blinelen(bp)-1] = 0;
		if(cistrstr(p, buf) != 0) {
			fprint(2, "%s: found dup entry in arp cache\n", argv0);
			dupfound = 1;
			break;
		}
	}
	Bterm(bp);

	if (dupfound)
		doremove();
	else {
		n = sprint(buf, "add %I %M", conf.laddr, conf.mask);
		if(validip(conf.raddr)){
			n += snprint(buf+n, sizeof buf-n, " %I", conf.raddr);
			if(conf.mtu != 0)
				n += snprint(buf+n, sizeof buf-n, " %d",
					conf.mtu);
		}
		write(conf.cfd, buf, n);
	}
	return 0;
}

static int
recvra6on(char *net, int conn)
{
	Ipifc* ifc;

	ifc = readipifc(net, nil, conn);
	if (ifc == nil)
		return 0;
	else if (ifc->sendra6 > 0)
		return IsRouter;
	else if (ifc->recvra6 > 0)
		return IsHostRecv;
	else
		return IsHostNoRecv;
}

static void
sendrs(int fd)
{
	Routersol *rs;
	uchar buff[sizeof *rs];

	memset(buff, 0, sizeof buff);
	rs = (Routersol *)buff;
	memmove(rs->dst, v6allroutersL, IPaddrlen);
	memmove(rs->src, v6Unspecified, IPaddrlen);
	rs->type = ICMP6_RS;

	if(write(fd, rs, sizeof buff) < sizeof buff)
		ralog("sendrs: write failed, pkt size %d", sizeof buff);
}

/*
 * a router receiving a router adv from another
 * router calls this; it is basically supposed to
 * log the information in the ra and raise a flag
 * if any parameter value is different from its configured values.
 *
 * doing nothing for now since I don't know where to log this yet.
 */
static void
recvrarouter(uchar buf[], int pktlen)
{
	USED(buf, pktlen);
	ralog("i am a router and got a router advert");
}

/* host receiving a router advertisement calls this */

static void
recvrahost(uchar buf[], int pktlen)
{
	int arpfd, m, n;
	char abuf[100], configcmd[256];
	uchar optype;
	Lladdropt *llao;
	Mtuopt *mtuo;
	Prefixopt *prfo;
	Routeradv *ra;

	ra = (Routeradv*)buf;
	memmove(conf.v6gaddr, ra->src, IPaddrlen);
	conf.force = 0;
	conf.ttl = ra->cttl;
	conf.mflag = (MFMASK & ra->mor);
	conf.oflag = (OCMASK & ra->mor);
	conf.routerlt =  nhgets(ra->routerlt);
	conf.reachtime = nhgetl(ra->rchbltime);
	conf.rxmitra =   nhgetl(ra->rxmtimer);

	n = snprint(configcmd, sizeof configcmd, "%s %I %d %d %d %d %d %d %d",
		"gate6", conf.v6gaddr, conf.force, conf.ttl, conf.mflag,
		conf.oflag, conf.routerlt, conf.reachtime, conf.rxmitra);
	if (write(conf.cfd, configcmd, n) < 0)
		ralog("write (%s) failed", configcmd);

	m = sizeof *ra;
	while (pktlen - m > 0) {
		optype = buf[m];
		switch (optype) {
		case SRC_LLADDR:
			llao = (Lladdropt *)&buf[m];
			m += 8 * buf[m+1];
			if (llao->len != 1) {
				ralog(
	"recvrahost: illegal len(%d) for source link layer address option",
					llao->len);
				return;
			}
			if (memcmp(ra->src, v6linklocal, 2) != 0) {
				ralog(
			"recvrahost: non-linklocal src addr for router adv %I",
					ra->src);
				return;
			}

			snprint(abuf, sizeof abuf, "%s/arp", conf.mpoint);
			arpfd = open(abuf, OWRITE);
			if (arpfd < 0) {
				ralog(
				"recvrahost: couldn't open %s/arp to write: %r",
					conf.mpoint);
				return;
			}

			n = snprint(abuf, sizeof abuf, "add ether %I %E",
				ra->src, llao->lladdr);
			if (write(arpfd, abuf, n) < n)
				ralog("recvrahost: couldn't write to %s/arp",
					conf.mpoint);
			close(arpfd);
			break;
		case TARGET_LLADDR:
		case REDIR_HEADER:
			m += 8 * buf[m+1];
			ralog("ignoring unexpected optype %s in Routeradv",
				icmp6opts[optype]);
			break;
		case MTU_OPTION:
			mtuo = (Mtuopt*)&buf[m];
			m += 8 * mtuo->len;
			conf.linkmtu = nhgetl(mtuo->mtu);
			break;
		case PREFIX_INFO:
			prfo = (Prefixopt*)&buf[m];
			m += 8 * prfo->len;
			if (prfo->len != 4) {
				ralog("illegal len(%d) for prefix option",
					prfo->len);
				return;
			}
			memmove(conf.v6pref, prfo->pref, IPaddrlen);
			conf.prefixlen = prfo->plen;
			conf.onlink =   ((prfo->lar & OLMASK) != 0);
			conf.autoflag = ((prfo->lar & AFMASK) != 0);
			conf.validlt = nhgetl(prfo->validlt);
			conf.preflt =  nhgetl(prfo->preflt);
			n = snprint(configcmd, sizeof configcmd,
				"%s %I %d %d %d %uld %uld",
				"addpref6", conf.v6pref, conf.prefixlen,
				conf.onlink, conf.autoflag,
				conf.validlt, conf.preflt);
			if (write(conf.cfd, configcmd, n) < 0)
				ralog("write (%s) failed", configcmd);
			break;
		default:
			m += 8 * buf[m+1];
			ralog("ignoring optype %s in Routeradv", icmp6opts[0]);
			break;
		}
	}
}

/*
 * daemon to receive router adverisements
 */
void
recvra6(void)
{
	int fd, cfd, n, sendrscnt, sleepfor;
	uchar buf[4096];

	fd = dialicmp(v6allnodesL, ICMP6_RA, &cfd);
	if (fd < 0)
		sysfatal("can't open icmp_ra connection: %r");

	notify(catch);
	sendrscnt = MAX_RTR_SOLICITS;

	switch(rfork(RFPROC|RFMEM|RFFDG|RFNOWAIT|RFNOTEG)){
	case -1:
		sysfatal("can't fork: %r");
	default:
		return;
	case 0:
		procsetname("recvra6 on %s", conf.dev);
		ralog("recvra6 on %s", conf.dev);
		sleepfor = nrand(10);
		for (;;) {
			alarm(sleepfor);
			n = read(fd, buf, sizeof buf);
			alarm(0);
			if (n <= 0) {
				if (sendrscnt > 0) {
					sendrscnt--;
					if (recvra6on(conf.mpoint, myifc) ==
					    IsHostRecv)
						sendrs(fd);
					sleepfor = RTR_SOLICIT_INTVL+nrand(100);
				}
				if (sendrscnt == 0) {
					sendrscnt--;
					sleepfor = 0;
					ralog(
				"recvra6: no router advs after %d sols on %s",
						 MAX_RTR_SOLICITS, conf.dev);
				}
				continue;
			}

			sleepfor = 0;
			switch (recvra6on(conf.mpoint, myifc)) {
			case IsRouter:
				recvrarouter(buf, n);
				break;
			case IsHostRecv:
				recvrahost(buf, n);
				break;
			case IsHostNoRecv:
				ralog("recvra6: recvra off, quitting");
				close(fd);
				exits(0);
			default:
				ralog(
				"recvra6: unable to read router status on %s",
					conf.dev);
				break;
			}
		}
	}
}

/*
 * return -1 -- error, reading/writing some file,
 *         0 -- no arptable updates
 *         1 -- successful arptable update
 */
int
recvrs(uchar *buf, int pktlen, uchar *sol)
{
	int n, optsz, arpfd;
	char abuf[256];
	Routersol *rs;
	Lladdropt *llao;

	rs = (Routersol *)buf;
	n = sizeof *rs;
	optsz = pktlen - n;
	pkt2str(buf, buf+pktlen, abuf, abuf+nelem(abuf));

	if (optsz != sizeof *llao)
		return 0;
	if (buf[n] != SRC_LLADDR || 8*buf[n+1] != sizeof *llao) {
		ralog("rs opt err %s", abuf);
		return -1;
	}

	ralog("rs recv %s", abuf);

	if (memcmp(rs->src, v6Unspecified, IPaddrlen) == 0)
		return 0;

	snprint(abuf, sizeof abuf, "%s/arp", conf.mpoint);
	arpfd = open(abuf, OWRITE);
	if (arpfd < 0) {
		ralog("recvrs: can't open %s/arp to write: %r", conf.mpoint);
		return -1;
	}

	llao = (Lladdropt *)buf[n];
	n = snprint(abuf, sizeof abuf, "add ether %I %E", rs->src, llao->lladdr);
	if (write(arpfd, abuf, n) < n) {
		ralog("recvrs: can't write to %s/arp: %r", conf.mpoint);
		close(arpfd);
		return -1;
	}

	memmove(sol, rs->src, IPaddrlen);
	close(arpfd);
	return 1;
}

void
sendra(int fd, uchar *dst, int rlt)
{
	int pktsz, preflen;
	char abuf[1024], tmp[40];
	uchar buf[1024], macaddr[6], src[IPaddrlen];
	Ipifc *ifc = nil;
	Iplifc *lifc, *nlifc;
	Lladdropt *llao;
	Prefixopt *prfo;
	Routeradv *ra;

	memset(buf, 0, sizeof buf);
	ra = (Routeradv *)buf;

	myetheraddr(macaddr, conf.dev);
	ea2lla(src, macaddr);
	memmove(ra->src, src, IPaddrlen);
	memmove(ra->dst, dst, IPaddrlen);
	ra->type = ICMP6_RA;
	ra->cttl = conf.ttl;

	if (conf.mflag > 0)
		ra->mor |= MFMASK;
	if (conf.oflag > 0)
		ra->mor |= OCMASK;
	if (rlt > 0)
		hnputs(ra->routerlt, conf.routerlt);
	else
		hnputs(ra->routerlt, 0);
	hnputl(ra->rchbltime, conf.reachtime);
	hnputl(ra->rxmtimer, conf.rxmitra);

	pktsz = sizeof *ra;

	/* include all global unicast prefixes on interface in prefix options */
	ifc = readipifc(conf.mpoint, ifc, myifc);
	for (lifc = (ifc? ifc->lifc: nil); lifc; lifc = nlifc) {
		nlifc = lifc->next;
		prfo = (Prefixopt *)(buf + pktsz);
		if (isv6global(lifc->ip)) {
			memmove(prfo->pref, lifc->net, IPaddrlen);

			/* hack to find prefix length */
			snprint(tmp, sizeof tmp, "%M", lifc->mask);
			preflen = atoi(&tmp[1]);
			prfo->plen = preflen & 0xff;
			if (prfo->plen == 0)
				continue;

			prfo->type = PREFIX_INFO;
			prfo->len = 4;
			prfo->lar = AFMASK;
			hnputl(prfo->validlt, lifc->validlt);
			hnputl(prfo->preflt, lifc->preflt);
			pktsz += sizeof *prfo;
		}
	}
	/*
	 * include link layer address (mac address for now) in
	 * link layer address option
	 */
	llao = (Lladdropt *)(buf + pktsz);
	llao->type = SRC_LLADDR;
	llao->len = 1;
	memmove(llao->lladdr, macaddr, sizeof macaddr);
	pktsz += sizeof *llao;

	pkt2str(buf+40, buf+pktsz, abuf, abuf+1024);
	if(write(fd, buf, pktsz) < pktsz)
		ralog("ra wr fail %s", abuf);
	else
		ralog("ra wr succ %s", abuf);
}

/*
 * daemon to send router advertisements
 */
void
sendra6(void)
{
	int fd, cfd, n, dstknown = 0, sendracnt, sleepfor, nquitmsgs;
	long lastra, ctime;
	uchar buf[4096], dst[IPaddrlen];
	Ipifc *ifc = nil;

	fd = dialicmp(v6allnodesL, ICMP6_RS, &cfd);
	if (fd < 0)
		sysfatal("can't open icmp_rs connection: %r");

	notify(catch);
	sendracnt = MAX_INIT_RTR_ADVERTS;
	nquitmsgs = MAX_FINAL_RTR_ADVERTS;

	switch(rfork(RFPROC|RFMEM|RFFDG|RFNOWAIT|RFNOTEG)){
	case -1:
		sysfatal("can't fork: %r");
	default:
		return;
	case 0:
		procsetname("sendra6 on %s", conf.dev);
		ralog("sendra6 on %s", conf.dev);
		sleepfor = nrand(10);
		for (;;) {
			lastra = time(0);
			alarm(sleepfor);
			n = read(fd, buf, sizeof buf);
			alarm(0);

			ifc = readipifc(conf.mpoint, ifc, myifc);
			if (ifc == nil) {
				ralog("sendra6: unable to read router pars on %s",
					conf.mpoint);
				continue;
			}

			if (ifc->sendra6 <= 0)
				if (nquitmsgs > 0) {
					sendra(fd, v6allnodesL, 0);
					nquitmsgs--;
					sleepfor = MIN_DELAY_BETWEEN_RAS +
						nrand(10);
					continue;
				} else {
					ralog("sendra6: quitting");
					exits(0);
				}

			nquitmsgs = MAX_FINAL_RTR_ADVERTS;

			if (n <= 0) {		/* no router solicitations */
				if (sendracnt > 0)
					sendracnt--;
				sleepfor = ifc->rp.minraint +
					nrand(ifc->rp.maxraint + 1 -
						ifc->rp.minraint);
			} else {		/* respond to router solicit'n */
				dstknown = recvrs(buf, n, dst);
				ctime = time(0);

				if (ctime - lastra < MIN_DELAY_BETWEEN_RAS) {
					/* too close, skip */
					sleepfor = lastra +
						MIN_DELAY_BETWEEN_RAS +
						nrand(10) - ctime;
					continue;
				}
				sleepfor = ifc->rp.minraint +
					nrand(ifc->rp.maxraint + 1 -
						ifc->rp.minraint);
				sleep(nrand(10));
			}
			if (dstknown > 0)
				sendra(fd, dst, 1);
			else
				sendra(fd, v6allnodesL, 1);
		}
	}
}

void
ra6(void)
{
	static char routeon[] = "iprouting 1";

	if (conf.recvra > 0)
		recvra6();

	if (conf.sendra > 0) {
		if (write(conf.cfd, routeon, sizeof routeon - 1) < 0) {
			fprint(2, "%s: write (iprouting 1) failed\n", argv0);
			return;
		}
		sendra6();
		if (conf.recvra <= 0)
			recvra6();
	}
}

void
dov6stuff(int what)
{
	char buf[256];
	int n;

	nip = nipifcs(conf.mpoint);
	if(!noconfig){
		lookforip(conf.mpoint);
		controldevice();
		binddevice();
	}

	switch (what) {
	default:
		fprint(2, "%s: unknown IPv6 verb\n", argv0);
		break;
	case Vaddpref6:
		n = snprint(buf, sizeof buf, "addpref6 %I %d %d %d %uld %uld",
			conf.v6pref, conf.prefixlen, conf.onlink, conf.autoflag,
			conf.validlt, conf.preflt);
		if (write(conf.cfd, buf, n) < n)
			fprint(2, "%s: write (%s) failed\n", argv0, buf);
		break;
	case Vra6:
		n = snprint(buf, sizeof buf,
			"ra6 sendra %d recvra %d mflag %d oflag %d"
			" maxraint %d minraint %d linkmtu %d reachtime %d"
			" rxmitra %d ttl %d routerlt %d",
			conf.sendra, conf.recvra, conf.mflag, conf.oflag,
			conf.maxraint, conf.minraint, conf.linkmtu,
			conf.reachtime, conf.rxmitra, conf.ttl, conf.routerlt);
		if (write(conf.cfd, buf, n) < n)
			fprint(2, "%s: write (%s) failed\n", argv0, buf);
		ra6();
		break;
	}
}
