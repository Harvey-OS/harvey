#include "all.h"
#include "mem.h"

#include "../ip/ip.h"

enum			/* Packet Types */
{
	EchoReply	= 0,
	Unreachable	= 3,
	SrcQuench	= 4,
	EchoRequest	= 8,
	TimeExceed	= 11,
	Timestamp	= 13,
	TimestampReply	= 14,
	InfoRequest	= 15,
	InfoReply	= 16,
};

void
icmprecv(Msgbuf *mb, Ifc *ifc)
{
	Icmppkt *p;
	uchar tmp[Pasize];

	p = (Icmppkt*)mb->data;

	switch(p->icmptype) {
	default:
		goto drop;

	case EchoRequest:
		memmove(tmp, p->src, Pasize);
		memmove(p->src, p->dst, Pasize);
		memmove(p->dst, tmp, Pasize);
		p->icmptype = EchoReply;
		p->icmpsum[0] = 0;
		p->icmpsum[1] = 0;
		hnputs(p->icmpsum,
			ptclcsum((uchar*)mb->data+(Ensize+Ipsize),
				mb->count-(Ensize+Ipsize)));

		/* note that tmp contains dst */
		if((nhgetl(ifc->ipa)&ifc->mask) != (nhgetl(p->dst)&ifc->mask))
			iproute(tmp, p->dst, ifc->netgate);
		ipsend1(mb, ifc, tmp);
		break;
	}
	return;

drop:
	mbfree(mb);
}

void
igmprecv(Msgbuf *mb, Ifc*)
{
	mbfree(mb);
}

void
tcprecv(Msgbuf *mb, Ifc*)
{
	mbfree(mb);
}

