#include "all.h"

#include "../ip/ip.h"

void
udprecv(Msgbuf *mb, Ifc *ifc)
{
	Udppkt *uh;
	int udplen, plen;

	uh = (Udppkt*)mb->data;

	plen = mb->count;
	if(plen < Ensize+Ipsize+Udpsize)
		goto drop;

	udplen = nhgets(uh->udplen);
	if(udplen+Ensize+Ipsize > plen)
		goto drop;

	/* construct pseudo hdr and check sum */
	uh->ttl = 0;
	hnputs(uh->cksum, udplen);
	if(nhgets(uh->udpsum)
	  && ptclcsum((uchar*)uh+(Ensize+Ipsize-Udpphsize), udplen + Udpphsize) != 0) {
		if(ifc->sumerr < 3)
			print("udp: cksum error %I\n", uh->src);
		ifc->sumerr++;
		goto drop;
	}

	switch(nhgets(uh->udpdst)) {
	case 520:
		riprecv(mb, ifc);
		break;
	case SNTP_LOCAL:
		sntprecv(mb, ifc);
		break;
	default:
		mbfree(mb);
		break;
	}
	return;

drop:
	mbfree(mb);
}
