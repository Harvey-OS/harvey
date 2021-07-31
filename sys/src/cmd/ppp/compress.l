#include <alef.h>
#include "ppp.h"
#include "compress.h"

int
encode(void *p, uint n)
{
	byte	*cp;

	cp = p;
	if(n >= 256 || n == 0) {
		*cp++ = 0;
		cp[0] = n >> 8;
		cp[1] = n;
		return 3;
	} else 
		*cp = n;
	return 1;
}

#define DECODEL(f) { \
	if (*cp == 0) {\
		hnputl(f, nhgetl(f) + ((cp[1] << 8) | cp[2])); \
		cp += 3; \
	} else { \
		hnputl(f, nhgetl(f) + (uint)*cp++); \
	} \
}
#define DECODES(f) { \
	if (*cp == 0) {\
		hnputs(f, nhgets(f) + ((cp[1] << 8) | cp[2])); \
		cp += 3; \
	} else { \
		hnputs(f, nhgets(f) + (uint)*cp++); \
	} \
}

/*
 * These routines do not compress/uncompress packets whose
 * TCP/IP header sizes are not 20 each (40 bytes total).
 * If the other side compresses a >40 header length packet,
 * we decode it wrong and count on the uncompressed retransmit
 * to fix us up. This makes casts unnecessary. -egs
 */

(usint, byte*, int)
tcpcompress(PPPstate *s, byte *packet)
{
	Tcpip		*ip, *oip;	/* current and last pkt */
	uint 		hlen;		/* header length in BYTES */
	uint 		deltaS, deltaA;	/* general purpose temporaries */
	uint 		changes;	/* change mask */
	uint		ndata;
	byte 		new_seq[16];	/* changes from last to current */
	byte 		*cp;
	int 		i, j;

	rescue {
		/*
		 * Update connection state & send uncompressed packet
		 */
		memcpy(oip, ip, sizeof(Tcpip));
		ip->proto = j;
		s->tcpcomp->lastxmit = j;
		return(PRTCL_TCPUNCOMPRSSD, packet, ndata);
	};

	/*
	 * Bail if this is not a compressible TCP/IP packet
	 */
	ip = (Tcpip *) packet;
	ndata = nhgets(ip->length);
	if((nhgets(ip->flag) & (SYN|FIN|RST|ACK)) != ACK)
		return(PRTCL_IP, packet, ndata);
	/*
	 * Packet is compressible
	 */
	changes = 0;
	cp = new_seq;
	hlen = ((ip->vihl & 0xf) + (ip->flag[0] >> 4)) << 2;
	j = s->tcpcomp->lastxmit;
	oip = &s->tcpcomp->tip[j];
	if(memcmp(ip->src, oip->src, CONNIDSZ) != 0) {
		for(i = 0; i < TCPMAX_STATES; ++i) {
			j = (s->tcpcomp->basexmit + i) % TCPMAX_STATES;
			oip = &s->tcpcomp->tip[j];
			if(memcmp(ip->src, oip->src, CONNIDSZ) == 0)
				goto found;
		}
		if(i == TCPMAX_STATES) {
			j = s->tcpcomp->basexmit;
			j = (j + TCPMAX_STATES - 1) % TCPMAX_STATES;
			s->tcpcomp->basexmit = j;
			oip = &s->tcpcomp->tip[j];
			raise;
		}
	}
found:
	/*
	 * Make sure that only what we expect to change changed. 
	 */
	if(ip->vihl  != oip->vihl || ip->tos   != oip->tos ||
	   ip->ttl   != oip->ttl  || ip->proto != oip->proto ||
	   hlen != sizeof(Tcpip))
		raise;

	if(ip->flag[1] & URG) {
		cp += encode(cp, nhgets(ip->urg));
		changes |= NEW_U;
	} else if(memcmp(ip->urg, oip->urg, sizeof(ip->urg)) != 0)
		raise;
	if(deltaS = nhgets(ip->win) - nhgets(oip->win)) {
		cp += encode(cp, deltaS);
		changes |= NEW_W;
	}
	if(deltaA = nhgetl(ip->ack) - nhgetl(oip->ack)) {
		if(deltaA > 0xffff)
			raise;
		cp += encode(cp, deltaA);
		changes |= NEW_A;
	}
	if(deltaS = nhgetl(ip->seq) - nhgetl(oip->seq)) {
		if (deltaS > 0xffff)
			raise;
		cp += encode(cp, deltaS);
		changes |= NEW_S;
	}
	/*
	 * Look for the special-case encodings.
	 */
	switch(changes) {
	case 0:
		/*
		 * Nothing changed. If this packet contains data and the last
		 * one didn't, this is probably a data packet following an
		 * ack (normal on an interactive connection) and we send it
		 * compressed. Otherwise it's probably a retransmit,
		 * retransmitted ack or window probe.Send it uncompressed
		 * in case the other side missed the compressed version.
		 */
		if(nhgets(ip->length) != nhgets(oip->length) &&
		   nhgets(oip->length) == hlen)
			break;
		/* fall through */
	case SPECIAL_I:
	case SPECIAL_D:
		/*
		 * Actual changes match one of our special case encodings --
		 * send packet uncompressed.
		 */
		raise;
	case NEW_S | NEW_A:
		if (deltaS == deltaA &&
			deltaS == nhgets(oip->length) - hlen) {
			/* special case for echoed terminal traffic */
			changes = SPECIAL_I;
			cp = new_seq;
		}
		break;
	case NEW_S:
		if (deltaS == nhgets(oip->length) - hlen) {
			/* special case for data xfer */
			changes = SPECIAL_D;
			cp = new_seq;
		}
		break;
	}
	deltaS = nhgets(ip->id) - nhgets(oip->id);
	if(deltaS != 1) {
		cp += encode(cp, deltaS);
		changes |= NEW_I;
	}
	if (ip->flag[1] & PSH)
		changes |= TCP_PUSH_BIT;
	/*
	 * Grab the cksum before we overwrite it below. Then update our
	 * state with this packet's header.
	 */
	deltaA = nhgets(ip->tcpcksum);
	memcpy(oip, ip, sizeof(Tcpip));
	/*
	 * We want to use the original packet as our compressed packet. (cp -
	 * new_seq) is the number of bytes we need for compressed sequence
	 * numbers. In addition we need one byte for the change mask, one
	 * for the connection id and two for the tcp checksum. So, (cp -
	 * new_seq) + 4 bytes of header are needed. hlen is how many bytes
	 * of the original packet to toss so subtract the two to get the new
	 * packet size. The temporaries are gross -egs.
	 */
	deltaS = cp - new_seq;
	cp = (byte *) ip;
	if(s->tcpcomp->lastxmit != j) {
		s->tcpcomp->lastxmit = j;
		hlen -= deltaS + 4;
		cp += hlen;
		*cp++ = (changes | NEW_C);
		*cp++ = j;
	} else {
		hlen -= deltaS + 3;
		cp += hlen;
		*cp++ = changes;
	}
	hnputs(cp, deltaA);
	cp += 2;
	memmove(cp, new_seq, deltaS);
	return(PRTCL_TCPCOMPRSSD, packet + hlen, ndata - hlen);
}

(byte *, int)
tcpuncompress(PPPstate *s, byte *bufp, int ndata, usint type)
{
	byte	*cp, changes;
	sint	i;
	int	len;
	Tcpip	*nip, *oip;

	rescue {
		DPRINT("Bad Packet!\n");
		s->tcpcomp->err = 1;
		return (nil, 0);
	};

	if(type == PRTCL_TCPUNCOMPRSSD) {
		/*
		 * Locate the saved state for this connection. If the state
		 * index is legal, clear the 'discard' flag.
		 */
		nip = (Tcpip *) bufp;
		if(nip->proto >= TCPMAX_STATES)
			raise;
		s->tcpcomp->lastrecv = nip->proto;
		oip = &s->tcpcomp->rip[s->tcpcomp->lastrecv];
		s->tcpcomp->err = 0;
		/*
		 * Restore the IP protocol field then save a copy of this
		 * packet header. The checksum is zeroed in the copy so we
		 * don't have to zero it each time we process a compressed
		 * packet.
		 */
		nip->proto = IP_TCPPROTO;
		memcpy(oip, nip, sizeof(Tcpip));
		oip->ipcksum[0] = oip->ipcksum[1] = 0;
		return(bufp, ndata);
	}
	cp = bufp;
	changes = *cp++;
	if(changes & NEW_C) {
		/*
		 * Make sure the state index is in range, then grab the
		 * state. If we have a good state index, clear the 'discard'
		 * flag.
		 */
		if(*cp >= TCPMAX_STATES)
			raise;
		s->tcpcomp->err = 0;
		s->tcpcomp->lastrecv = *cp++;
	} else {
		/*
		 * This packet has no state index. If we've had a
		 * line error since the last time we got an explicit state
		 * index, we have to toss the packet.
		 */
		if(s->tcpcomp->err != 0)
			return(nil, 0);
	}
	/*
	 * Find the state then fill in the TCP checksum and PUSH bit.
	 */
	oip = &s->tcpcomp->rip[s->tcpcomp->lastrecv];
	memcpy(oip->tcpcksum, cp, sizeof oip->tcpcksum);
	cp += 2;
	if(changes & TCP_PUSH_BIT)
		oip->flag[1] |= PSH;
	else
		oip->flag[1] &= ~PSH;
	/*
	 * Fix up the state's ack, seq, urg and win fields based on the
	 * changemask.
	 */
	switch (changes & SPECIALS_MASK) {
	case SPECIAL_I:
		i = nhgets(oip->length) - sizeof(Tcpip);
		hnputl(oip->ack, nhgetl(oip->ack) + i);
		hnputl(oip->seq, nhgetl(oip->seq) + i);
		break;

	case SPECIAL_D:
		hnputl(oip->seq, nhgetl(oip->seq) + nhgets(oip->length)
					 - sizeof(Tcpip));
		break;

	default:
		if(changes & NEW_U) {
			oip->flag[1] |= URG;
			if(*cp == 0)
				hnputs(oip->urg, nhgets(cp));
			else
				hnputs(oip->urg, *cp++);
		} else
			oip->flag[1] &= ~URG;
		if(changes & NEW_W)
			DECODES(oip->win)
		if(changes & NEW_A)
			DECODEL(oip->ack)
		if(changes & NEW_S)
			DECODEL(oip->seq)
		break;
	}
	/* Update the IP ID */
	if (changes & NEW_I)
		DECODES(oip->id)
	else
		hnputs(oip->id, nhgets(oip->id) + 1);
	/*
	 * At this point, cp points to the first byte of data in the packet.
	 * Back up cp by the TCP/IP header length to make room for the
	 * reconstructed header.
	 * We assume the packet we were handed has enough space to prepend 128
	 * bytes of header.
	 */
	len = ndata - (cp - bufp) + sizeof(Tcpip);
	cp -= sizeof(Tcpip);
	hnputs(oip->length, len);
	memcpy(cp, oip, sizeof(Tcpip));

	/* recompute the ip header checksum */
	hnputs(((Tcpip *)cp)->ipcksum, ip_csum(cp));
	return(cp, len);
}

/*
 * IL Compression
 */

(usint, byte*, int)
ilcompress(PPPstate *s, byte *packet)
{
	Il		*ip, *oip;	/* current and last pkt */
	uint 		new_len, delta;
	uint		ndata;
	byte 		changes, new_pkt[24], *cp;
	int 		i, j;

	rescue {
		/*
		 * Update connection state & send uncompressed packet
		 */
		memcpy(oip, ip, sizeof(Il));
		ip->proto = j;
		s->ilcomp->lastxmit = j;
		return(PRTCL_ILUNCOMPRSSD, packet, ndata);
	};

	/*
	 * Bail if this is not a compressible IL packet
	 */
	ip = (Il *) packet;
	ndata = nhgets(ip->length);
	j = s->ilcomp->lastxmit;
	oip = &s->ilcomp->tip[j];
	if(memcmp(ip->src, oip->src, 8) != 0 || memcmp(ip->ilsrc, oip->ilsrc, 4) != 0) {
		for(i = 0; i < ILMAX_STATES; ++i) {
			j = (s->ilcomp->basexmit + i) % ILMAX_STATES;
			oip = &s->ilcomp->tip[j];
			if(memcmp(ip->src, oip->src, 8) == 0 &&
			   memcmp(ip->ilsrc, oip->ilsrc, 4) == 0)
				goto found;
		}
		if(i == ILMAX_STATES) {
			j = s->ilcomp->basexmit;
			j = (j + ILMAX_STATES - 1) % ILMAX_STATES;
			s->ilcomp->basexmit = j;
			oip = &s->ilcomp->tip[j];
			raise;
		}
	}
found:
	/*
	 * Make sure that only what we expect to change changed. 
	 */
	if(ip->vihl  != oip->vihl || ip->tos   != oip->tos ||
	   ip->ttl   != oip->ttl  || ip->proto != oip->proto ||
	   (ip->vihl & 0xf) != 5)
		raise;
	/*
	 * Figure out which of the changing fields changed. The receiver
	 * expects the packet in the order:
	 * 	changemask, connid, cksum, ipid, len, type, seqid, ack.
	 */
	changes = 0;
	cp = new_pkt + 1;
	if(s->ilcomp->lastxmit != j) {
		s->ilcomp->lastxmit = j;
		changes |= NEW_C;
		*cp++ = j;
	}
	memmove(cp, ip->ilsum, sizeof(ip->ilsum));
	cp += sizeof(ip->ilsum);
	delta = nhgets(ip->id) - nhgets(oip->id);
	if(delta != 1) {
		cp += encode(cp, delta);
		changes |= NEW_I;
	}
	if(ip->iltype != oip->iltype) {
		*cp++ = ip->iltype;
		changes |= NEW_T;
	}
	delta = ip->ilid[3] - oip->ilid[3];
	if(delta > 1) {
		cp += encode(cp, nhgetl(ip->ilack) - nhgetl(oip->ilack));
		changes |= NEW_S;
	} else if(delta == 0)
		raise;
	delta = ip->ilack[3] - oip->ilack[3];
	if(delta > 1) {
		cp += encode(cp, nhgetl(ip->ilack) - nhgetl(oip->ilack));
		changes |= NEW_A;
	} else if(delta == 0)
		raise;
	new_pkt[0] = changes;

	new_len = cp - new_pkt;
	ndata = ndata - sizeof(Il) + new_len;
	memmove((byte *) ip + sizeof(Il) - new_len, new_pkt, new_len);
	return(PRTCL_ILCOMPRSSD, packet, ndata);
}

(byte *, int)
iluncompress(PPPstate *s, byte *bufp, int ndata, usint type)
{
	byte	*cp, changes;
	int	len;
	Il	*nip, *oip;

	rescue {
		s->ilcomp->err = 0;
		return(nil, 0);
	};

	if(type == PRTCL_ILUNCOMPRSSD) {
		nip = (Il *) bufp;
		if(nip->proto >= ILMAX_STATES)
			raise;
		s->ilcomp->lastrecv = nip->proto;
		oip = &s->ilcomp->rip[s->ilcomp->lastrecv];
		s->ilcomp->err = 0;
		nip->proto = IP_ILPROTO;
		memcpy(oip, nip, sizeof(Tcpip));
		oip->ipcksum[0] = oip->ipcksum[1] = 0;
		return(bufp, ndata);
	}
	cp = bufp;
	changes = *cp++;
	if(changes & NEW_C) {
		if (*cp >= TCPMAX_STATES)
			raise;
		s->ilcomp->err = 0;
		s->ilcomp->lastrecv = *cp++;
	} else
		if(s->ilcomp->err != 0)
			return(nil, 0);
	/*
	 * 	changemask, connid, cksum, ipid, type, seqid, ack.
	 */
	oip = &s->ilcomp->rip[s->ilcomp->lastrecv];
	memcpy(oip->ilsum, cp, sizeof(oip->ilsum));
	cp += sizeof(oip->ilsum);

	if(changes & NEW_I)
		DECODES(oip->id)
	else
		hnputs(oip->id, nhgets(oip->id) + 1);
	if(changes & NEW_T)
		oip->iltype = *cp++;
	if(changes & NEW_S)
		DECODEL(oip->ilid)
	else
		oip->ilid[3]++;
	if(changes & NEW_A)
		DECODEL(oip->ilack)
	else
		oip->ilack[3]++;
	/*
	 * At this point, cp points to the first byte of data in the packet.
	 * Back up cp by the TCP/IP header length to make room for the
	 * reconstructed header.
	 * We assume the packet we were handed has enough space to
	 * prepend the header.
	 */
	len = ndata - (cp - bufp) + sizeof(Il);
	cp -= sizeof(Il);
	hnputs(oip->length, len);
	hnputs(oip->illen, len - IL_IPHDR);
	memcpy(cp, oip, sizeof(Tcpip));
	/* recompute the ip header checksum */
	hnputs(((Il *)cp)->ipcksum, ip_csum(cp));
	return(cp, len);
}

void
compress_init(PPPstate *s)
{
	alloc s->ilcomp;
	alloc s->tcpcomp;
	s->ilcomp->basexmit = ILMAX_STATES - 1;
	s->tcpcomp->basexmit = TCPMAX_STATES - 1;
}

(usint, byte*, int)
compress(PPPstate *s, byte *packet)
{
	Il		*ip;

	/*
	 * Bail if this is not a compressible IL packet
	 */
	ip = (Il *) packet;
	if((nhgets(ip->frag) & 0x3fff) != 0)
		return(PRTCL_IP, packet, nhgets(ip->length));
	switch(ip->proto) {
	case IP_TCPPROTO:
		if(s->ipcp.coptions & VJCompression)
			return tcpcompress(s, packet);
		goto uncomp;
	case IP_ILPROTO:
		if(s->ipcp.coptions & ILCompression)
			return ilcompress(s, packet);
		/* fall through */
	default:
	uncomp:
		return(PRTCL_IP, packet, nhgets(ip->length));
	}
}
