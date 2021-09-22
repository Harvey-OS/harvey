/*
 * 9boot - load next kernel via bootp and (t)tftp, and start it
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

#include	"../port/netif.h"
#include	"etherif.h"
#include	"../ip/ip.h"
#include	"pxe.h"
#include	"tftp.h"

enum {
	Debug =		0,
};

typedef struct Kernname Kernname;
struct Kernname {
	char	*edev;
	char	*bootfile;
};

typedef struct Ethaddr Ethaddr;
struct Ethaddr {		/* communication with sleep procs */
	Openeth	*oe;
	Pxenetaddr *a;
};

/* exported to tftp.c */
uchar myea[Eaddrlen];
Pxenetaddr myaddr;		/* actually, local ip addr & port */
Pxenetaddr tftpserv;		/* actually, remote ip addr & port */
int tftpphase;			/* odd that it's here, not in tftp.c */
int progress;

static char ethernm[] = "ether";
static Pxenetaddr bootpserv;

uchar *
etheraddr(Openeth *oe)
{
	int n;
	char name[32], buf[32];
	static uchar ea[Eaddrlen];

	memset(ea, 0, sizeof ea);
	snprint(name, sizeof name, "#l%d/ether%d/addr", oe->ctlrno, oe->ctlrno);
	n = readfile(name, buf, sizeof buf - 1);
	if (n < 0)
		return ea;
	buf[n] = '\0';
	parseether(ea, buf);
	return ea;
}

void
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

int
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
	now = TK2MS(sys->ticks);
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
			now = TK2MS(sys->ticks);
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

char *
filetoload(Openeth *oe, char *file, Bootp *rep)
{
	char *filename;

	/*
	 * read file from tftp server in bootp answer
	 */
	filename = oe->filename;
	if (file)
		filename = file;
	if(filename == nil || *filename == 0){
		if(strcmp(rep->file, "/386/9boot") == 0 ||
		   strcmp(rep->file, "/386/9pxeload") == 0) {
			print("won't load another boot loader (%s)\n", rep->file);
			return nil;		/* avoid infinite loop */
		}
		filename = rep->file;
	}
	return filename;
}

int
cprint(Chan *c, char *fmt, ...)
{
	int n;
	char buf[PRINTSIZE];
	va_list args;

	va_start(args, fmt);
	n = vsnprint(buf, sizeof buf, fmt, args);
	va_end(args);
	return devtab[c->type]->write(c, buf, n, c->offset);
}

/* request file via ttftp on oe */
Chan *
ttftpopen(Openeth *oe, char *file, Bootp *rep)
{
	int n;
	char *filename;
	char ds[80], dirno[32];
	Chan *tcpdir, *connctl, *conndata;

	tcpdir = conndata = connctl = nil;
	if(waserror()) {
		print("ttftpopen: %s\n", up->errstr);
		if (connctl)
			cclose(connctl);
		if (conndata)
			cclose(conndata);
		if (tcpdir)
			cclose(tcpdir);
		return nil;
	}

	/* allocate tcp connection */
	tcpdir = namecopen("/net/tcp/clone", OREAD);
	if (tcpdir == nil)
		error("no tcp conns");
	n = devtab[tcpdir->type]->read(tcpdir, dirno, sizeof dirno - 1, 0);
	if (n <= 0)
		error("out of tcp conns");
	dirno[n] = '\0';

	/* open its ctl and data channels */
	snprint(ds, sizeof ds, "/net/tcp/%s/ctl", dirno);
	connctl = namecopen(ds, ORDWR);
	snprint(ds, sizeof ds, "/net/tcp/%s/data", dirno);
	conndata = namecopen(ds, ORDWR);
	if (connctl == nil || conndata == nil)
		error("can't open tcp conn");
	cclose(tcpdir);

	/* dial ttftp port on fs */
	// print("dialing tcp!%V!17015...", rep->siaddr);
	cprint(connctl, "connect %V!17015", rep->siaddr);

	/* determine what file to load */
	filename = filetoload(oe, file, rep);
	if (filename == nil)
		error("can't choose file to load");

	/* write its name to ttftp server */
	// print("requesting %s...", filename);
	cprint(conndata, "%s\n", filename);
	cclose(connctl);

	poperror();
	print("ttftp %s", filename);
	prflush();
	return conndata;
}

/* load the kernel in file via ttftp on oe */
int
ttftpboot(Openeth *oe, char *file, Bootp *rep, Boot *b)
{
	int n, rv;
	Chan *conndata;
	static char buf[8192];

	conndata = ttftpopen(oe, file, rep);
	if (conndata == nil)
		return Err;

	/* load whatever the ttftp server gives us into memory */
	progress = 0;			/* no more dots; we're on a roll now */
	print(" ");			/* after "sys (ip!port): kernel ..." */
	while ((n = devtab[conndata->type]->read(conndata, buf, sizeof buf,
	    conndata->offset)) > 0) {
		if (bootpass(b, buf, n) != MORE)
			break;
	}
	if (n < 0) {
		print("error reading %s\n", file);
		rv = Err;
	} else
		rv = Ok;
	bootpass(b, nil, 0);		/* boot if possible */
	cclose(conndata);
	return rv;
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

static void
cclosez(Chan **chanp)
{
	if (chanp && *chanp) {
		cclose(*chanp);
		*chanp = nil;
	}
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
		cclosez(&oe->ethctl);
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
	cclosez(&oe->ethctl);		/* turn promiscuous mode off */
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
			if(*np > len)
				return 0;
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

void
closeudp(Openeth *oe)
{
	cclosez(&oe->udpctl);
	cclosez(&oe->udpdata);
}

int
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
	cclosez(&oe->udpctl);
	return udpconv;
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

/*
 * use bootp answer (rep) to open cfgpxe.
 * tftp reads first pkt of cfgpxe into tftpb->data.
 */
static int
rdcfgpxe(Openeth *oe, Bootp *rep, char *cfgpxe)
{
	int n;
	char *ini;
	Chan *conndata;

	ini = smalloc(2*BOOTARGSLEN);

	/* try ttftp first */
	conndata = ttftpopen(oe, cfgpxe, rep);
	if (conndata) {
		n = devtab[conndata->type]->read(conndata, ini, 2*BOOTARGSLEN-1, 0);
		cclose(conndata);
		if (n >= 0)
			ini[n] = '\0';
	} else {
		/* cfgpxe is optional */
		n = tftpopen(oe, cfgpxe, rep);
		if (n < 0)
			return n;
		if (Debug)
			print("\opened %s\n", cfgpxe);

		/* starts by copying data from tftpb->data into ini */
		n = tftprdfile(oe, n, ini, 2*BOOTARGSLEN);
	}
	if (n < 0) {
		print("error reading %s\n", cfgpxe);
		free(ini);
		return n;
	}
	print(" read %d bytes\n", n);

	/*
	 * take note of plan9.ini contents.  consumes ini to make config vars,
	 * thus we can't free ini.
	 */
	dotini(ini);
	return Ok;
}

/*
 * break kp->bootfile into kp->edev & kp->bootfile,
 * copy any args for new kernel to low memory.
 */
static int
parsebootfile(Kernname *kp)
{
	char *p;

	p = strchr(kp->bootfile, '!');
	if (p != nil) {
		*p++ = '\0';
		kp->edev = kp->bootfile;
		kp->bootfile = nil;
		kstrdup(&kp->bootfile, p);
		if (strncmp(kp->edev, ethernm, sizeof ethernm - 1) != 0) {
			print("bad ether device %s\n", kp->edev);
			return Err;
		}
	}

	/* pass any arguments to kernels that expect them */
	strecpy(BOOTLINE, BOOTLINE+BOOTLINELEN, kp->bootfile);
	p = strchr(kp->bootfile, ' ');
	if(p != nil)
		*p = '\0';
	return Ok;
}

static int
getkernname(Openeth *oe, Bootp *rep, Kernname *kp)
{
	int n;
	char *p;
	char cfgpxe[32], buf[64];

	if (kp->bootfile) {
		/* i think returning here is a bad idea */
		// print("getkernname: already have bootfile %s\n",
		//	kp->bootfile);
		free(kp->bootfile);
		// return Ok;
	}
	kp->edev = kp->bootfile = nil;
	i8250console();		/* configure serial port with defaults */

	/* use our mac address instead of relying on a bootp answer. */
	snprint(cfgpxe, sizeof cfgpxe, "/cfg/pxe/%E", myea);
	n = rdcfgpxe(oe, rep, cfgpxe);
	switch (n) {
	case Ok:
		p = getconf("bootfile");
		if (p)
			kstrdup(&kp->bootfile, p);
		if (kp->bootfile == nil)
			askbootfile(buf, sizeof buf, &kp->bootfile, Promptsecs,
				"ether0!/386/9pccpu");
		if (strcmp(kp->bootfile, "manual") == 0)
			askbootfile(buf, sizeof buf, &kp->bootfile, 0, "");
		break;
	case Err:
		print("\nfailed.\n");
		return n;
	case Nonexist:
		askbootfile(buf, sizeof buf, &kp->bootfile, 0, "");
		break;
	}
	return parsebootfile(kp);
}

static void
unbinddevip(Openeth *oe)
{
	Chan *icc;
	static char unbind[] = "unbind";

	icc = oe->ifcctl;
	if (icc) {
		devtab[icc->type]->write(icc, unbind, sizeof unbind - 1, 0);
		cclosez(&oe->ifcctl);
	}
}

static int
trytftpboot(Openeth *oe, Kernname *kp, Bootp *bootp, Boot *boot)
{
	memset(boot, 0, sizeof *boot);
	boot->state = INITKERNEL;
	return tftpboot(oe, kp->bootfile, bootp, boot);
}

/*
 * phase 1: get our ip (v4) configuration via bootp, set new ip configuration.
 * phase 2: load /cfg/pxe with (t)tftp, parse it, extract kernel filename.
 * phase 3: load kernel with (t)tftp and jump to it.
 */
static int
ipload(Openeth *oe, Kernname *kp)
{
	int r, n;
	char buf[64];
	Bootp rep;
	Boot boot;

	r = -1;
	if(waserror()) {
		print("ipload: %s\n", up->errstr);
		closeudp(oe);
		unbinddevip(oe);
		return r;
	}

	memset(&rep, 0, sizeof rep);
	if (setipcfg(oe, &rep) < 0)
		error("can't set ip config");

	n = getkernname(oe, &rep, kp);
	if (n < 0) {
		r = n;			/* pass reason back to caller */
		USED(r);
		nexterror();
	}
	do {
		if (kp->edev &&
		    oe->ctlrno != strtol(kp->edev + sizeof ethernm - 1, 0, 10)){
			/* user specified an ether & it's not this one; next! */
			r = Ok;
			USED(r);
			nexterror();
		}

		memset(&boot, 0, sizeof boot);
		boot.state = INITKERNEL;
		if(waserror())
			r = trytftpboot(oe, kp, &rep, &boot);
		else {
			r = ttftpboot(oe, kp->bootfile, &rep, &boot);
			poperror();
			if (r != Ok)
				r = trytftpboot(oe, kp, &rep, &boot);
		}
		
		/* we failed or bootfile asked for another ether */
		if (r == Nonexist)
			do {
				askbootfile(buf, sizeof buf, &kp->bootfile, 0, "");
			} while (parsebootfile(kp) != Ok);
	} while (r == Nonexist);

	poperror();
	closeudp(oe);
	unbinddevip(oe);
	return r;
}

static int
etherload(int eth, Kernname *kp)
{
	int r;
	Openeth *oe;

	print("bootp on ether%d ", eth);
	oe = smalloc(sizeof *oe);
	memset(oe, 0, sizeof *oe);
	oe->ctlrno = eth;
	snprint(oe->ethname, sizeof oe->ethname, "ether%d", oe->ctlrno);
	snprint(oe->netethname, sizeof oe->netethname, "/net/ether%d",
		oe->ctlrno);
	initbind(oe);

	r = ipload(oe, kp);

	/* failed to boot; keep going */
	unmount(nil, "/net");
	return r;
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

	tftprandinit();
	kernnm.edev = kernnm.bootfile = nil;

	while(waserror()) {
		print("bootloadproc: %s\n", up->errstr);
		tsleep(&up->sleep, return0, 0, 25*1000);
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
			if (eth == 0)
				break;
			neth = eth;
			needattach = 0;
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
		tsleep(&up->sleep, return0, 0, 25*1000);
	}
	print("no known ethernet interfaces found\n");
	pcihinv(nil);
	for(;;)
		tsleep(&up->sleep, return0, 0, 1000);
}
