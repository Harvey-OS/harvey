#include "nat.h"

// NAT packet handling
// assumptions: IPv4, no options, no fragments

// copies new to old, while incrementing sum
void
chksum_update(uchar *sum, uchar *old, uchar *new, int nbytes)
{
	// ASSUMES: nbytes even; sum, old, new aligned as ushort
	// see RFC 1071 and 1141:  ~C' = ~C + m + ~m'
	ushort s;
	ulong m, C;

	C = *(ushort*)sum;
	while(nbytes>0){
		m = *(ushort*)old;
		s = *(ushort*)new;
		*(ushort*)old = s;
		C += m + (ushort)~s;
		new += 2;
		old += 2;
		nbytes -= 2;
	}
	while(C>>16)
		C = (C & 0xffff) + (C>>16);
	*(ushort*)sum = C;
}

// using rewritten f, rewrite IP and protocol headers
static void
NATrewrite(int dir, IP *ip, uchar *Psum, uchar *Pport, FiveTuple *f)
{
	uchar *addr, pseudoaddr[4];
	if(dir == OUTBOUND){
		addr = ip->src;
	}else{
		addr = ip->dst;
	}
	memcpy(pseudoaddr, addr, sizeof pseudoaddr);
	chksum_update(ip->cksum, addr, f->iaddr, 4);
	if(Psum == nil)
		return;
	chksum_update(Psum, pseudoaddr, f->iaddr, 4);
	chksum_update(Psum, Pport, f->iport, 2);
	if(f->proto == IP_TCPPROTO)
		netlog("%c %F\n", dn(dir), f);
}

// rewrite packet in place
int
NATpacket(double tim, int dir, uchar *packet, int *raddrind)
{
	FiveTuple f;
	IP *ip;
	// ICMP *icmp;
	TCP *t;
	UDP *u;
	IL *il;
	ushort frag, flag;
	int action;
	uchar *Pport = nil;	// location of port in protocol header
	uchar *Psum = nil;	// location of chksum in protocol header

	ip = (IP*)packet;
	if((ip->vihl) != 0x45)
		return NAT_ERR;
	frag = 0x3fff & nhgets(ip->frag);
	if(frag)
		return NAT_ERR;

	memset(&f, 0, sizeof f);
	if(dir == OUTBOUND){
		memcpy(f.iaddr, ip->src, 4);
		memcpy(f.oaddr, ip->dst, 4);
	}else{
		memcpy(f.iaddr, ip->dst, 4);
		memcpy(f.oaddr, ip->src, 4);
	}
	f.proto = ip->proto;

	action = NAT_DROP;
	switch(ip->proto){
	case IP_TCPPROTO:
		netlog("NATwrite:%c\t\t\tiplen %d ipid %d\n", dn(dir),
			nhgets(ip->length), nhgets(ip->id));
		t = (TCP*)(packet+sizeof(IP));
		flag = nhgets(t->tcpflag);
		if(dir == OUTBOUND){
			memcpy(f.iport, t->tcpsport, 2);
			memcpy(f.oport, t->tcpdport, 2);
			Pport = t->tcpsport;
		}else{
			memcpy(f.iport, t->tcpdport, 2);
			memcpy(f.oport, t->tcpsport, 2);
			Pport = t->tcpdport;
		}
		action = NATsession(tim, dir, &f, flag, raddrind);
		Psum = t->tcpcksum;
		break;
	case IP_UDPPROTO:
		netlog("NATwrite:udp packet\n");
		u = (UDP*)(packet+sizeof(IP));
		if(dir == OUTBOUND){
			memcpy(f.iport, u->udpsport, 2);
			memcpy(f.oport, u->udpdport, 2);
			Pport = u->udpsport;
		}else{
			memcpy(f.iport, u->udpdport, 2);
			memcpy(f.oport, u->udpsport, 2);
			Pport = u->udpdport;
		}
		action = NATsession(tim, dir, &f, 0, raddrind);
		Psum = u->udpcksum;
		if(Psum[0] == 0 && Psum[1] == 0)
			Psum = nil;
		break;
	case IP_ILPROTO:
		il = (IL*)(packet+sizeof(IP));
		if(dir == OUTBOUND){
			memcpy(f.iport, il->ilsrc, 2);
			memcpy(f.oport, il->ildst, 2);
			Pport = il->ilsrc;
		}else{
			memcpy(f.iport, il->ildst, 2);
			memcpy(f.oport, il->ilsrc, 2);
			Pport = il->ildst;
		}
		action = NATsession(tim, dir, &f, 0, raddrind);
		Psum = il->ilsum;
		break;
	case IP_ICMPPROTO:
		// icmp = (ICMP*)(packet+sizeof(IP));
		break;
	case IP_IPsecESP:
	case IP_IPsecAH:
	default:
		break;
	}
	if(action == NAT_REWRITE){
		NATrewrite(dir, ip, Psum, Pport, &f);
		action = NAT_PASS;
	}
	return action;
}
