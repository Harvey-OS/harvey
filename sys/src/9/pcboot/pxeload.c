/*
 * 9boot - load next kernel via pxe (bootp, tftp) and start it
 *
 * intel says that pxe can only load into the bottom 640K,
 * and intel's boot agent takes 128K, leaving only 512K for 9boot.
 *
 * some of this code is from the old 9load's bootp.c.
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"pool.h"
#include	"../port/netif.h"
#include	"etherif.h"
#include	"../ip/ip.h"
#include	"pxe.h"

#define TFTPDEF "135.104.9.6"	/* IP of default tftp server */

enum {
	Tftpusehdrs =	0,	/* flag: use announce+headers for tftp? */
	Debug =		0,

	Tftphdrsz =	4,
	/*
	 * this can be bigger than the ether mtu and
	 * will work due to ip fragmentation, at least on v4.
	 */
	Prefsegsize =	1400,
	Maxsegsize =	2048,
	Bufsz =		Maxsegsize + 2,
};

typedef struct Ethaddr Ethaddr;
typedef struct Kernname Kernname;
typedef struct Openeth Openeth;
typedef struct Tftp Tftp;

struct Tftp {
	uchar	header[Tftphdrsz];
	uchar	data[Maxsegsize];
};

struct Kernname {
	char	*edev;
	char	*bootfile;
};

struct Openeth {
	/* names */
	int	ctlrno;
	char	ethname[16];	/* ether%d */
	char	netethname[32];	/* /net/ether%d */
	char	filename[128];	/* from bootp, for tftp */

	Chan	*ifcctl;	/* /net/ipifc/clone */
	Chan	*ethctl;	/* /net/etherN/0/ctl, for promiscuous mode */

	/* udp connection */
	Chan	*udpctl;
	Chan	*udpdata;
	Pxenetaddr *netaddr;
	int	rxactive;
};

struct Ethaddr {		/* communication with sleep procs */
	Openeth	*oe;
	Pxenetaddr *a;
};

static char ethernm[] = "ether";

/*
 * there can be at most one concurrent tftp session until we move these
 * variables into Openeth or some other struct.
 */
static ushort tftpport;
static int tftpblockno;
static int tftpphase;
static int progress;
static int segsize;
static Tftp *tftpb;

static uchar myea[Eaddrlen];
static Pxenetaddr myaddr;		/* actually, local ip addr & port */
static Pxenetaddr tftpserv;		/* actually, remote ip addr & port */
static Pxenetaddr bootpserv;

uchar *
etheraddr(Openeth *oe)
{
	int n;
	char name[32], buf[32];
	uchar ea[Eaddrlen];

	memset(ea, 0, sizeof ea);
	snprint(name, sizeof name, "#l%d/ether%d/addr", oe->ctlrno, oe->ctlrno);
	n = readfile(name, buf, sizeof buf - 1);
	if (n < 0)
		return ea;
	buf[n] = '\0';
	parseether(ea, buf);
	return ea;
}

static void
udpsend(Openeth *oe, Pxenetaddr *a, void *data, int dlen)
{
	int n;
	uchar *buf;
	Chan *c;
	Etherpkt pkt;
	Udphdr *uh;

	buf = data;
	if (dlen > sizeof pkt)
		panic("udpsend: packet too big");

	oe->netaddr = a;
	/*
	 * add Plan 9 UDP pseudo-headers
	 */
	if (!tftpphase || Tftpusehdrs) {
		memset(&pkt, 0, sizeof pkt);
		uh = (Udphdr*)&pkt;
		memmove(uh + 1, data, dlen);
		USED(buf);
		buf = (uchar *)uh;
		dlen += sizeof *uh;
		if (dlen > sizeof pkt)
			panic("udpsend: packet too big");

		ipmove(uh->laddr, myaddr.ip);
		hnputs(uh->lport, myaddr.port);
		ipmove(uh->raddr, a->ip);
		hnputs(uh->rport, a->port);
		if(Debug)
			print("udpsend %I!%d -> %I!%d ", uh->laddr,
				nhgets(uh->lport), uh->raddr, nhgets(uh->rport));
	}
	if (waserror()) {
		iprint("udp write error\n");
		return;			/* send another req later */
	}
	c = oe->udpdata;
	assert(oe->udpdata != nil);
	n = devtab[c->type]->write(c, buf, dlen, c->offset);
	poperror();
	c->offset += n;
	if (n != dlen)
		print("udpsend: wrote %d/%d\n", n, dlen);
	else if (progress)
		print(".");
}

static void
nak(Openeth *oe, Pxenetaddr *a, int code, char *msg, int report)
{
	char buf[4 + 32];

	buf[0] = 0;
	buf[1] = Tftp_ERROR;
	buf[2] = 0;
	buf[3] = code;
	strncpy(buf+4, msg, sizeof buf - 4 - 1);
	udpsend(oe, a, buf, 4 + strlen(buf+4) + 1);
	if(report)
		print("\ntftp: error(%d): %s\n", code, msg);
}

/* a is the source address we're looking for */
static int
tuplematch(Pxenetaddr *a, Udphdr *h)
{
	int port;
	uchar *ip;

	if (tftpphase && !Tftpusehdrs)
		return 1;
	/*
	 * we're using udp headers mode, because we're still doing bootp,
	 * or we are doing tftp and we chose to use headers mode.
	 */
	port = a->port;
	ip = a->ip;
	/*
	 * we're accepting any src port or it's from the port we want, and
	 * it's from the ip we want or we sent to a broadcast address, and
	 * it's for us or it's a broadcast.
	 */
	return (port == 0 || nhgets(h->rport) == port) &&
		(equivip6(h->raddr, ip) || equivip6(ip, IPv4bcast)) &&
		(equivip6(h->laddr, myaddr.ip) || equivip6(h->laddr, IPv4bcast));
}

/* extract UDP payload into data and set a */
static int
udppayload(Udphdr *h, int len, Pxenetaddr *a, uchar *data, int dlen)
{
	if(Debug)
		print("udprecv %I!%d to %I!%d...\n",
			h->raddr, nhgets(h->rport), h->laddr, nhgets(h->lport));

	if(a->port != 0 && nhgets(h->rport) != a->port) {
		if(Debug)
			print("udpport %ux not %ux\n", nhgets(h->rport), a->port);
		return -1;
	}

	if(!equivip6(a->ip, IPv4bcast) && !equivip6(a->ip, h->raddr)) {
		if(Debug)
			print("bad ip %I not %I\n", h->raddr, a->ip);
		return -1;
	}

	len -= sizeof *h;		/* don't count pseudo-headers */
	if(len > dlen) {
		print("udp packet too big: %d > %d; from addr %I\n",
			len, dlen, h->raddr);
		return -1;
	}
	memmove(data, h + 1, len);	/* skip pseudo-headers */

	/* set a from remote address */
	ipmove(a->ip, h->raddr);
	a->port = nhgets(h->rport);
	return len;
}

static int
chanlen(Chan *ch)
{
	int len;
	Dir *dp;

	dp = dirchstat(ch);
	if (dp == nil)
		return -1;
	len = dp->length;		/* qlen(cv->rq) in devip */
	free(dp);
	return len;
}

static int
udprecv(Openeth *oe, Pxenetaddr *a, void *data, int dlen)
{
	int len, buflen, chlen;
	ulong timo, now;
	char *buf;
	Chan *c;
	Etherpkt pkt;

	oe->netaddr = a;
	/* timo is frequency of tftp ack and broadcast bootp retransmission */
	if(oe->rxactive == 0)
		timo = 1000;
	else
		timo = Timeout;
	now = TK2MS(m->ticks);
	timo += now;			/* deadline */

	c = oe->udpdata;
	spllo();			/* paranoia */
	do {
		/*
		 * wait for data to arrive or time-out.
		 * alarms only work for user procs, so we poll to avoid getting
		 * stuck in ipread.
		 */
		for (chlen = chanlen(c); chlen == 0 && now < timo;
		     chlen = chanlen(c)) {
			/* briefly give somebody else a chance to run */
			tsleep(&up->sleep, return0, 0, 0);
			now = TK2MS(m->ticks);
		}
		if (chlen <= 0) {
			print("T");
			return -1;		/* timed out */
		}

		while (waserror()) {
			print("read err: %s\n", up->errstr);
			tsleep(&up->sleep, return0, 0, 1000);
		}

		/*
		 * using Plan 9 UDP pseudo-headers?
		 */
		if (tftpphase && !Tftpusehdrs) {
			buf = data;	/* read directly in caller's buffer */
			buflen = dlen;
		} else {
			buf = (char *)&pkt;  /* read pkt with hdrs */
			buflen = sizeof pkt;
		}
		/* devtab[c->type]->read calls ipread */
		len = devtab[c->type]->read(c, buf, buflen, c->offset);
		poperror();

		if (len <= 0)
			return len;
		c->offset += len;
	} while (!tuplematch(oe->netaddr, (Udphdr *)buf));

	/*
	 * using Plan 9 UDP pseudo-headers? extract payload into caller's buf.
	 */
	if (!tftpphase || Tftpusehdrs)
		len = udppayload((Udphdr *)&pkt, len, a, data, dlen);
	if (len >= 0)
		oe->rxactive = 1;
	return len;
}

static void
ack(Openeth *oe, Pxenetaddr *a, int blkno)
{
	char buf[4];

	buf[0] = 0;
	buf[1] = Tftp_ACK;
	buf[2] = blkno>>8;
	buf[3] = blkno;
	udpsend(oe, a, buf, sizeof buf);
}

static char *
skipwd(char *wd)
{
	while (*wd != '\0')
		wd++;
	return wd + 1;		/* skip terminating NUL */
}

static int
optval(char *opt, char *pkt, int len)
{
	char *wd, *ep, *p;

	ep = pkt + len;
	for (p = pkt; p < ep && *p != '\0'; p = skipwd(wd)) {
		wd = skipwd(p);
		if (cistrcmp(p, opt) == 0)
			return strtol(wd, 0, 10);
	}
	return -1;
}

/*
 * send a tftp read request to `a' for name.  if we get a data packet back,
 * ack it and stash it in tftp for later.
 *
 * format of a request packet, from the RFC:
 *
 *          2 bytes     string    1 byte     string   1 byte
 *          ------------------------------------------------
 *         | Opcode |  Filename  |   0  |    Mode    |   0  |
 *          ------------------------------------------------
 */
static int
tftpread1st(Openeth *oe, Pxenetaddr *a, char *name, Tftp *tftp)
{
	int i, n, len, rlen, oport, sendack;
	static char *buf;

	if (buf == nil)
		buf = malloc(Bufsz);
	buf[0] = 0;
	buf[1] = Tftp_READ;
	len = 2 + snprint(buf+2, Bufsz - 2, "%s", name) + 1;
	len += snprint(buf+len, Bufsz - len, "octet") + 1;
	len += snprint(buf+len, Bufsz - len, "blksize") + 1; /* option */
	len += snprint(buf+len, Bufsz - len, "%d", Prefsegsize) + 1;

	/*
	 * keep sending the same packet until we get an answer.
	 */
	if (Debug)
		print("tftpread1st %s\n", name);
	oe->netaddr = a;
	/*
	 * the first packet or two sent seem to get dropped,
	 * so use a shorter time-out on the first packet.
	 */
	oe->rxactive = 0;
	oport = a->port;
	tftpblockno = 0;
	segsize = Defsegsize;
	sendack = 0;
	for(i = 0; i < 10; i++){
		a->port = oport;
		if (sendack)
			ack(oe, a, tftpblockno);
		else
			udpsend(oe, a, buf, len);	/* tftp read name */

		if((rlen = udprecv(oe, a, tftp, sizeof(Tftp))) < Tftphdrsz)
			continue;		/* runt or time-out */

		switch((tftp->header[0]<<8)|tftp->header[1]){

		case Tftp_ERROR:
			print("tftpread1st: error (%d): %s\n",
				(tftp->header[2]<<8)|tftp->header[3], (char*)tftp->data);
			return -1;

		case Tftp_OACK:
			n = optval("blksize", (char *)tftp->header+2, rlen-2);
			if (n <= 0) {
				nak(oe, a, 0, "bad blksize option value", 0);
				return -1;
			}
			segsize = n;
			/* no bytes stashed in tftp.data */
			i = 0;
			sendack = 1;
			break;

		case Tftp_DATA:
			tftpblockno = 1;
			len = (tftp->header[2]<<8)|tftp->header[3];
			if(len != tftpblockno){
				print("tftpread1st: block error: %d\n", len);
				nak(oe, a, 1, "block error", 0);
				return -1;
			}
			rlen -= Tftphdrsz;
			if(rlen < segsize)
				/* ACK now, in case we don't later */
				ack(oe, a, tftpblockno);
			return rlen;

		default:
			print("tftpread1st: unexpected pkt type recv'd\n");
			nak(oe, a, 0, "unexpected pkt type recv'd", 0);
			return -1;
		}
	}

	print("tftpread1st: failed to connect to server (%I!%d)\n", a->ip, oport);
	return -1;
}

static int
tftpread(Openeth *oe, Pxenetaddr *a, Tftp *tftp, int dlen)
{
	int try, blockno, len;

	dlen += Tftphdrsz;

	/*
	 * keep sending ACKs until we get an answer.
	 */
	for(try = 0; try < 10; try++) {
		ack(oe, a, tftpblockno);

		len = udprecv(oe, a, tftp, dlen);
		/*
		 * NB: not `<='; just a header is legal and happens when
		 * file being read is a multiple of segsize bytes long.
		 */
		if(len < Tftphdrsz){
			if(Debug)
				print("tftpread: too short %d <= %d\n",
					len, Tftphdrsz);
			continue;
		}
		switch((tftp->header[0]<<8)|tftp->header[1]){
		case Tftp_ERROR:
			print("tftpread: error (blk %d): %s\n",
				(tftp->header[2]<<8)|tftp->header[3],
				(char*)tftp->data);
			nak(oe, a, 0, "error pkt recv'd", 0);
			return -1;
		case Tftp_OACK:
			print("tftpread: oack pkt recv'd too late\n");
			nak(oe, a, 0, "oack pkt recv'd too late", 0);
			return -1;
		default:
			print("tftpread: unexpected pkt type recv'd\n");
			nak(oe, a, 0, "unexpected pkt type recv'd", 0);
			return -1;
		case Tftp_DATA:
			break;
		}
		blockno = (tftp->header[2]<<8)|tftp->header[3];
		if(blockno <= tftpblockno){
			if(Debug)
				print("tftpread: blkno %d <= %d\n",
					blockno, tftpblockno);
			continue;
		}

		if(blockno == tftpblockno+1) {
			tftpblockno++;
			if(len < dlen)	/* last packet? send final ack */
				ack(oe, a, tftpblockno);
			return len-Tftphdrsz;
		}
		print("tftpread: block error: %d, expected %d\n",
			blockno, tftpblockno+1);
	}

	return -1;
}

/*
 * broadcast a bootp request for file.  stash any answer in rep.
 */
static int
bootpbcast(Openeth *oe, char *file, Bootp *rep)
{
	Bootp req;
	int i;
	uchar *ea;
	char name[128], *filename, *sysname;
	static char zeroes[IPaddrlen];

	oe->filename[0] = '\0';
	if (Debug)
		if (file == nil)
			print("bootpopen: %s...", oe->ethname);
		else
			print("bootpopen: %s!%s...", oe->ethname, file);
	if((ea = etheraddr(oe)) == nil){
		print("bad ether %s\n", oe->ethname);
		return -1;
	}

	filename = nil;
	sysname = 0;
	if(file && *file){
		strncpy(name, file, sizeof name);
		if(filename = strchr(name, '!')){
			sysname = name;
			*filename++ = 0;
		}
		else
			filename = name;
	}

	/*
	 * form a bootp request packet
	 */
	memset(&req, 0, sizeof(req));
	req.op = Bootrequest;
	req.htype = 1;			/* ethernet */
	req.hlen = Eaddrlen;		/* ethernet */
	memmove(req.chaddr, ea, Eaddrlen);
	req.flags[0] = 0x80;		/* request broadcast reply */
	if(filename != nil) {
		strncpy(req.file, filename, sizeof(req.file));
		strncpy(oe->filename, filename, sizeof oe->filename);
	}
	if(sysname != nil)		/* if server name given, supply it */
		strncpy(req.sname, sysname, sizeof(req.sname));

	if (memcmp(myaddr.ip, zeroes, sizeof myaddr.ip) == 0)
		ipmove(myaddr.ip, IPv4bcast);	/* didn't know my ip yet */
	myaddr.port = BPportsrc;
	memmove(myea, ea, Eaddrlen);

	/* send to 255.255.255.255!67 */
	ipmove(bootpserv.ip, IPv4bcast);
	bootpserv.port = BPportdst;

	/*
	 * send it until we get a matching answer
	 */
	memset(rep, 0, sizeof *rep);
	for(i = 10; i > 0; i--) {
		req.xid[0] = i;			/* try different xids */
		udpsend(oe, &bootpserv, &req, sizeof(req));

		if(udprecv(oe, &bootpserv, rep, sizeof(*rep)) <= 0)
			continue;
		if(memcmp(req.chaddr, rep->chaddr, Eaddrlen) != 0)
			continue;
		if(rep->htype != 1 || rep->hlen != Eaddrlen)
			continue;
		if(sysname == 0 || strcmp(sysname, rep->sname) == 0)
			break;
	}
	if(i <= 0) {
		if (file == nil)
			print("bootp on %s timed out\n", oe->ethname);
		else
			print("bootp on %s for %s timed out\n", oe->ethname, file);
		return -1;
	}
	return 0;
}

/*
 * request file via tftp from server named in rep.
 * initial data packet will be stashed in tftpb.
 */
static int
tftpopen(Openeth *oe, char *file, Bootp *rep)
{
	char *filename;
	char buf[128];
	static uchar ipv4noaddr[IPv4addrlen];

	/*
	 * read file from tftp server in bootp answer
	 */
	filename = oe->filename;
	if (file)
		filename = file;
	if(filename == 0 || *filename == 0){
		if(strcmp(rep->file, "/386/9boot") == 0 ||
		   strcmp(rep->file, "/386/9pxeload") == 0) {
			print("won't load another boot loader (%s)\n", rep->file);
			return -1;		/* avoid infinite loop */
		}
		filename = rep->file;
	}

	print("\n");
	if(rep->sname[0] != '\0')
		print("%s ", rep->sname);

	v4tov6(myaddr.ip, rep->yiaddr);
	myaddr.port = tftpport;
	if (equivip4(rep->siaddr, ipv4noaddr)) { /* no server address? */
		getstr("tftp server IP address", buf, sizeof buf, TFTPDEF, 0);
		v4parseip(rep->siaddr, buf);
	}
	v4tov6(tftpserv.ip, rep->siaddr);
	tftpserv.port = TFTPport;
	if (tftpb == nil)
		tftpb = malloc(sizeof *tftpb);

	print("(%V!%d): %s ", rep->siaddr, tftpserv.port, filename);

	return tftpread1st(oe, &tftpserv, filename, tftpb);
}

int
tftpboot(Openeth *oe, char *file, Bootp *rep, Boot *b)
{
	int n;

	if((n = tftpopen(oe, file, rep)) < 0)
		return -1;

	progress = 0;			/* no more dots; we're on a roll now */
	print(" ");			/* after "sys (ip!port): kernel ..." */
	while(bootpass(b, tftpb->data, n) == MORE){
		n = tftpread(oe, &tftpserv, tftpb, segsize);
		if(n < segsize)
			break;
	}
	if(0 < n && n < segsize)	/* got to end of file */
		bootpass(b, tftpb->data, n);
	else
		nak(oe, &tftpserv, 3, "ok", 0);	/* tftpclose to abort transfer */
	bootpass(b, nil, 0);	/* boot if possible */
	return -1;
}

/* leave the channel to /net/ipifc/clone open */
static int
binddevip(Openeth *oe)
{
	Chan *icc;
	char buf[32];

	if (waserror()) {
		print("binddevip: can't bind ether %s: %s\n",
			oe->netethname, up->errstr);
		nexterror();
	}
	/* get a new ip interface */
	oe->ifcctl = icc = namecopen("/net/ipifc/clone", ORDWR);
	if(icc == nil)
		error("can't open /net/ipifc/clone");

	/*
	 * specify medium as ethernet, bind the interface to it.
	 * this should trigger chandial of types 0x800, 0x806 and 0x86dd.
	 */
	snprint(buf, sizeof buf, "bind ether %s", oe->netethname);
	devtab[icc->type]->write(icc, buf, strlen(buf), 0);  /* bind ether %s */
	poperror();
	return 0;
}

/* set the default route */
static int
adddefroute(char *, uchar *gaddr)
{
	char buf[64];
	Chan *rc;

	rc = nil;
	if (waserror()) {
		if (rc)
			cclose(rc);
		return -1;
	}
	rc = enamecopen("/net/iproute", ORDWR);

	if(isv4(gaddr))
		snprint(buf, sizeof buf, "add 0 0 %I", gaddr);
	else
		snprint(buf, sizeof buf, "add :: /0 %I", gaddr);
	devtab[rc->type]->write(rc, buf, strlen(buf), 0);
	poperror();
	cclose(rc);
	return 0;
}

static int
validip(uchar *ip)
{
	return ipcmp(ip, IPnoaddr) != 0 && ipcmp(ip, v4prefix) != 0;
}

static int
openetherdev(Openeth *oe)
{
	int n;
	char num[16];
	Chan *c;
	static char promisc[] = "promiscuous";

	if (chdir(oe->netethname) < 0)
		return -1;			/* out of ethers */

	oe->ethctl = nil;
	if (waserror()) {
		print("error opening /net/ether%d/0/ctl: %s\n",
			oe->ctlrno, up->errstr);
		if (oe->ethctl) {
			cclose(oe->ethctl);
			oe->ethctl = nil;
		}
		chdir("/");			/* don't hold conv. open */
		return -1;
	}
	oe->ethctl = c = namecopen("0/ctl", ORDWR);	/* should be ipv4 */
	if (c == nil) {
		/* read clone file to make conversation 0 since not present */
		oe->ethctl = c = enamecopen("clone", ORDWR);
		n = devtab[c->type]->read(c, num, sizeof num - 1, 0);
		if (n < 0)
			print("no %s/clone: %s\n", oe->netethname, up->errstr);
		else {
			num[n] = 0;
			print("%s/clone returned %s\n", oe->netethname, num);
		}
	}
	/* shouldn't be needed to read bootp (broadcast) reply */
	devtab[c->type]->write(c, promisc, sizeof promisc-1, 0);
	poperror();
	chdir("/");
	/* leave oe->ethctl open to keep promiscuous mode on */
	return 0;
}

/* add a logical interface to the ip stack */
int
minip4cfg(Openeth *oe)
{
	int n;
	char buf[64];

	n = snprint(buf, sizeof buf, "add %I", IPnoaddr);
	devtab[oe->ifcctl->type]->write(oe->ifcctl, buf, n, 0);	/* add %I */

	openetherdev(oe);
	return 0;
}

/* remove the :: address added by minip4cfg */
int
unminip4cfg(Openeth *oe)
{
	int n;
	char buf[64];

	n = snprint(buf, sizeof buf, "remove %I /128", IPnoaddr);
	if (waserror()) {
		print("failed write to ifc: %s: %s\n", buf, up->errstr);
		return -1;
	}
	devtab[oe->ifcctl->type]->write(oe->ifcctl, buf, n, 0);	/* remove %I */
	cclose(oe->ethctl);		/* turn promiscuous mode off */
	oe->ethctl = nil;
	poperror();
	return 0;
}

/*
 * parse p, looking for option `op'.  if non-nil, np points to minimum length.
 * return nil if option is too small, else ptr to opt, and
 * store actual length via np if non-nil.
 */
uchar*
optget(uchar *p, int op, int *np)
{
	int len, code;

	while ((code = *p++) != OBend) {
		if(code == OBpad)
			continue;
		len = *p++;
		if(code != op) {
			p += len;
			continue;
		}
		if(np != nil){
			if(*np > len) {
				return 0;
			}
			*np = len;
		}
		return p;
	}
	return 0;
}

int
optgetaddr(uchar *p, int op, uchar *ip)
{
	int len;

	len = 4;
	p = optget(p, op, &len);
	if(p == nil)
		return 0;
	v4tov6(ip, p);
	return 1;
}

int beprimary = 1;

/* add a logical interface to the ip stack */
int
ip4cfg(Openeth *oe, Bootp *rep)
{
	int n;
	uchar gaddr[IPaddrlen], v6mask[IPaddrlen];
	uchar v4mask[IPv4addrlen];
	char buf[64];
	static uchar zeroes[4];

	v4tov6(gaddr, rep->yiaddr);
	if(!validip(gaddr))
		return -1;

	/* dig subnet mask, if any, out of options.  if none, guess. */
	if(optgetaddr(rep->optdata, OBmask, v6mask)) {
		v6tov4(v4mask, v6mask);
		n = snprint(buf, sizeof buf, "add %V %M", rep->yiaddr, v4mask);
	} else
		n = snprint(buf, sizeof buf, "add %V 255.255.255.0", rep->yiaddr);

	devtab[oe->ifcctl->type]->write(oe->ifcctl, buf, n, 0);

	v4tov6(gaddr, rep->giaddr);
	if(beprimary==1 && validip(gaddr) && !equivip4(rep->giaddr, zeroes))
		adddefroute("/net", gaddr);
	return 0;
}

static int
openudp(Openeth *oe)
{
	int n;
	char buf[16];
	Chan *cc;

	/* read clone file for conversation number */
	if (waserror())
		panic("openudp: can't open /net/udp/clone");
	cc = enamecopen("/net/udp/clone", ORDWR);
	oe->udpctl = cc;
	n = devtab[cc->type]->read(cc, buf, sizeof buf - 1, 0);
	poperror();
	buf[n] = '\0';
	return atoi(buf);
}

static void
initbind(Openeth *oe)
{
	char buf[8];

	if (waserror()) {
		print("error while binding: %s\n", up->errstr);
		return;
	}
	snprint(buf, sizeof buf, "#I%d", oe->ctlrno);
	bind(buf, "/net", MAFTER);
	snprint(buf, sizeof buf, "#l%d", oe->ctlrno);
	bind(buf, "/net", MAFTER);
	binddevip(oe);
	poperror();
}

static void
closeudp(Openeth *oe)
{
	if (oe->udpctl) {
		cclose(oe->udpctl);
		oe->udpctl = nil;
	}
	if (oe->udpdata) {
		cclose(oe->udpdata);
		oe->udpdata = nil;
	}
}

static int
announce(Openeth *oe, char *port)
{
	int udpconv;
	char buf[32];
	static char hdrs[] = "headers";

	while (waserror()) {
		print("can't announce udp!*!%s: %s\n", port, up->errstr);
		closeudp(oe);
		nexterror();
	}
	udpconv = openudp(oe);
	if (udpconv < 0)
		panic("can't open udp conversation: %s", up->errstr);

	/* headers is only effective after a udp announce */
	snprint(buf, sizeof buf, "announce %s", port);
	devtab[oe->udpctl->type]->write(oe->udpctl, buf, strlen(buf), 0);
	devtab[oe->udpctl->type]->write(oe->udpctl, hdrs, sizeof hdrs - 1, 0);
	poperror();

	/* now okay to open the data file */
	snprint(buf, sizeof buf, "/net/udp/%d/data", udpconv);
	/*
	 * we must use create, not open, to get Conv->rq and ->wq
	 * allocated by udpcreate.
	 */
	oe->udpdata = enameccreate(buf, ORDWR);
	cclose(oe->udpctl);
	oe->udpctl = nil;
	return udpconv;
}

static long
tftprdfile(Openeth *oe, int openread, void* va, long len)
{
	int n;
	char *p, *v;

	n = openread;	/* have read this many bytes already into tftpb->data */
	p = v = va;
	len--;				/* leave room for NUL */
	while(n > 0) {
		if((p-v)+n > len)
			n = len - (p-v);
		memmove(p, tftpb->data, n);
		p += n;
		*p = 0;
		if(n != segsize)
			break;

		if((n = tftpread(oe, &tftpserv, tftpb, segsize)) < 0)
			return -1;
	}
	return p-v;
}

static int
newtftpconn(Openeth *oe, Bootp *rep)
{
	char num[16], dialstr[64];

	if (waserror()) {
		print("can't dial: %s\n", up->errstr);
		return -1;
	}
	closeudp(oe);

	tftpphase = 1;
	tftpport = 5000 + nrand(20480);
	snprint(num, sizeof num, "%d", tftpport);
	if (Tftpusehdrs)
		announce(oe, num);
	else {
		snprint(dialstr, sizeof dialstr, "/net/udp!%V!%d",
			rep->siaddr, TFTPport);
		oe->udpdata = chandial(dialstr, num, nil, nil);
		oe->udpctl = nil;
	}
	poperror();
	return 0;
}

static int
setipcfg(Openeth *oe, Bootp *rep)
{
	int r;

	tftpphase = 0;
	progress = 1;

	/* /net/iproute is unpopulated here; add at least broadcast */
	minip4cfg(oe);
	announce(oe, "68");
	r = bootpbcast(oe, nil, rep);
	closeudp(oe);
	unminip4cfg(oe);
	if(r < 0)
		return -1;

	ip4cfg(oe, rep);
	if (Debug)
		print("got & set ip config\n");
	return 0;
}

static int
getkernname(Openeth *oe, Bootp *rep, Kernname *kp)
{
	int n;
	char *ini, *p;
	char cfgpxe[32], buf[64];

	if (kp->bootfile) {
		print("getkernname: already have bootfile %s\n", kp->bootfile);
		return 0;
	}
	if (newtftpconn(oe, rep) < 0)
		return -1;

	/* use our mac address instead of relying on a bootp answer */
	snprint(cfgpxe, sizeof cfgpxe, "/cfg/pxe/%E", myea);
	/*
	 * use bootp answer (rep) to open cfgpxe.
	 * reads first pkt of cfgpxe into tftpb->data.
	 */
	n = tftpopen(oe, cfgpxe, rep);
	if (n < 0) {
		print("\nfailed.\n");
		return -1;
	}
	if (Debug)
		print("\opened %s\n", cfgpxe);

	ini = smalloc(2*BOOTARGSLEN);
	/* starts by copying data from tftpb->data into ini */
	n = tftprdfile(oe, n, ini, 2*BOOTARGSLEN);
	if (n < 0) {
		print("error reading %s\n", cfgpxe);
		free(ini);
		return -1;
	}
	print(" read %d bytes", n);

	/*
	 * take note of plan9.ini contents.  consumes ini to make config vars,
	 * thus we can't free ini.
	 */
	dotini(ini);
	i8250console();		/* configure serial port with defaults */

	kp->edev = kp->bootfile = nil;
	p = getconf("bootfile");
	if (p)
		kstrdup(&kp->bootfile, p);
	if (kp->bootfile == nil)
		askbootfile(buf, sizeof buf, &kp->bootfile, Promptsecs,
			"ether0!/386/9pccpu");
	if (strcmp(kp->bootfile, "manual") == 0)
		askbootfile(buf, sizeof buf, &kp->bootfile, 0, "");

	p = strchr(kp->bootfile, '!');
	if (p != nil) {
		*p++ = '\0';
		kp->edev = kp->bootfile;
		kp->bootfile = nil;
		kstrdup(&kp->bootfile, p);
		if (strncmp(kp->edev, ethernm, sizeof ethernm - 1) != 0) {
			print("bad ether device %s\n", kp->edev);
			return -1;
		}
	}

	/* pass arguments to kernels that can use them */
	strecpy(BOOTLINE, BOOTLINE+BOOTLINELEN, kp->bootfile);
	p = strchr(kp->bootfile, ' ');
	if(p != nil)
		*p = '\0';
	return 0;
}

static void
unbinddevip(Openeth *oe)
{
	Chan *icc;
	static char unbind[] = "unbind";

	icc = oe->ifcctl;
	if (icc) {
		devtab[icc->type]->write(icc, unbind, sizeof unbind - 1, 0);
		cclose(icc);
		oe->ifcctl = nil;
	}
}

/*
 * phase 1: get our ip (v4) configuration via bootp, set new ip configuration.
 * phase 2: load /cfg/pxe, parse it, extract kernel filename.
 * phase 3: load kernel and jump to it.
 */
static void
tftpload(Openeth *oe, Kernname *kp)
{
	Bootp rep;
	Boot boot;

	if(waserror()) {
		print("tftpload: %s\n", up->errstr);
		closeudp(oe);
		unbinddevip(oe);
		return;
	}

	memset(&rep, 0, sizeof rep);
	if (setipcfg(oe, &rep) >= 0 &&
	    getkernname(oe, &rep, kp) >= 0 &&
	    (!kp->edev ||
	     oe->ctlrno == strtol(kp->edev + sizeof ethernm - 1, 0, 10)) &&
	    newtftpconn(oe, &rep) >= 0) {
		memset(&boot, 0, sizeof boot);
		boot.state = INITKERNEL;
		tftpboot(oe, kp->bootfile, &rep, &boot);
	}

	/* we failed or bootfile asked for another ether */
	poperror();
	closeudp(oe);
	unbinddevip(oe);
}

static int
etherload(int eth, Kernname *kp)
{
	Openeth *oe;

	print("pxe on ether%d ", eth);
	oe = smalloc(sizeof *oe);
	memset(oe, 0, sizeof *oe);
	oe->ctlrno = eth;
	snprint(oe->ethname, sizeof oe->ethname, "ether%d", oe->ctlrno);
	snprint(oe->netethname, sizeof oe->netethname, "/net/ether%d",
		oe->ctlrno);
	initbind(oe);

	tftpload(oe, kp);

	/* failed to boot; keep going */
	unmount(nil, "/net");
	return 0;
}

static int
attacheth(int neth)
{
	char num[4];
	Chan *cc;

	cc = nil;
	if (waserror()) {		/* no more interfaces */
		if (cc)
			cclose(cc);
		return -1;
	}
	snprint(num, sizeof num, "%d", neth);
	cc = etherattach(num);
	if (cc)
		cclose(cc);
	poperror();
	return cc == nil? -1: 0;
}

void
bootloadproc(void *)
{
	int eth, neth, needattach;
	Kernname kernnm;

	srand(TK2MS(m->ticks));			/* for local port numbers */
	nrand(20480);				/* 1st # is always 0; toss it */
	kernnm.edev = kernnm.bootfile = nil;

	while(waserror()) {
		print("%s\n", up->errstr);
		tsleep(&up->sleep, return0, 0, 30*1000);
	}
	neth = MaxEther;
	needattach = 1;
	for (;;) {
		/* try each interface in turn: first get /cfg/pxe file */
		for (eth = 0; eth < neth && kernnm.edev == nil; eth++) {
			if (needattach && attacheth(eth) < 0)
				break;
			etherload(eth, &kernnm);
		}
		if (needattach) {
			neth = eth;
			needattach = 0;
			if (neth == 0)
				print("no ethernet interfaces found\n");
		}
		if (kernnm.edev != nil) {
			eth = strtol(kernnm.edev + sizeof ethernm - 1, 0, 10);
			etherload(eth, &kernnm);
		}
		/*
		 * couldn't boot on any ether.  don't give up;
		 * perhaps the boot servers are down, so try again later.
		 */
		print("failed to boot via pxe; will try again.\n");
		tsleep(&up->sleep, return0, 0, 15*1000);
	}
}
