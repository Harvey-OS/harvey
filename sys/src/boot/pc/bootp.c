#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "ip.h"

uchar broadcast[Eaddrlen] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

static ushort tftpport = 5000;
static int Id = 1;
static Netaddr myaddr;
static Netaddr server;

typedef struct {
	uchar	header[4];
	uchar	data[Segsize];
} Tftp;
static Tftp tftpb;

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

enum {
	/* this is only true of IPv4, but we're not doing v6 yet */
	Min_udp_payload = ETHERMINTU - ETHERHDRSIZE - UDP_HDRSIZE,
};

static void
udpsend(int ctlrno, Netaddr *a, void *data, int dlen)
{
	char payload[ETHERMAXTU];
	Udphdr *uh;
	Etherhdr *ip;
	Etherpkt pkt;
	int len, ptcllen;

	/*
	 * if packet is too short, make it longer rather than relying
	 * on ethernet interface or lower layers to pad it.
	 */
	if (dlen < Min_udp_payload) {
		memmove(payload, data, dlen);
		data = payload;
		dlen = Min_udp_payload;
	}

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
	strcpy(buf+4, msg);
	n = strlen(msg) + 4 + 1;
	udpsend(ctlrno, a, buf, n);
	if(report)
		print("\ntftp: error(%d): %s\n", code, msg);
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
		n = etherrxpkt(ctlrno, &pkt, timo-TK2MS(m->ticks));
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
				print("udpport %ux %ux\n",
					nhgets(h->udpsport), a->port);
			continue;
		}

		addr = nhgetl(h->udpsrc);
		if(a->ip != Bcastip && addr != a->ip) {
			if(debug)
				print("bad ip\n");
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
tftpopen(int ctlrno, Netaddr *a, char *name, Tftp *tftp)
{
	int i, len, rlen, oport;
	char buf[Segsize+2];

	buf[0] = 0;
	buf[1] = Tftp_READ;
	len = 2 + sprint(buf+2, "%s", name) + 1;
	len += sprint(buf+len, "octet") + 1;

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
				(tftp->header[2]<<8)|tftp->header[3], (char*)tftp->data);
			return -1;

		case Tftp_DATA:
			tftpblockno = 1;
			len = (tftp->header[2]<<8)|tftp->header[3];
			if(len != tftpblockno){
				print("tftpopen: block error: %d\n", len);
				nak(ctlrno, a, 1, "block error", 0);
				return -1;
			}
			rlen -= sizeof(tftp->header);
			if(rlen < Segsize){
				/* ACK now, in case we don't later */
				buf[0] = 0;
				buf[1] = Tftp_ACK;
				buf[2] = tftpblockno>>8;
				buf[3] = tftpblockno;
				udpsend(ctlrno, a, buf, sizeof(tftp->header));
			}
			return rlen;
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

	if((ea = etheraddr(ctlrno)) == 0){
		print("invalid ctlrno %d\n", ctlrno);
		return -1;
	}

	filename = 0;
	sysname = 0;
	if(file && *file){
		strcpy(name, file);
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
		print("bootp timed out\n");
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

	if((n = tftpopen(ctlrno, &server, filename, &tftpb)) < 0)
		return -1;

	return n;
}

int
bootpboot(int ctlrno, char *file, Boot *b)
{
	int n;
	Bootp rep;

	if((n = bootpopen(ctlrno, file, &rep, 1)) < 0)
		return -1;

	while(bootpass(b, tftpb.data, n) == MORE){
		n = tftpread(ctlrno, &server, &tftpb, sizeof(tftpb.data));
		if(n < sizeof(tftpb.data))
			break;
	}

	if(0 < n && n < sizeof(tftpb.data))	/* got to end of file */
		bootpass(b, tftpb.data, n);
	else
		nak(ctlrno, &server, 3, "ok", 0);	/* tftpclose to abort transfer */
	bootpass(b, nil, 0);	/* boot if possible */
	return -1;
}

#include "fs.h"

#define INIPATHLEN	64

static struct {
	Fs	fs;
	char	ini[INIPATHLEN];
} pxether[MaxEther];

static vlong
pxediskseek(Fs*, vlong)
{
	return -1LL;
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

	if((n = bootpopen(f->fs->dev, pxether[f->fs->dev].ini, &rep, 1)) < 0)
		return -1;

	p = v = va;
	while(n > 0) {
		if((p-v)+n > len)
			n = len - (p-v);
		memmove(p, tftpb.data, n);
		p += n;
		if(n != Segsize)
			break;
		if((n = tftpread(f->fs->dev, &server, &tftpb, sizeof(tftpb.data))) < 0)
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

		ini = pxether[f->fs->dev].ini;
		snprint(ini, INIPATHLEN, "/cfg/pxe/%E", rep.chaddr);
		f->path = ini;

		return 1;
	}
	return 0;
}

void*
pxegetfspart(int ctlrno, char* part, int)
{
	if(!pxe)
		return nil;
	if(strcmp(part, "*") != 0)
		return nil;
	if(ctlrno >= MaxEther)
		return nil;

	pxether[ctlrno].fs.dev = ctlrno;
	pxether[ctlrno].fs.diskread = pxediskread;
	pxether[ctlrno].fs.diskseek = pxediskseek;

	pxether[ctlrno].fs.read = pxeread;
	pxether[ctlrno].fs.walk = pxewalk;

	pxether[ctlrno].fs.root.fs = &pxether[ctlrno].fs;
	pxether[ctlrno].fs.root.walked = 0;

	return &pxether[ctlrno].fs;
}
