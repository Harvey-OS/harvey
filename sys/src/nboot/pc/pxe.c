#include <u.h>
#include "fns.h"

enum {
	Tftp_READ	= 1,
	Tftp_WRITE	= 2,
	Tftp_DATA	= 3,
	Tftp_ACK	= 4,
	Tftp_ERROR	= 5,
	Tftp_OACK	= 6,

	TftpPort	= 69,

	Segsize		= 512,
	Maxpath		= 64,
};

typedef uchar IP4[4];

typedef struct Tftp Tftp;
typedef struct Dhcp Dhcp;

struct Tftp
{
	IP4 sip;
	IP4 dip;
	IP4 gip;

	int sport;
	int dport;

	char *rp;
	char *ep;

	int seq;
	int eof;
	
	char pkt[2+2+Segsize];
	char nul;
};

struct Dhcp
{
	uchar opcode;
	uchar hardware;
	uchar hardlen;
	uchar gatehops;
	uchar ident[4];
	uchar seconds[2];
	uchar flags[2];
	uchar cip[4];
	uchar yip[4];
	uchar sip[4];
	uchar gip[4];
	uchar mac[16];
	char sname[64];
	char bootfile[128];
};

int pxeinit(void);
int pxecall(int op, void *buf);

static void*
unfar(ulong seg, ulong off)
{
	return (void*)((off & 0xFFFF) + (seg & 0xFFFF)*16);
}

static void
puts(void *x, ushort v)
{
	uchar *p = x;

	p[1] = (v>>8) & 0xFF;
	p[0] = v & 0xFF;
}

static ushort
gets(void *x)
{
	uchar *p = x;

	return p[1]<<8 | p[0];
}

static void
hnputs(void *x, ushort v)
{
	uchar *p = x;

	p[0] = (v>>8) & 0xFF;
	p[1] = v & 0xFF;
}

static ushort
nhgets(void *x)
{
	uchar *p = x;

	return p[0]<<8 | p[1];
}

static void
moveip(IP4 d, IP4 s)
{
	memmove(d, s, sizeof(d));
}

void
unload(void)
{
	struct {
		uchar status[2];
		uchar junk[10];
	} buf;
	static uchar shutdown[] = { 0x05, 0x070, 0x02, 0 };
	uchar *o;

	for(o = shutdown; *o; o++){ 
		memset(&buf, 0, sizeof(buf));
		if(pxecall(*o, &buf))
			break;
	}
}

static int
getip(IP4 yip, IP4 sip, IP4 gip, char mac[16])
{
	struct {
		uchar status[2];
		uchar pkttype[2];
		uchar bufsize[2];
		uchar off[2];
		uchar seg[2];
		uchar lim[2];
	} buf;
	int i, r;
	Dhcp *p;

	memset(&buf, 0, sizeof(buf));
	puts(buf.pkttype, 3);

	if(r = pxecall(0x71, &buf))
		return -r;
	if((p = unfar(gets(buf.seg), gets(buf.off))) == 0)
		return -1;
	moveip(yip, p->yip);
	moveip(sip, p->sip);
	moveip(gip, p->gip);
	mac[12] = 0;
	for(i=0; i<6; i++){
		mac[i*2] = hex[p->mac[i]>>4];
		mac[i*2+1] = hex[p->mac[i]&15];
	}
	return 0;
}

static int
udpopen(IP4 sip)
{
	struct {
		uchar status[2];
		uchar sip[4];
	} buf;

	puts(buf.status, 0);
	moveip(buf.sip, sip);
	return pxecall(0x30, &buf);
}

static int
udpclose(void)
{
	uchar status[2];
	puts(status, 0);
	return pxecall(0x31, status);
}

static int
udpread(IP4 sip, IP4 dip, int *sport, int dport, int *len, void *data)
{
	struct {
		uchar status[2];
		uchar sip[4];
		uchar dip[4];
		uchar sport[2];
		uchar dport[2];
		uchar len[2];
		uchar off[2];
		uchar seg[2];
	} buf;
	int r;

	puts(buf.status, 0);
	moveip(buf.sip, sip);
	moveip(buf.dip, dip);
	hnputs(buf.sport, *sport);
	hnputs(buf.dport, dport);
	puts(buf.len, *len);
	puts(buf.off, (long)data);
	puts(buf.seg, 0);
	if(r = pxecall(0x32, &buf))
		return r;
	moveip(sip, buf.sip);
	*sport = nhgets(buf.sport);
	*len = gets(buf.len);
	return 0;
}

static int
udpwrite(IP4 ip, IP4 gw, int sport, int dport, int len, void *data)
{
	struct {
		uchar status[2];
		uchar ip[4];
		uchar gw[4];
		uchar sport[2];
		uchar dport[2];
		uchar len[2];
		uchar off[2];
		uchar seg[2];
	} buf;

	puts(buf.status, 0);
	moveip(buf.ip, ip);
	moveip(buf.gw, gw);
	hnputs(buf.sport, sport);
	hnputs(buf.dport, dport);
	puts(buf.len, len);
	puts(buf.off, (long)data);
	puts(buf.seg, 0);
	return pxecall(0x33, &buf);
}

int
read(void *f, void *data, int len)
{
	Tftp *t = f;
	int seq, n;

	while(!t->eof && t->rp >= t->ep){
		for(;;){
			n = sizeof(t->pkt);
			if(udpread(t->dip, t->sip, &t->dport, t->sport, &n, t->pkt))
				continue;
			if(n >= 4)
				break;
		}
		switch(nhgets(t->pkt)){
		case Tftp_DATA:
			seq = nhgets(t->pkt+2);
			if(seq > t->seq){
				putc('?');
				continue;
			}
			hnputs(t->pkt, Tftp_ACK);
			while(udpwrite(t->dip, t->gip, t->sport, t->dport, 4, t->pkt))
				putc('!');
			if(seq < t->seq){
				putc('@');
				continue;
			}
			t->seq = seq+1;
			n -= 4;
			t->rp = t->pkt + 4;
			t->ep = t->rp + n;
			t->eof = n < Segsize;
			break;
		case Tftp_ERROR:
			print(t->pkt+4);
			print("\n");
		default:
			t->eof = 1;
			return -1;
		}
		break;
	}
	n = t->ep - t->rp;
	if(len > n)
		len = n;
	memmove(data, t->rp, len);
	t->rp += len;
	return len;
}

void
close(void *f)
{
	Tftp *t = f;
	t->eof = 1;
	udpclose();
}


static int
tftpopen(Tftp *t, char *path, IP4 sip, IP4 dip, IP4 gip)
{
	static ushort xport = 6666;
	int r, n;
	char *p;

	moveip(t->sip, sip);
	moveip(t->gip, gip);
	memset(t->dip, 0, sizeof(t->dip));
	t->sport = xport++;
	t->dport = 0;
	t->rp = t->ep = 0;
	t->seq = 1;
	t->eof = 0;
	t->nul = 0;
	if(r = udpopen(t->sip))
		return r;
	p = t->pkt;
	hnputs(p, Tftp_READ); p += 2;
	n = strlen(path)+1;
	memmove(p, path, n); p += n;
	memmove(p, "octet", 6); p += 6;
	n = p - t->pkt;
	for(;;){
		if(r = udpwrite(dip, t->gip, t->sport, TftpPort, n, t->pkt))
			break;
		if(r = read(t, 0, 0))
			break;
		return 0;
	}
	close(t);
	return r;
}

void
start(void *)
{
	char mac[16], path[Maxpath], *kern;
	IP4 yip, sip, gip;
	void *f;
	Tftp t;

	if(pxeinit()){
		print("pxe init\n");
		halt();
	}
	if(getip(yip, sip, gip, mac)){
		print("bad dhcp\n");
		halt();
	}
	memmove(path, "/cfg/pxe/", 9);
	memmove(path+9, mac, 13);
	if(tftpopen(f = &t, path, yip, sip, gip))
		if(tftpopen(f, "/cfg/pxe/default", yip, sip, gip)){
			print("no config\n");
			f = 0;
		}
	for(;;){
		kern = configure(f, path); f = 0;
		if(tftpopen(&t, kern, yip, sip, gip)){
			print("not found\n");
			continue;
		}
		print(bootkern(&t));
		print("\n");
	}
}
