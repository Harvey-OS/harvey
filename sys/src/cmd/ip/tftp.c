/*
 * tftp [-rw] system file - access remote file via tftp,
 *	see /lib/rfc/rfc783 (now rfc1350 + 234[789])
 */
#include <u.h>
#include <libc.h>
#include <ip.h>
#include "tftp.h"

enum {
	Debug=		0,
};

typedef struct Opt Opt;
struct Opt {
	char	*name;
	int	*valp;		/* set to client's value if within bounds */
	int	min;
	int	max;
};

int 	dbg = Debug;

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

char	*sendfile(int, char *, int, char*, char*, int);
char	*recvfile(int, char *, int, char*);
void	nak(int, int, char*);
void	ack(int, ushort);
char	*doclient(int, char *);

char	raddr[NETPATHLEN];
char	net[Maxpath];
int	tftpop;

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
	fprint(2, "usage: %s [-rw] [-x netmtpt] system file\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int cfd, dfd;
	char *svc = "69", *addr, *system, *remfile;

	tftpop = Tftp_READ;
	setnetmtpt(net, sizeof net, "/net/udp");
	ARGBEGIN{
	case 'd':
		dbg++;
		break;
	case 'r':
		tftpop = Tftp_READ;
		break;
	case 'w':
		tftpop = Tftp_WRITE;
		break;
	case 'x':
		setnetmtpt(net, sizeof net, EARGF(usage()));
		break;
	default:
		usage();
	}ARGEND
	if (argc != 2)
		usage();
	system = argv[0];
	remfile = argv[1];

	fmtinstall('E', eipfmt);
	fmtinstall('I', eipfmt);

	addr = netmkaddr(system, net, svc);
	strncpy(raddr, addr, sizeof raddr);
	dfd = dial(addr, nil, nil, &cfd);
	if (dfd < 0)
		sysfatal("dialing %s: %r", addr);
	if (dbg)
		fprint(2, "dialed %s...", addr);
	exits(doclient(dfd, remfile));
}

void
catcher(void *junk, char *msg)
{
	USED(junk);

	if(strncmp(msg, "exit", 4) == 0)
		noted(NDFLT);
	noted(NCONT);
}

char *
doclient(int net, char *file)
{
	int dlen, wrlen, opts = 0;
	char *p;
	char reqpkt[512];

	/* form read or write request packet */
	if (dbg)
		fprint(2, "requesting %s...", file);
	p = reqpkt;
	*p++ = 0;
	*p++ = tftpop;
	strcpy(p, file);
	p += strlen(p) + 1;		/* skip the NUL too */
	strcpy(p, "octet");
	p += strlen(p) + 1;		/* skip the NUL too */

	/* send request */
	wrlen = p - reqpkt;
	dlen = write(net, reqpkt, wrlen);
	if(dlen != wrlen)
		sysfatal("write: %r");

	/* if the request had contained option(s), we could get an OACK here. */
	notify(catcher);
	if(tftpop == Tftp_READ)
		return recvfile(net, reqpkt, wrlen, file);
	else
		return sendfile(net, reqpkt, wrlen, file, "octet", opts);
}

static int
awaitack(int net, int block)
{
	int ackblock, al, rxl;
	ushort op;
	uchar ack[1024];

	for(rxl = 0; rxl < 10; rxl++) {
		memset(ack, 0, Hdrsize);
		alarm(1000);
		al = read(net, ack, sizeof(ack));
		alarm(0);
		if(al < 0) {
			if (Debug)
				fprint(2, "%s: timed out waiting for ack from %s\n",
					argv0, raddr);
			return Ackrexmit;
		}
		op = ack[0]<<8|ack[1];
		if(op == Tftp_ERROR) {
			if (Debug)
				fprint(2, "%s: got error waiting for ack from %s\n",
					argv0, raddr);
			return Ackerr;
		} else if(op != Tftp_ACK) {
			fprint(2, "%s: rcvd %s op from %s\n", argv0,
				(op < nelem(opnames)? opnames[op]: "gok"),
				raddr);
			return Ackerr;
		}
		ackblock = ack[2]<<8|ack[3];
		if (Debug)
			fprint(2, "%s: read ack of %d bytes "
				"for block %d", argv0, al, ackblock);
		if(ackblock == block)
			return Ackok;		/* for block just sent */
		else if(ackblock == block + 1)	/* intel pxe eof bug */
			return Ackok;
		else if(ackblock == 0xffff)
			return Ackrexmit;
		else
			/* ack is for some other block; ignore it, try again */
			fprint(2, "%s: expected ack for block %d, got %d\n",
				argv0, block, ackblock);
	}
	return Ackrexmit;
}

/* copy stdin to remote file */
char *
sendfile(int net, char *reqpkt, int wrlen, char *name, char *mode, int opts)
{
	int file, block, ret, rexmit, n, txtry, failed;
	uchar buf[Maxsegsize+Hdrsize];
	char errbuf[Maxerr];

	failed = 1;
	file = 0;			/* stdin */
	block = 0;
	rexmit = Ackok;
	n = 0;
	/*
	 * if we had sent an oack previously, we would wait for the
	 * client's ack or error.
	 * if we get no ack for our oack, it could be that we returned
	 * a tsize that the client can't handle, or it could be intel
	 * pxe just read-with-tsize to get size, couldn't be bothered to
	 * ack our oack and has just gone ahead and issued another read.
	 */
	USED(opts);
	if(awaitack(net, 0) != Ackok)
		goto error;

	for(txtry = 0; txtry < timeout;) {
		if(rexmit == Ackok) {
			block++;
			buf[0] = 0;
			buf[1] = Tftp_DATA;
			buf[2] = block>>8;
			buf[3] = block;
			n = read(file, buf+Hdrsize, blksize);
			if (dbg)
				fprint(2, "read %d payload bytes\n", n);
			/* on first timeout, resend reqpkt. untested. */
			if(0 && block == 1 && n <= 0) {
				fprint(2, "resending write request\n");
				write(net, reqpkt, wrlen);
				continue;
			}
			if(n < 0) {	/* timed out. could resend? */
				if (dbg)
					fprint(2, "read error: %r\n");
				errstr(errbuf, sizeof errbuf);
				nak(net, 0, errbuf);
				goto error;
			}
			txtry = 0;
		}
		else {
			fprint(2, "%s: rexmit %d %s:%d to %s\n",
				argv0, Hdrsize+n, name, block, raddr);
			txtry++;
		}
		if (dbg)
			fprint(2, "writing %d payload bytes to net\n", n);
		ret = write(net, buf, Hdrsize+n);
		if(ret < Hdrsize+n)
			sysfatal("network write error: %r");
		if (Debug)
			fprint(2, "%s: sent block %d\n", argv0, block);

		rexmit = awaitack(net, block);
		if (rexmit == Ackerr)
			break;
		if(ret != blksize+Hdrsize && rexmit == Ackok) {
			failed = 0;
			break;
		}
	}
error:
	if (dbg)
		fprint(2, "%s: %s file '%s' %s to %s\n", argv0,
			(failed? "failed to send": "sent"), name, mode, raddr);
	close(net);
	return failed? "failed to send": "";
}

/* copy remote file to stdout */
char *
recvfile(int net, char *reqpkt, int wrlen, char *name)
{
	ushort op, block, inblock;
	uchar buf[Maxsegsize+8];
	char errbuf[Maxerr];
	int n, ret;

	block = 1;
	for (;;) {
		memset(buf, 0, 8);
		alarm(5000);
		n = read(net, buf, blksize+8);
		alarm(0);
		if (dbg)
			fprint(2, "read %d bytes from net...", n);
		/* on first timeout, resend reqpkt. */
		if(block == 1 && n <= 0) {
			fprint(2, "resending read request\n");
			write(net, reqpkt, wrlen);
			continue;
		}
		if(n < 0)
			n = 0;	/* timed-out, read nothing */

		/*
		 * NB: not `<='; just a header is legal and happens when
		 * file being read is a multiple of segment-size bytes long.
		 */
		if(n > 0 && n < Hdrsize) {
			fprint(2, "%s: short read from network, reading %s\n",
				argv0, name);
			return "short read";
		}
		op = buf[0]<<8|buf[1];
		if(op == Tftp_ERROR) {
			fprint(2, "%s: error reading %s\n", argv0, name);
			return "remote read error";
		}

		n -= Hdrsize;
		inblock = buf[2]<<8|buf[3];
		if(op == Tftp_DATA) {
			if(inblock == block) {
				if (dbg)
					fprint(2, "blk %d...", block);
				if (n <= 0) {
					fprint(2, "write less than hdr\n");
					ret = n = blksize;
				} else
					ret = write(1, buf+Hdrsize, n);
				if(ret != n) {
					errstr(errbuf, sizeof errbuf);
					nak(net, 0, errbuf);
					fprint(2, "%s: error writing %s: %s\n",
						argv0, name, errbuf);
					return "write error";
				}
				ack(net, block);
				block++;
				if (n < blksize)
					break;
			} else
				ack(net, 0xffff);	/* tell him to resend */
		}
	}
	return "";
}

void
ack(int fd, ushort block)
{
	uchar ack[4];

	ack[0] = 0;
	ack[1] = Tftp_ACK;
	ack[2] = block>>8;
	ack[3] = block;
	if(write(fd, ack, 4) < 4)
		sysfatal("network write ack: %r");
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
		sysfatal("network write nak: %r");
}
