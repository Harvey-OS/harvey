#include "nat.h"

// auxiliary printing

int verbose = 3;
Biobuf *Bout;

void
netlog(char *fmt, ...)
{
	char buf[128], *out;
	va_list arg;
	int n;

	if(verbose == 0)
		return;
	va_start(arg, fmt);
	out = vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	n = out - buf;
	write(2, buf, n);
}

char*
protoname(uchar proto)
{
	static char buf[10];

	switch(proto){
	case IP_ICMPPROTO:
		snprint(buf, sizeof buf, "icmp");
		break;
	case IP_TCPPROTO:
		snprint(buf, sizeof buf, "tcp");
		break;
	case IP_UDPPROTO:
		snprint(buf, sizeof buf, "udp");
		break;
	case IP_ILPROTO:
		snprint(buf, sizeof buf, "il");
		break;
	case IP_IPsecESP:
		snprint(buf, sizeof buf, "esp");
		break;
	default:
		snprint(buf, sizeof buf, "proto%d", proto);
		break;
	}
	return buf;
}

char
dn(int dir)
{
	return (dir==OUTBOUND)?'O':'I';
}

char*
tcpflag(ushort flag)
{
	static char buf[128];

	buf[0] = 0;
	if(flag & TCP_RST)
		strcat(buf, " RST");
	if(flag & TCP_SYN)
		strcat(buf, " SYN");
	if(flag & TCP_FIN)
		strcat(buf, " FIN");
	if(flag & (TCP_RST|TCP_SYN|TCP_FIN) && flag & TCP_ACK)
		strcat(buf, " ACK");
	return buf;
}

void
Bbytes(int n, uchar *b)
{
	int i;

	for(i=0; i<n;i++){
		if(i%20 == 0)
			netlog("\n");
		netlog("%2x ", b[i]);
	}
	netlog("\n");
}

int
FiveTuple_conv(Fmt *fp)
{
	char str[100];
	FiveTuple *p;

	p = va_arg(fp->args, FiveTuple*);
	snprint(str,sizeof str,"%V %V %s/%ud/%ud", p->iaddr, p->oaddr,
		protoname(p->proto), nhgets(p->iport), nhgets(p->oport));
	
	return fmtstrcpy(fp, str);
}

void
pr_udp(int dir, uchar *p)
{
	IP *ip;
	UDP *u;
	ushort frag;

	ip = (IP*)p;
	if(ip->proto != IP_UDPPROTO)
		return;
	if((ip->vihl) != 0x45)
		netlog("vihl %ux ", ip->vihl);
	frag = 0x3fff & nhgets(ip->frag);
	if(frag)
		netlog("frag %d ", frag);
	u = (UDP*)(p+sizeof(IP));
	netlog("%c[%ud] %V -> %V udp/%d/%d\n", dn(dir), nhgets(ip->length),
		ip->src, ip->dst, nhgets(u->udpsport), nhgets(u->udpdport));
}
