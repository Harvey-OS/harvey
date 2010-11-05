/*
 * tftpd - tftp service, see /lib/rfc/rfc783 (now rfc1350 + 234[789])
 */
#include <u.h>
#include <libc.h>
#include <auth.h>
#include <bio.h>
#include <ip.h>
#include <ndb.h>

enum
{
	Maxpath=	128,
	Maxerr=		256,

	Debug=		0,

	Opsize=		sizeof(short),
	Blksize=	sizeof(short),
	Hdrsize=	Opsize + Blksize,

	Ackerr=		-1,
	Ackok=		0,
	Ackrexmit=	1,

	/* op codes */
	Tftp_READ	= 1,
	Tftp_WRITE	= 2,
	Tftp_DATA	= 3,
	Tftp_ACK	= 4,
	Tftp_ERROR	= 5,
	Tftp_OACK	= 6,		/* option acknowledge */

	Errnotdef	= 0,		/* see textual error instead */
	Errnotfound	= 1,
	Errnoaccess	= 2,
	Errdiskfull	= 3,
	Errbadop	= 4,
	Errbadtid	= 5,
	Errexists	= 6,
	Errnouser	= 7,
	Errbadopt	= 8,		/* really bad option value */

	Defsegsize	= 512,
	Maxsegsize	= 65464,	/* from rfc2348 */

	/*
	 * bandt (viaduct) tunnels use smaller mtu than ether's
	 * (1400 bytes for tcp mss of 1300 bytes).
	 */
	Bandtmtu	= 1400,
	/*
	 * maximum size of block's data content, excludes hdrs,
	 * notably IP/UDP and TFTP, using worst-case (IPv6) sizes.
	 */
	Bandtblksz	= Bandtmtu - 40 - 8,
};

typedef struct Opt Opt;
struct Opt {
	char	*name;
	int	*valp;		/* set to client's value if within bounds */
	int	min;
	int	max;
};

int 	dbg;
int	restricted;
int	pid;

/* options */
int	blksize = Defsegsize;		/* excluding 4-byte header */
int	timeout = 5;			/* seconds */
int	tsize;
static Opt option[] = {
	"timeout",	&timeout,	1,	255,
	/* see "hack" below */
	"blksize",	&blksize,	8,	Maxsegsize,
	"tsize",	&tsize,		0,	~0UL >> 1,
};

void	sendfile(int, char*, char*, int);
void	recvfile(int, char*, char*);
void	nak(int, int, char*);
void	ack(int, ushort);
void	clrcon(void);
void	setuser(void);
char*	sunkernel(char*);
void	remoteaddr(char*, char*, int);
void	doserve(int);

char	bigbuf[32768];
char	raddr[64];

char	*dir = "/lib/tftpd";
char	*dirsl;
int	dirsllen;
char	flog[] = "ipboot";
char	net[Maxpath];

static char *opnames[] = {
[Tftp_READ]	"read",
[Tftp_WRITE]	"write",
[Tftp_DATA]	"data",
[Tftp_ACK]	"ack",
[Tftp_ERROR]	"error",
[Tftp_OACK]	"oack",
};

void
usage(void)
{
	fprint(2, "usage: %s [-dr] [-h homedir] [-s svc] [-x netmtpt]\n",
		argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	char buf[64];
	char adir[64], ldir[64];
	int cfd, lcfd, dfd;
	char *svc = "69";

	setnetmtpt(net, sizeof net, nil);
	ARGBEGIN{
	case 'd':
		dbg++;
		break;
	case 'h':
		dir = EARGF(usage());
		break;
	case 'r':
		restricted = 1;
		break;
	case 's':
		svc = EARGF(usage());
		break;
	case 'x':
		setnetmtpt(net, sizeof net, EARGF(usage()));
		break;
	default:
		usage();
	}ARGEND

	snprint(buf, sizeof buf, "%s/", dir);
	dirsl = strdup(buf);
	dirsllen = strlen(dirsl);

	fmtinstall('E', eipfmt);
	fmtinstall('I', eipfmt);

	/*
	 * setuser calls newns, and typical /lib/namespace files contain
	 * "cd /usr/$user", so call setuser before chdir.
	 */
	setuser();
	if(chdir(dir) < 0)
		sysfatal("can't get to directory %s: %r", dir);

	if(!dbg)
		switch(rfork(RFNOTEG|RFPROC|RFFDG)) {
		case -1:
			sysfatal("fork: %r");
		case 0:
			break;
		default:
			exits(0);
		}

	snprint(buf, sizeof buf, "%s/udp!*!%s", net, svc);
	cfd = announce(buf, adir);
	if (cfd < 0)
		sysfatal("announcing on %s: %r", buf);
	syslog(dbg, flog, "tftpd started on %s dir %s", buf, adir);
//	setuser();
	for(;;) {
		lcfd = listen(adir, ldir);
		if(lcfd < 0)
			sysfatal("listening on %s: %r", adir);

		switch(fork()) {
		case -1:
			sysfatal("fork: %r");
		case 0:
			dfd = accept(lcfd, ldir);
			if(dfd < 0)
 				exits(0);
			remoteaddr(ldir, raddr, sizeof(raddr));
			pid = getpid();
			syslog(0, flog, "tftp %d connection from %s dir %s",
				pid, raddr, ldir);
			doserve(dfd);
			exits("done");
			break;
		default:
			close(lcfd);
			continue;
		}
	}
}

static Opt *
handleopt(int fd, char *name, char *val)
{
	int n;
	Opt *op;

	for (op = option; op < option + nelem(option); op++)
		if(cistrcmp(name, op->name) == 0) {
			n = strtol(val, nil, 10);
			if (n < op->min || n > op->max) {
				nak(fd, Errbadopt, "option value out of range");
				syslog(dbg, flog, "tftp bad option value from "
					"client: %s %s", name, val);
				sysfatal("bad option value from client: %s %s",
					name, val);
			}
			*op->valp = n;
			syslog(dbg, flog, "tftpd %d setting %s to %d",
				pid, name, n);
			return op;
		}
	return nil;
}

static vlong
filesize(char *file)
{
	vlong size;
	Dir *dp;

	dp = dirstat(file);
	if (dp == nil)
		return 0;
	size = dp->length;
	free(dp);
	return size;
}

static int
options(int fd, char *buf, char *file, ushort oper, char *p, int dlen)
{
	int nmlen, vallen, nopts;
	vlong size;
	char *val, *bp;
	Opt *op;

	buf[0] = 0;
	buf[1] = Tftp_OACK;
	bp = buf + Opsize;
	nopts = 0;
	while (dlen > 0 && *p != '\0') {
		nmlen = strlen(p) + 1;		/* include NUL */
		if (nmlen > dlen)
			break;
		dlen -= nmlen;
		val = p + nmlen;
		if (dlen <= 0 || *val == '\0')
			break;

		vallen = strlen(val) + 1;
		if (vallen > dlen)
			break;
		dlen -= vallen;

		nopts++;
		op = handleopt(fd, p, val);
		if (op) {
			/* append OACK response to buf */
			sprint(bp, "%s", p);
			bp += nmlen;
			if (oper == Tftp_READ && cistrcmp(p, "tsize") == 0) {
				size = filesize(file);
				sprint(bp, "%lld", size);
				syslog(dbg, flog, "tftpd %d %s tsize is %,lld",
					pid, file, size);
			}
			/*
			 * hack: bandt (viaducts) uses smaller mtu than ether's
			 * (1400 bytes for tcp mss of 1300 bytes),
			 * so offer at most bandt's mtu minus headers,
			 * to avoid failure of pxe booting via viaduct.
			 */
			else if (oper == Tftp_READ &&
			    cistrcmp(p, "blksize") == 0 &&
			    blksize > Bandtblksz) {
				blksize = Bandtblksz;
				sprint(bp, "%d", blksize);
				syslog(dbg, flog,
					"tftpd %d overriding blksize to %d",
					pid, blksize);
			} else
				strcpy(bp, val);  /* use requested value */
			bp += strlen(bp) + 1;
		}
		p = val + vallen;
	}
	if (nopts == 0)
		return 0;		/* no options actually seen */
	*bp++ = '\0';
	*bp++ = '\0';			/* overkill */
	*bp++ = '\0';
	if (write(fd, buf, bp - buf) < bp - buf) {
		syslog(dbg, flog, "tftpd network write error on oack to %s: %r",
			raddr);
		sysfatal("tftpd: network write error: %r");
	}
	if(Debug)
		syslog(dbg, flog, "tftpd oack: options to %s", raddr);
	return nopts;
}

/* this doesn't stop the cavium from barging ahead */
//static void
//sendnoopts(int fd, char *name)
//{
//	char buf[64];
//
//	memset(buf, 0, sizeof buf);
//	buf[0] = 0;
//	buf[1] = Tftp_OACK;
//
//	if(write(fd, buf, sizeof buf) < sizeof buf) {
//		syslog(dbg, flog, "tftpd network write error on %s oack to %s: %r",
//			name, raddr);
//		sysfatal("tftpd: network write error: %r");
//	}
//	if(Debug)
//		syslog(dbg, flog, "tftpd oack: no options to %s", raddr);
//}

static void
optlog(char *bytes, char *p, int dlen)
{
	char *bp;

	bp = bytes;
	sprint(bp, "tftpd %d option bytes: ", dlen);
	bp += strlen(bp);
	for (; dlen > 0; dlen--, p++)
		*bp++ = *p? *p: ' ';
	*bp = '\0';
	syslog(dbg, flog, "%s", bytes);
}

void
doserve(int fd)
{
	int dlen, opts;
	char *mode, *p, *file;
	short op;

	dlen = read(fd, bigbuf, sizeof(bigbuf)-1);
	if(dlen < 0)
		sysfatal("listen read: %r");

	bigbuf[dlen] = '\0';
	op = (bigbuf[0]<<8) | bigbuf[1];
	dlen -= Opsize;
	mode = file = bigbuf + Opsize;
	while(*mode != '\0' && dlen--)
		mode++;
	mode++;
	p = mode;
	while(*p && dlen--)
		p++;
	if(dlen == 0) {
		nak(fd, 0, "bad tftpmode");
		close(fd);
		syslog(dbg, flog, "tftpd %d bad mode %s for file %s from %s",
			pid, mode, file, raddr);
		return;
	}

	if(op != Tftp_READ && op != Tftp_WRITE) {
		nak(fd, Errbadop, "Illegal TFTP operation");
		close(fd);
		syslog(dbg, flog, "tftpd %d bad request %d %s", pid, op, raddr);
		return;
	}

	if(restricted){
		if(file[0] == '#' || strncmp(file, "../", 3) == 0 ||
		  strstr(file, "/../") != nil ||
		  (file[0] == '/' && strncmp(file, dirsl, dirsllen) != 0)){
			nak(fd, Errnoaccess, "Permission denied");
			close(fd);
			syslog(dbg, flog, "tftpd %d bad request %d from %s file %s",
				pid, op, raddr, file);
			return;
		}
	}

	/*
	 * options are supposed to be negotiated, but the cavium board's
	 * u-boot really wants us to use a block size of 1432 bytes and won't
	 * take `no' for an answer.
	 */
	p++;				/* skip NUL after mode */
	dlen--;
	opts = 0;
	if(dlen > 0) {			/* might have options */
		char bytes[32*1024];

		if(Debug)
			optlog(bytes, p, dlen);
		opts = options(fd, bytes, file, op, p, dlen);
	}
	if(op == Tftp_READ)
		sendfile(fd, file, mode, opts);
	else
		recvfile(fd, file, mode);
}

void
catcher(void *junk, char *msg)
{
	USED(junk);

	if(strncmp(msg, "exit", 4) == 0)
		noted(NDFLT);
	noted(NCONT);
}

static int
awaitack(int fd, int block)
{
	int ackblock, al, rxl;
	ushort op;
	uchar ack[1024];

	for(rxl = 0; rxl < 10; rxl++) {
		memset(ack, 0, Hdrsize);
		alarm(1000);
		al = read(fd, ack, sizeof(ack));
		alarm(0);
		if(al < 0) {
			if (Debug)
				syslog(dbg, flog, "tftpd %d timed out "
					"waiting for ack from %s", pid, raddr);
			return Ackrexmit;
		}
		op = ack[0]<<8|ack[1];
		if(op == Tftp_ERROR) {
			if (Debug)
				syslog(dbg, flog, "tftpd %d got error "
					"waiting for ack from %s", pid, raddr);
			return Ackerr;
		} else if(op != Tftp_ACK) {
			syslog(dbg, flog, "tftpd %d rcvd %s op from %s", pid,
				(op < nelem(opnames)? opnames[op]: "gok"),
				raddr);
			return Ackerr;
		}
		ackblock = ack[2]<<8|ack[3];
		if (Debug)
			syslog(dbg, flog, "tftpd %d read ack of %d bytes "
				"for block %d", pid, al, ackblock);
		if(ackblock == block)
			return Ackok;		/* for block just sent */
		else if(ackblock == block + 1)	/* intel pxe eof bug */
			return Ackok;
		else if(ackblock == 0xffff)
			return Ackrexmit;
		else
			/* ack is for some other block; ignore it, try again */
			syslog(dbg, flog, "tftpd %d expected ack for block %d, "
				"got %d", pid, block, ackblock);
	}
	return Ackrexmit;
}

void
sendfile(int fd, char *name, char *mode, int opts)
{
	int file, block, ret, rexmit, n, txtry;
	uchar buf[Maxsegsize+Hdrsize];
	char errbuf[Maxerr];

	syslog(dbg, flog, "tftpd %d send file '%s' %s to %s",
		pid, name, mode, raddr);
	name = sunkernel(name);
	if(name == 0){
		nak(fd, 0, "not in our database");
		return;
	}

	notify(catcher);

	file = open(name, OREAD);
	if(file < 0) {
		errstr(errbuf, sizeof errbuf);
		nak(fd, 0, errbuf);
		return;
	}
	block = 0;
	rexmit = Ackok;
	n = 0;
	/*
	 * if we sent an oack previously, wait for the client's ack or error.
	 * if we get no ack for our oack, it could be that we returned
	 * a tsize that the client can't handle, or it could be intel
	 * pxe just read-with-tsize to get size, couldn't be bothered to
	 * ack our oack and has just gone ahead and issued another read.
	 */
	if(opts && awaitack(fd, 0) != Ackok)
		goto error;

	for(txtry = 0; txtry < timeout;) {
		if(rexmit == Ackok) {
			block++;
			buf[0] = 0;
			buf[1] = Tftp_DATA;
			buf[2] = block>>8;
			buf[3] = block;
			n = read(file, buf+Hdrsize, blksize);
			if(n < 0) {
				errstr(errbuf, sizeof errbuf);
				nak(fd, 0, errbuf);
				return;
			}
			txtry = 0;
		}
		else {
			syslog(dbg, flog, "tftpd %d rexmit %d %s:%d to %s",
				pid, Hdrsize+n, name, block, raddr);
			txtry++;
		}

		ret = write(fd, buf, Hdrsize+n);
		if(ret < Hdrsize+n) {
			syslog(dbg, flog,
				"tftpd network write error on %s to %s: %r",
				name, raddr);
			sysfatal("tftpd: network write error: %r");
		}
		if (Debug)
			syslog(dbg, flog, "tftpd %d sent block %d", pid, block);

		rexmit = awaitack(fd, block);
		if (rexmit == Ackerr)
			break;
		if(ret != blksize+Hdrsize && rexmit == Ackok)
			break;
	}
error:
	close(fd);
	close(file);
}

void
recvfile(int fd, char *name, char *mode)
{
	ushort op, block, inblock;
	uchar buf[Maxsegsize+8];
	char errbuf[Maxerr];
	int n, ret, file;

	syslog(dbg, flog, "receive file '%s' %s from %s", name, mode, raddr);

	file = create(name, OWRITE, 0666);
	if(file < 0) {
		errstr(errbuf, sizeof errbuf);
		nak(fd, 0, errbuf);
		syslog(dbg, flog, "can't create %s: %r", name);
		return;
	}

	block = 0;
	ack(fd, block);
	block++;

	for (;;) {
		alarm(15000);
		n = read(fd, buf, blksize+8);
		alarm(0);
		if(n < 0) {
			syslog(dbg, flog, "tftpd: network error reading %s: %r",
				name);
			goto error;
		}
		if(n <= Hdrsize) {
			syslog(dbg, flog,
				"tftpd: short read from network, reading %s",
				name);
			goto error;
		}
		op = buf[0]<<8|buf[1];
		if(op == Tftp_ERROR) {
			syslog(dbg, flog, "tftpd: tftp error reading %s", name);
			goto error;
		}

		n -= Hdrsize;
		inblock = buf[2]<<8|buf[3];
		if(op == Tftp_DATA) {
			if(inblock == block) {
				ret = write(file, buf+Hdrsize, n);
				if(ret != n) {
					errstr(errbuf, sizeof errbuf);
					nak(fd, 0, errbuf);
					syslog(dbg, flog,
					    "tftpd: error writing %s: %s",
						name, errbuf);
					goto error;
				}
				ack(fd, block);
				block++;
			} else
				ack(fd, 0xffff);	/* tell him to resend */
		}
	}
error:
	close(file);
}

void
ack(int fd, ushort block)
{
	uchar ack[4];
	int n;

	ack[0] = 0;
	ack[1] = Tftp_ACK;
	ack[2] = block>>8;
	ack[3] = block;

	n = write(fd, ack, 4);
	if(n < 4)
		sysfatal("network write: %r");
}

void
nak(int fd, int code, char *msg)
{
	char buf[128];
	int n;

	buf[0] = 0;
	buf[1] = Tftp_ERROR;
	buf[2] = 0;
	buf[3] = code;
	strcpy(buf+4, msg);
	n = strlen(msg) + 4 + 1;
	if(write(fd, buf, n) < n)
		sysfatal("write nak: %r");
}

void
setuser(void)
{
	int fd;

	fd = open("#c/user", OWRITE);
	if(fd < 0 || write(fd, "none", strlen("none")) < 0)
		sysfatal("can't become none: %r");
	close(fd);
	if(newns("none", nil) < 0)
		sysfatal("can't build namespace: %r");
}

char*
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
	tval[len-1] = 0;
	ndbfree(t);
	return tval;
}

/*
 *  for sun kernel boots, replace the requested file name with
 *  a one from our database.  If the database doesn't specify a file,
 *  don't answer.
 */
char*
sunkernel(char *name)
{
	ulong addr;
	uchar v4[IPv4addrlen];
	uchar v6[IPaddrlen];
	char buf[256];
	char ipbuf[128];
	char *suffix;

	addr = strtoul(name, &suffix, 16);
	if(suffix-name != 8 || (strcmp(suffix, "") != 0 && strcmp(suffix, ".SUN") != 0))
		return name;

	v4[0] = addr>>24;
	v4[1] = addr>>16;
	v4[2] = addr>>8;
	v4[3] = addr;
	v4tov6(v6, v4);
	sprint(ipbuf, "%I", v6);
	return lookup("ip", ipbuf, "bootf", buf, sizeof buf);
}

void
remoteaddr(char *dir, char *raddr, int len)
{
	char buf[64];
	int fd, n;

	snprint(buf, sizeof(buf), "%s/remote", dir);
	fd = open(buf, OREAD);
	if(fd < 0){
		snprint(raddr, sizeof(raddr), "unknown");
		return;
	}
	n = read(fd, raddr, len-1);
	close(fd);
	if(n <= 0){
		snprint(raddr, sizeof(raddr), "unknown");
		return;
	}
	if(n > 0)
		n--;
	raddr[n] = 0;
}
