/*
 * simple tftp client implementation.
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

#define TFTPDEF "135.104.9.6"	/* IP of default tftp server */

enum {
	Debug =		0,

	/*
	 * this can be bigger than the ether mtu and
	 * will work due to ip fragmentation, at least on v4.
	 */
	Prefsegsize =	1400,
	Bufsz =		Maxsegsize + 2,
};

/* externs are imported from ipload.c */
uchar myea[Eaddrlen];
Pxenetaddr myaddr;		/* actually, local ip addr & port */
Pxenetaddr tftpserv;		/* actually, remote ip addr & port */
/*
 * there can be at most one concurrent tftp session until we move these
 * variables into Openeth or some other struct (Tftpstate).
 */
int progress;
int segsize;
static ushort tftpport;
static Tftp *tftpb;
static int tftpblockno;

static char ethernm[] = "ether";

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
			if(strstr((char *)tftp->data, "does not exist") != nil){
				print("%s\n", (char*)tftp->data);
				return Nonexist;
			}
			print("tftpread1st: error (%d): %s\n",
				(tftp->header[2]<<8)|tftp->header[3], (char*)tftp->data);
			return Err;

		case Tftp_OACK:
			n = optval("blksize", (char *)tftp->header+2, rlen-2);
			if (n <= 0) {
				nak(oe, a, 0, "bad blksize option value", 0);
				return Err;
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
				return Err;
			}
			rlen -= Tftphdrsz;
			if(rlen < segsize)
				/* ACK now, in case we don't later */
				ack(oe, a, tftpblockno);
			return rlen;

		default:
			print("tftpread1st: unexpected pkt type recv'd\n");
			nak(oe, a, 0, "unexpected pkt type recv'd", 0);
			return Err;
		}
	}

	print("tftpread1st: failed to connect to server (%I!%d)\n", a->ip, oport);
	return Err;
}

int
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

void
tftprandinit(void)
{
	srand(TK2MS(sys->ticks));			/* for local port numbers */
	nrand(20480);				/* 1st # is always 0; toss it */
}

static int
tftpconnect(Openeth *oe, Bootp *rep)
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

/*
 * request file via tftp from server named in rep.
 * initial data packet will be stashed in tftpb.
 */
int
tftpopen(Openeth *oe, char *file, Bootp *rep)
{
	char *filename;
	char buf[128];
	static uchar ipv4noaddr[IPv4addrlen];

	if (tftpconnect(oe, rep) < 0)
		return Err;

	/*
	 * read file from tftp server in bootp answer
	 */
	filename = filetoload(oe, file, rep);
	if (filename == nil)
		return Err;

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

long
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
			return n;
	}
	return p-v;
}

/* load the kernel in file via tftp on oe */
int
tftpboot(Openeth *oe, char *file, Bootp *rep, Boot *b)
{
	int n;

	/* file must exist, else it's an error */
	if((n = tftpopen(oe, file, rep)) < 0)
		return n;

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
	return Err;
}
