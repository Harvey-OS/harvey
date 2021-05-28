#include "include.h"
#include "ip.h"
#include "fs.h"

#define INIPATHLEN	64

typedef struct Pxether Pxether;
static struct Pxether {
	Fs	fs;
	char	ini[INIPATHLEN];
} *pxether;

extern int debug;
extern int debugload;
extern char *persist;

uchar broadcast[Eaddrlen] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

static ushort tftpport = 5000;
static int Id = 1;
static int ctlrinuse;
static Netaddr myaddr;
static Netaddr server;

typedef struct {
	uchar	header[4];
	uchar	data[Segsize];
} Tftp;
static Tftp *tftpbp;

static void
hnputs(uchar *ptr, ushort val)
{
	ptr[0] = val>>8;
	ptr[1] = val;
}

static void
hnputl(uchar *ptr, ulong val)
{
	ptr[0] = val>>24;
	ptr[1] = val>>16;
	ptr[2] = val>>8;
	ptr[3] = val;
}

static ulong
nhgetl(uchar *ptr)
{
	return ((ptr[0]<<24) | (ptr[1]<<16) | (ptr[2]<<8) | ptr[3]);
}

static ushort
nhgets(uchar *ptr)
{
	return ((ptr[0]<<8) | ptr[1]);
}

static	short	endian	= 1;
static	char*	aendian	= (char*)&endian;
#define	LITTLE	*aendian

static ushort
ptcl_csum(void *a, int len)
{
	uchar *addr;
	ulong t1, t2;
	ulong losum, hisum, mdsum, x;

	addr = a;
	losum = 0;
	hisum = 0;
	mdsum = 0;

	x = 0;
	if((ulong)addr & 1) {
		if(len) {
			hisum += addr[0];
			len--;
			addr++;
		}
		x = 1;
	}
	while(len >= 16) {
		t1 = *(ushort*)(addr+0);
		t2 = *(ushort*)(addr+2);	mdsum += t1;
		t1 = *(ushort*)(addr+4);	mdsum += t2;
		t2 = *(ushort*)(addr+6);	mdsum += t1;
		t1 = *(ushort*)(addr+8);	mdsum += t2;
		t2 = *(ushort*)(addr+10);	mdsum += t1;
		t1 = *(ushort*)(addr+12);	mdsum += t2;
		t2 = *(ushort*)(addr+14);	mdsum += t1;
		mdsum += t2;
		len -= 16;
		addr += 16;
	}
	while(len >= 2) {
		mdsum += *(ushort*)addr;
		len -= 2;
		addr += 2;
	}
	if(x) {
		if(len)
			losum += addr[0];
		if(LITTLE)
			losum += mdsum;
		else
			hisum += mdsum;
	} else {
		if(len)
			hisum += addr[0];
		if(LITTLE)
			hisum += mdsum;
		else
			losum += mdsum;
	}

	losum += hisum >> 8;
	losum += (hisum & 0xff) << 8;
	while(hisum = losum>>16)
		losum = hisum + (losum & 0xffff);

	return ~losum;
}

static ushort
ip_csum(uchar *addr)
{
	int len;
	ulong sum = 0;

	len = (addr[0]&0xf)<<2;

	while(len > 0) {
		sum += addr[0]<<8 | addr[1] ;
		len -= 2;
		addr += 2;
	}

	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);
	return (sum^0xffff);
}

static void
udpsend(int ctlrno, Netaddr *a, void *data, int dlen)
{
	Udphdr *uh;
	Etherhdr *ip;
	Etherpkt pkt;
	int len, ptcllen;

	uh = (Udphdr*)&pkt;

	memset(uh, 0, sizeof(Etherpkt));
	memmove(uh->udpcksum+sizeof(uh->udpcksum), data, dlen);

	/*
	 * UDP portion
	 */
	ptcllen = dlen + (UDP_HDRSIZE-UDP_PHDRSIZE);
	uh->ttl = 0;
	uh->udpproto = IP_UDPPROTO;
	uh->frag[0] = 0;
	uh->frag[1] = 0;
	hnputs(uh->udpplen, ptcllen);
	hnputl(uh->udpsrc, myaddr.ip);
	hnputs(uh->udpsport, myaddr.port);
	hnputl(uh->udpdst, a->ip);
	hnputs(uh->udpdport, a->port);
	hnputs(uh->udplen, ptcllen);
	uh->udpcksum[0] = 0;
	uh->udpcksum[1] = 0;
	dlen = (dlen+1)&~1;
	hnputs(uh->udpcksum, ptcl_csum(&uh->ttl, dlen+UDP_HDRSIZE));

	/*
	 * IP portion
	 */
	ip = (Etherhdr*)&pkt;
	len = UDP_EHSIZE+UDP_HDRSIZE+dlen;		/* non-descriptive names */
	ip->vihl = IP_VER|IP_HLEN;
	ip->tos = 0;
	ip->ttl = 255;
	hnputs(ip->length, len-ETHER_HDR);
	hnputs(ip->id, Id++);
	ip->frag[0] = 0;
	ip->frag[1] = 0;
	ip->cksum[0] = 0;
	ip->cksum[1] = 0;
	hnputs(ip->cksum, ip_csum(&ip->vihl));

	/*
	 * Ethernet MAC portion
	 */
	hnputs(ip->type, ET_IP);
	memmove(ip->d, a->ea, sizeof(ip->d));

if(debug) {
	print("udpsend ");
}
	/*
	 * if packet is too short, make it longer rather than relying
	 * on ethernet interface or lower layers to pad it.
	 */
	if (len < ETHERMINTU)
		len = ETHERMINTU;
	ethertxpkt(ctlrno, &pkt, len, Timeout);
}

static void
nak(int ctlrno, Netaddr *a, int code, char *msg, int report)
{
	int n;
	char buf[128];

	buf[0] = 0;
	buf[1] = Tftp_ERROR;
	buf[2] = 0;
	buf[3] = code;
	strecpy(buf+4, buf + sizeof buf, msg);
	n = strlen(msg) + 4 + 1;
	udpsend(ctlrno, a, buf, n);
	if(report)
		print("\ntftp: error(%d): %s\n", code, msg);
}

void
dump(void *vaddr, int words)
{
	ulong *addr;

	addr = vaddr;
	while (words-- > 0)
		print("%.8lux%c", *addr++, words % 8 == 0? '\n': ' ');
}

static int
udprecv(int ctlrno, Netaddr *a, void *data, int dlen)
{
	int n, len;
	ushort csm;
	Udphdr *h;
	ulong addr, timo;
	Etherpkt pkt;
	static int rxactive;

	if(rxactive == 0)
		timo = 1000;
	else
		timo = Timeout;
	timo += TK2MS(m->ticks);
	while(timo > TK2MS(m->ticks)){
		n = etherrxpkt(ctlrno, &pkt, timo - TK2MS(m->ticks));
		if(n <= 0)
			continue;

		h = (Udphdr*)&pkt;
		if(debug)
			print("udprecv %E to %E...\n", h->s, h->d);

		if(nhgets(h->type) != ET_IP) {
			if(debug)
				print("not ip...");
			continue;
		}

		if(ip_csum(&h->vihl)) {
			print("ip chksum error\n");
			continue;
		}
		if(h->vihl != (IP_VER|IP_HLEN)) {
			print("ip bad vers/hlen\n");
			continue;
		}

		if(h->udpproto != IP_UDPPROTO) {
			if(debug)
				print("not udp (%d)...", h->udpproto);
			continue;
		}

		if(debug)
			print("okay udp...");

		h->ttl = 0;
		len = nhgets(h->udplen);
		hnputs(h->udpplen, len);

		if(nhgets(h->udpcksum)) {
			csm = ptcl_csum(&h->ttl, len+UDP_PHDRSIZE);
			if(csm != 0) {
				print("udp chksum error csum #%4ux len %d\n",
					csm, n);
				break;
			}
		}

		if(a->port != 0 && nhgets(h->udpsport) != a->port) {
			if(debug)
				print("udpport %ux not %ux\n",
					nhgets(h->udpsport), a->port);
			continue;
		}

		addr = nhgetl(h->udpsrc);
		if(a->ip != Bcastip && a->ip != addr) {
			if(debug)
				print("bad ip %lux not %lux\n", addr, a->ip);
			continue;
		}

		len -= UDP_HDRSIZE-UDP_PHDRSIZE;
		if(len > dlen) {
			print("udp: packet too big: %d > %d; from addr %E\n",
				len, dlen, h->udpsrc);
			continue;
		}

		memmove(data, h->udpcksum+sizeof(h->udpcksum), len);
		a->ip = addr;
		a->port = nhgets(h->udpsport);
		memmove(a->ea, pkt.s, sizeof(a->ea));

		rxactive = 1;
		return len;
	}

	return 0;
}

static int tftpblockno;

/*
 * format of a request packet, from the RFC:
 *
            2 bytes     string    1 byte     string   1 byte
            ------------------------------------------------
           | Opcode |  Filename  |   0  |    Mode    |   0  |
            ------------------------------------------------
 */
static int
tftpopen(int ctlrno, Netaddr *a, char *name, Tftp *tftp, int op)
{
	int i, len, rlen, oport;
	char *end;
	static char buf[Segsize+2];	/* reduce stack use */

	ctlrinuse = ctlrno;
	buf[0] = 0;
	buf[1] = op;
	end = seprint(buf+2, buf + sizeof buf, "%s", name) + 1;
	end = seprint(end, buf + sizeof buf, "octet") + 1;
	len = end - buf;

	oport = a->port;
	for(i = 0; i < 5; i++){
		a->port = oport;
		udpsend(ctlrno, a, buf, len);
		a->port = 0;
		if((rlen = udprecv(ctlrno, a, tftp, sizeof(Tftp))) < sizeof(tftp->header))
			continue;

		switch((tftp->header[0]<<8)|tftp->header[1]){
		case Tftp_ERROR:
			print("tftpopen: error (%d): %s\n",
				(tftp->header[2]<<8)|tftp->header[3],
				(char*)tftp->data);
			return -1;
		case Tftp_DATA:
			/* this should only happen when opening to read */
			tftpblockno = 1;
			len = (tftp->header[2]<<8)|tftp->header[3];
			if(len != tftpblockno){
				print("tftpopen: block error: %d\n", len);
				nak(ctlrno, a, 1, "block error", 0);
				return -1;
			}
			rlen -= sizeof(tftp->header);
			if(rlen < Segsize){
				/* ACK last block now, in case we don't later */
				buf[0] = 0;
				buf[1] = Tftp_ACK;
				buf[2] = tftpblockno>>8;
				buf[3] = tftpblockno;
				udpsend(ctlrno, a, buf, sizeof(tftp->header));
			}
			return rlen;
		case Tftp_ACK:
			/* this should only happen when opening to write */
			len = (tftp->header[2]<<8)|tftp->header[3];
			if(len != 0){
				print("tftpopen: block # error: %d != 0\n",
					len);
				nak(ctlrno, a, 1, "block # error", 0);
				return -1;
			}
			return 0;
		}
	}

	print("tftpopen: failed to connect to server\n");
	return -1;
}

static int
tftpread(int ctlrno, Netaddr *a, Tftp *tftp, int dlen)
{
	uchar buf[4];
	int try, blockno, len;

	dlen += sizeof(tftp->header);

	for(try = 0; try < 10; try++) {
		buf[0] = 0;
		buf[1] = Tftp_ACK;
		buf[2] = tftpblockno>>8;
		buf[3] = tftpblockno;

		udpsend(ctlrno, a, buf, sizeof(buf));
		len = udprecv(ctlrno, a, tftp, dlen);
		if(len <= sizeof(tftp->header)){
			if(debug)
				print("tftpread: too short %d <= %d\n",
					len, sizeof(tftp->header));
			continue;
		}
		blockno = (tftp->header[2]<<8)|tftp->header[3];
		if(blockno <= tftpblockno){
			if(debug)
				print("tftpread: blkno %d <= %d\n",
					blockno, tftpblockno);
			continue;
		}

		if(blockno == tftpblockno+1) {
			tftpblockno++;
			if(len < dlen) {	/* last packet; send final ack */
				tftpblockno++;
				buf[0] = 0;
				buf[1] = Tftp_ACK;
				buf[2] = tftpblockno>>8;
				buf[3] = tftpblockno;
				udpsend(ctlrno, a, buf, sizeof(buf));
			}
			return len-sizeof(tftp->header);
		}
		print("tftpread: block error: %d, expected %d\n",
			blockno, tftpblockno+1);
	}

	return -1;
}

static int
bootpopen(int ctlrno, char *file, Bootp *rep, int dotftpopen)
{
	Bootp req;
	int i, n;
	uchar *ea;
	char name[128], *filename, *sysname;

	if (debugload)
		print("bootpopen: ether%d!%s...", ctlrno, file);
	if((ea = etheraddr(ctlrno)) == 0){
		print("invalid ctlrno %d\n", ctlrno);
		return -1;
	}

	filename = 0;
	sysname = 0;
	if(file && *file){
		strecpy(name, name + sizeof name, file);
		if(filename = strchr(name, '!')){
			sysname = name;
			*filename++ = 0;
		}
		else
			filename = name;
	}

	memset(&req, 0, sizeof(req));
	req.op = Bootrequest;
	req.htype = 1;			/* ethernet */
	req.hlen = Eaddrlen;		/* ethernet */
	memmove(req.chaddr, ea, Eaddrlen);
	if(filename != nil)
		strncpy(req.file, filename, sizeof(req.file));
	if(sysname != nil)
		strncpy(req.sname, sysname, sizeof(req.sname));

	myaddr.ip = 0;
	myaddr.port = BPportsrc;
	memmove(myaddr.ea, ea, Eaddrlen);

	etherrxflush(ctlrno);
	for(i = 0; i < 10; i++) {
		server.ip = Bcastip;
		server.port = BPportdst;
		memmove(server.ea, broadcast, sizeof(server.ea));
		udpsend(ctlrno, &server, &req, sizeof(req));
		if(udprecv(ctlrno, &server, rep, sizeof(*rep)) <= 0)
			continue;
		if(memcmp(req.chaddr, rep->chaddr, Eaddrlen))
			continue;
		if(rep->htype != 1 || rep->hlen != Eaddrlen)
			continue;
		if(sysname == 0 || strcmp(sysname, rep->sname) == 0)
			break;
	}
	if(i >= 10) {
		print("bootp on ether%d for %s timed out\n", ctlrno, file);
		return -1;
	}

	if(!dotftpopen)
		return 0;

	if(filename == 0 || *filename == 0){
		if(strcmp(rep->file, "/386/9pxeload") == 0)
			return -1;
		filename = rep->file;
	}

	if(rep->sname[0] != '\0')
		 print("%s ", rep->sname);
	print("(%d.%d.%d.%d!%d): %s\n",
		rep->siaddr[0],
		rep->siaddr[1],
		rep->siaddr[2],
		rep->siaddr[3],
		server.port,
		filename);

	myaddr.ip = nhgetl(rep->yiaddr);
	myaddr.port = tftpport++;
	server.ip = nhgetl(rep->siaddr);
	server.port = TFTPport;

	if((n = tftpopen(ctlrno, &server, filename, tftpbp, Tftp_READ)) < 0)
		return -1;

	return n;
}

int
bootpboot(int ctlrno, char *file, Boot *b)
{
	int n;
	Bootp rep;

	if (tftpbp == nil)
		tftpbp = malloc(sizeof *tftpbp);
	if((n = bootpopen(ctlrno, file, &rep, 1)) < 0)
		return -1;

	while(bootpass(b, tftpbp->data, n) == MORE){
		n = tftpread(ctlrno, &server, tftpbp, sizeof(tftpbp->data));
		if(n < sizeof(tftpbp->data))
			break;
	}

	if(0 < n && n < sizeof(tftpbp->data))	/* got to end of file */
		bootpass(b, tftpbp->data, n);
	else
		nak(ctlrno, &server, 3, "ok", 0);	/* tftpclose to abort transfer */
	bootpass(b, nil, 0);	/* boot if possible */
	return -1;
}

static vlong
pxediskseek(Fs*, vlong)
{
	return -1;
}

static long
pxediskread(Fs*, void*, long)
{
	return -1;
}

static long
pxeread(File* f, void* va, long len)
{
	int n;
	Bootp rep;
	char *p, *v;

	if (pxether == nil)
		pxether = malloc(MaxEther * sizeof *pxether);
	if((n = bootpopen(f->fs->dev, pxether[f->fs->dev].ini, &rep, 1)) < 0)
		return -1;

	p = v = va;
	while(n > 0) {
		if((p-v)+n > len)
			n = len - (p-v);
		memmove(p, tftpbp->data, n);
		p += n;
		if(n != Segsize)
			break;
		if((n = tftpread(f->fs->dev, &server, tftpbp, sizeof(tftpbp->data))) < 0)
			return -1;
	}
	return p-v;
}

static int
pxewalk(File* f, char* name)
{
	Bootp rep;
	char *ini;

	switch(f->walked){
	default:
		return -1;
	case 0:
		if(strcmp(name, "cfg") == 0){
			f->walked = 1;
			return 1;
		}
		break;
	case 1:
		if(strcmp(name, "pxe") == 0){
			f->walked = 2;
			return 1;
		}
		break;
	case 2:
		if(strcmp(name, "%E") != 0)
			break;
		f->walked = 3;

		if(bootpopen(f->fs->dev, nil, &rep, 0) < 0)
			return 0;

		if (pxether == nil)
			pxether = malloc(MaxEther * sizeof *pxether);
		ini = pxether[f->fs->dev].ini;
		/* use our mac address instead of relying on a bootp answer */
		seprint(ini, ini+INIPATHLEN, "/cfg/pxe/%E", (uchar *)myaddr.ea);
		f->path = ini;

		return 1;
	}
	return 0;
}

void*
pxegetfspart(int ctlrno, char* part, int)
{
	Fs *fs;

	if(!pxe || strcmp(part, "*") != 0 || ctlrno >= MaxEther ||
	    iniread && getconf("*pxeini") != nil)
		return nil;

	if (pxether == nil)
		pxether = malloc(MaxEther * sizeof *pxether);
	fs = &pxether[ctlrno].fs;
	fs->dev = ctlrno;
	fs->diskread = pxediskread;
	fs->diskseek = pxediskseek;

	fs->read = pxeread;
	fs->walk = pxewalk;

	fs->root.fs = fs;
	fs->root.walked = 0;

	return fs;
}

/*
 * tftp upload, for memory dumps and the like.
 */

void
sendfile(int ctlrno, Netaddr *a, void *p, int len, char *name)
{
	int ackblock, block, ret, rexmit, rlen, n, txtry, rxl;
	short op;
	char *mem, *emem;
	static uchar ack[1024], buf[Segsize+4];	/* reduce stack use */

	block = 0;
	rexmit = 0;
	n = 0;
	mem = p;
	emem = mem + len;
	for(txtry = 0; txtry < 10;) {
		if(rexmit == 0) {
			block++;
			buf[0] = 0;
			buf[1] = Tftp_DATA;
			buf[2] = block>>8;
			buf[3] = block;
			if (mem < emem) {
				if (emem - mem < Segsize)
					n = emem - mem;
				else
					n = Segsize;
				memmove(buf+4, mem, n);
				mem += n;
			} else
				n = 0;
			txtry = 0;
		} else {
			print("rexmit %d bytes to %s:%d\n", 4+n, name, block);
			txtry++;
		}

		/* write buf to network */
		udpsend(ctlrno, a, buf, 4+n);
		ret = 4+n;

		for(rxl = 0; rxl < 10; rxl++) {
			rexmit = 0;
			/* read ack from network */
			memset(ack, 0, 32);
			rlen = udprecv(ctlrno, a, ack, sizeof ack);
			if(rlen < sizeof tftpbp->header) {
				rexmit = 1;
				print("reply too small\n");
				break;
			}
			op = ack[0]<<8 | ack[1];
			if(op == Tftp_ERROR) {
				print("sendfile: tftp error\n");
				break;
			}
			if (op != Tftp_ACK) {
				print("expected ACK!\n");
				continue;
			}
			ackblock = ack[2]<<8 | ack[3];
			if(ackblock == block)
				break;
			else if(ackblock == 0xffff) {
				rexmit = 1;
				break;
			} else
				print("ack not for last block sent, "
					"awaiting another\n");
		}
		if (rxl >= 10)
			print("never got ack for block %d\n", block);
		if(ret != Segsize+4 && rexmit == 0) {
			print(" done.\n");
			break;
		}
		if (0 && rexmit == 0)
			print(".");
	}
	if (txtry >= 5)
		print("too many rexmits\n");
}

int
tftpupload(char *name, void *p, int len)
{
	int n;

	if (tftpbp == nil)
		tftpbp = malloc(sizeof *tftpbp);

	/* assume myaddr and server are still set from downloading */
	myaddr.port = tftpport++;
	server.port = TFTPport;

	n = tftpopen(ctlrinuse, &server, name, tftpbp, Tftp_WRITE);
	if(n < 0)
		return -1;
	sendfile(ctlrinuse, &server, p, len, name);
	return 0;
}
