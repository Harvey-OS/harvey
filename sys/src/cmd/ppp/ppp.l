#include <alef.h>
#include "ppp.h"
#include "compress.h"
#include "queue.h"

#define	USED(x)		if(0)x

/*
 * curq queue for outstanding requests, the other for serviced
 * and rescheduled requests. Alternate between the two.
 */
intern Queue	queues[2];
intern int	curq = 0;
intern Lock	ql;
int	ipfd;
int	debug=0;

byte *statestr[] = {
	[Pclosed]	"closed",
	[Pclosing]	"closing",
	[Preqsent]	"reqsent",
	[Packrcvd]	"ackrcvd",
	[Packsent]	"acksent",
	[Popened]	"opened",
};

int
min(int a, int b)
{
	if(a < b)	return a;
	else 		return b;
}

/*
 * send an ncp packet
 */
void
Ncp.ncpsenddata(Ncp *ncp, PPPstate *s, byte code, void *data, usint n, byte ident)
{
	Ctlpkt c;

	DPRINT("sending prtcl %ux request code %d\n", ncp->prtcl, code);
	c.code = code;
	if(ident == 0)
		ident = ++ncp->ident;
	c.ident = ident;
	switch(code) {
	case ConfReq:	
	case TermReq:
	case EchoReq:
	case DiscardReq:
		ncp->lastident = ident;
	}
	hnputs(c.length, n + LCP_HDR);
	memmove(c.rawdata, data, n);
	s->pppencode(ncp->prtcl, (void*) &c, n + LCP_HDR, 1);
}

int
Ncp.lcpmakecfr(Ncp *ncp, byte *p)
{
	Cfrinfo cpool[MAXLCPOPTIONS];
	Cfrinfo *c, *cp;
	int	n;

	c = cpool;
	if(ncp->options & (1 << MaxRecUnit)) {
		c->cfrtype = MaxRecUnit;
		c->cfrlength = 4;
		hnputs(c->data, ncp->maxsize);
		++c;
	}
	if(ncp->options & (1 << AsyncCtlCharMap)) {
		c->cfrtype = AsyncCtlCharMap;
		c->cfrlength = 6;
		hnputl(c->data, ncp->asyncctlmap);
		++c;
	}
	if(ncp->options & (1 << MagicNo)) {
		c->cfrtype = MagicNo;
		c->cfrlength = 6;
		hnputl(c->data, ncp->magicno);
		++c;
	}
	if(ncp->options & (1 << PrtclCompression)) {
		c->cfrtype = PrtclCompression;
		c->cfrlength = 2;
		++c;
	}
	if(ncp->options & (1 << AddrCtlCompression)) {
		c->cfrtype = AddrCtlCompression;
		c->cfrlength = 2;
		++c;
	}
	n=0;
	for(cp = cpool; cp < c; ++cp) {
		memmove(p + n, cp, cp->cfrlength);
		n += cp->cfrlength;
	}
	return n;
}

int
Ncp.ipcpmakecfr(Ncp *ncp, byte *p)
{
	Cfrinfo cpool[MAXIPCPOPTIONS];
	Cfrinfo *c, *cp;
	Vjcompressreq *v;
	int	n;

	c = cpool;
	c->cfrtype = IPAddress;
	c->cfrlength = 6;
	memcpy(c->data, ncp->myipaddr, sizeof(ncp->myipaddr));
	++c;
	if(ncp->options & (1 << IPCompressionPrtcl)) {
		c->cfrtype = IPCompressionPrtcl;
		c->cfrlength = 6;
		v = (Vjcompressreq *) c->data;
		hnputs(v->prtcl, VJCompression);
		v->maxslots = TCPMAX_STATES - 1;
		v->compslots = 1;
		++c;
	}
	if(ncp->options & (1 << IPCompressionPrtcl)) {
		c->cfrtype = IPCompressionPrtcl;
		c->cfrlength = 6;
		v = (Vjcompressreq *) c->data;
		hnputs(v->prtcl, ILCompression);
		v->maxslots = TCPMAX_STATES - 1;
		v->compslots = 1;
		++c;
	}
	n = 0;
	for(cp = cpool; cp < c; ++cp) {
		memcpy(p + n, cp, cp->cfrlength);
		n += cp->cfrlength;
	}
	return n;
}

void
Ncp.sendcfr(Ncp *ncp, PPPstate *s)
{
	byte	pkt[PPP_DATASZ];
	int	n;

	if(ncp->prtcl == PRTCL_IPCP)
		n = ncp->ipcpmakecfr(pkt);
	else
		n = ncp->lcpmakecfr(pkt);
	ncp->ncpsenddata(s, ConfReq, pkt, n, 0);
}

/*
 * parses an LCP config request and returns 0 for success, 1 for reject
 * we assume that ppp implementations are symmetric, i.e. they can
 * receive the types of packets that they send out.
 */
Response
Ncp.lcpparsecfr(Ncp *ncp, Cfrinfo *cfr)
{
	switch(cfr->cfrtype) {
	case MaxRecUnit:
		ncp->peermaxsize = nhgets(cfr->data);
		break;
	case AsyncCtlCharMap:
		/* BUG BUG
		 * we let the other side do whatever they like
		 * and we escape everything below <0x20 else cannot
		 * talk over dk
		 */
		break;
	case MagicNo:
		ncp->peermagicno = nhgetl(cfr->data);
		break;
	case PrtclCompression:
	case AddrCtlCompression:
		break;
	default:
		return Rej;
	}
	return OK;
}

/*
 * parses an IPCP config request
 */
Response
Ncp.ipcpparsecfr(Ncp *ncp, Cfrinfo *cfr)
{
	Vjcompressreq	*v;

	switch(cfr->cfrtype) {
	case IPAddress:
		if(nhgetl(cfr->data) == 0) {
			DPRINT("Peer wants to learn his ip address\n");
			memcpy(cfr->data, ncp->peeripaddr, sizeof(ncp->peeripaddr));
			return Nak;
		} else
			memcpy(ncp->peeripaddr, cfr->data, sizeof(ncp->peeripaddr));
		break;
	case IPCompressionPrtcl:
		v = (void *) cfr->data;
		DPRINT("Peer wants to do compress %x %d %d\n",
				nhgets(v->prtcl), v->maxslots, v->compslots);
		if(nhgets(v->prtcl) != VJCompression && nhgets(v->prtcl) != ILCompression)
			return Rej;
		if(v->maxslots != TCPMAX_STATES-1 || v->compslots != 1) {
			v->maxslots = TCPMAX_STATES-1;
			v->compslots = 1;
			return Nak;	
		}
		ncp->coptions |= nhgets(v->prtcl);
		break;
	default:
		return Rej;
	}
	return OK;
}

/*
 * returns 0 if config request is fine
 * != 0 if the request cannot be fulfilled.
 */
int
Ncp.parsecfr(Ncp *ncp, PPPstate *s, uint ident, byte *data, uint length)
{
	Cfrinfo		*cfr, reply;
	byte		*odata;
	sint		olength, response, cfrrefused;

	odata = data;
	olength = length;
	cfrrefused = 0;
	while(length > 0) {
		cfr = (void*) data;
		if(ncp->prtcl == PRTCL_LCP)
			response = ncp->lcpparsecfr(cfr);
		else
			response = ncp->ipcpparsecfr(cfr);
		if(response == Rej) {
			memcpy(reply.data + cfrrefused, cfr, cfr->cfrlength);
			cfrrefused += cfr->cfrlength;
		} else if (response == Nak) {
			ncp->ncpsenddata(s, ConfNak, cfr, cfr->cfrlength, ident);
			return 1;
		} else
			ncp->options |= (1 << cfr->cfrtype);
		length -= cfr->cfrlength;
		data += cfr->cfrlength;
	}
	if(cfrrefused > 0)
		ncp->ncpsenddata(s, ConfRej, reply.data, cfrrefused, ident);
	else
		ncp->ncpsenddata(s, ConfAck, odata, olength, ident);
	return cfrrefused;
}

/*
 * check config-naks and returns 0 if the nak'ed option can be fixed
 * != 0 otherwise
 */
int
Ncp.parsecfn(Ncp *ncp, byte *data, uint length)
{
	Cfrinfo		*cfr;
	uint		cantfix;

	cantfix = 0;
	while(length > 0) {
		cfr = (void*) data;
		if(ncp->prtcl == PRTCL_IPCP)
			if(cfr->cfrtype == IPAddress) {
				memcpy(ncp->myipaddr, cfr->data, sizeof(ncp->myipaddr));
				DPRINT("Peer says my IP address is %d.%d.%d.%d\n",
					ncp->myipaddr[0],ncp->myipaddr[1],
					ncp->myipaddr[2],ncp->myipaddr[3]);
			} else
				ncp->coptions &= ~nhgets(cfr->data);
		else
			if(ncp->options & (1 << cfr->cfrtype))
				ncp->options &= ~(1 << cfr->cfrtype);
			else
				cantfix++;
		length -= cfr->cfrlength;
		data += cfr->cfrlength;
	}
	return cantfix;
}

/*
 * parses an NCP packet, takes appropriate action for LCP and IPCP
 */
void
Ncp.ncpparse(Ncp *ncp, PPPstate *s, Ctlpkt *cpkt, uint size)
{
	Prtclrej	*rej;
	Ctlpkt		*recpkt;
	Echoinfo	*einfo;
	uint		n;

	DPRINT("received an NCP(%ux) packet %d\n", ncp->prtcl, cpkt->code);
	switch(cpkt->code) {
	case ConfAck:
	case ConfNak:
	case ConfRej:
	case TermAck:
	case CodeRej:
	case ProtocolRej:
	case EchoReply:
		if(cpkt->ident != ncp->lastident)
			return;
	}
	ncp->lock();
	switch(cpkt->code) {
	case ConfReq:
		if(ncp->parsecfr(s, cpkt->ident, cpkt->rawdata,
				nhgets(cpkt->length) - LCP_HDR) == 0)
			switch(ncp->state) {
				case Pclosed:
					ncp->sendcfr(s);
					/* fall through */
				case Preqsent:
				case Packsent:
					ncp->state = Packsent;
					break;
				case Packrcvd:
					ncp->state = Popened;
					break;
				case Popened:
					ncp->sendcfr(s);
					ncp->state = Packsent;
					break;
				};
		break;
	case ConfAck:
		switch(ncp->state) {
		case Pclosed:
			ncp->ncpsenddata(s, TermAck, nil, 0, 0);
			break;
		case Preqsent:
			ncp->state = Packrcvd;
			break;
		case Packrcvd:
		case Popened:
			ncp->sendcfr(s);
			ncp->state = Preqsent;
			break;
		case Packsent:
			ncp->state = Popened;
			break;
		}
		break;
	case ConfNak:
		DPRINT("Configure NAK\n");
	case ConfRej:
		if(ncp->parsecfn(cpkt->rawdata, nhgets(cpkt->length)-LCP_HDR)==0) {
			ncp->sendcfr(s);
			if(ncp->state == Packsent)
				ncp->state = Packsent;
			else
				ncp->state = Preqsent;
		} else
			s->killppp("Configuration refused by peer...\n");
		break;
	case TermReq:
		ncp->ncpsenddata(s, TermAck, cpkt->rawdata,
				nhgets(cpkt->length) - LCP_HDR, cpkt->ident);
		ncp->state = Pclosed;
		break;
	case TermAck:
		switch(ncp->state) {
		case Popened:
			ncp->sendcfr(s);
			break;
		case Pclosing:
			ncp->state = Pclosed;
		}
		break;
	case CodeRej:
		recpkt = (void *) cpkt->rawdata;
		if(recpkt->code >= 0 && recpkt->code < NCP_MAXCODE) {
			if(ncp->state == Popened) {
				ncp->ncpsenddata(s, TermReq, nil, 0, 0);
				ncp->state = Pclosed;
			}
		}
		break;
	case ProtocolRej:
		rej = (void *) cpkt->rawdata;
		if(nhgets(rej->rejinfo) == PRTCL_IP ||
		   nhgets(rej->rejinfo) == PRTCL_LCP ||
		   nhgets(rej->rejinfo) == PRTCL_IPCP) {
			if(ncp->state == Popened) {
				ncp->ncpsenddata(s, TermReq, nil, 0, 0);
				ncp->state = Pclosed;
			}
		}
		break;
	case EchoReq:
		if(ncp->state == Popened) {
			/*
			 * make the received packet in place
			 */
			einfo = (void *) cpkt->rawdata;
			hnputl(einfo->magicno, ncp->magicno);
			ncp->ncpsenddata(s, EchoReply, (void*) einfo,
					 nhgets(cpkt->length)-LCP_HDR, cpkt->ident);
		}
		break;
	case EchoReply:
		einfo = (void *) cpkt->rawdata;
		n = nhgetl(einfo->magicno);
		if(n != 0 && n != ncp->peermagicno && n == ncp->magicno) 
			s->killppp("loopbacked connection");
		break;
	default:
		/*
		 * Code Reject
		 */
		ncp->ncpsenddata(s, CodeRej, (byte*) cpkt, size, cpkt->ident);
		if(ncp->state == Popened) {
			ncp->sendcfr(s);
			ncp->state = Preqsent;
		}
	case DiscardReq:
		break;
	}
	ncp->unlock();
}

void
Ncp.ncptimeout(Ncp *ncp, PPPstate *s)
{
	switch(ncp->state) {
	case Pclosed:
	case Preqsent:
	case Packrcvd:
	case Packsent:	
		ncp->sendcfr(s);
		if(ncp->state != Packsent)
			ncp->state = Preqsent;
		break;
	case Pclosing:
		ncp->ncpsenddata(s, TermReq, nil, 0, 0);
		ncp->state = Pclosed;
		break;
	}
	ncp->unlock();
}

/*
 * ctl-escapes certain bytes between two flags,
 * returns the number of bytes buffer expanded.
 */
int
PPPstate.hdlccpy(PPPstate *s, void *a1, void *a2, int n)
{
	byte *s1, *s2;
	sint padn;

	padn = 0;
	s1 = a1;
	s2 = a2;
	while(n > 0) {
		if(*s2 == PPP_flag || *s2 == HDLCesc ||
		   ((*s2 < 0x20) && (s->lcp.asyncctlmap & (1 << *s2)) != 0)) {
			*s1++ = HDLCesc;
			*s1++ = *s2++ ^ 0x20;
			padn++;
		} else
			*s1++ = *s2++;
		n--;
	}
	return padn;
}
/*
 * removes ctl-escaped sequences from a given buffer,
 * returns the number of bytes buffer shrunk
 */
int
PPPstate.hdlcprune(PPPstate *s, void *a1, int n)
{
	byte *s1, *s2;
	sint padn;

	padn = 0;
	s1 = s2 = a1;
	while(n > 0) {
		if(*s2 == HDLCesc) {
			*++s2 ^= 0x20;
			padn++;
			--n;
		} else if((*s2 < 0x20) && (s->lcp.asyncctlmap & (1 << *s2))) {
			++s2;
			padn++;
			--n;
		}
		*s1++ = *s2++;
		--n;
	}
	return padn;
}

/*
 * do a write to the ppp channel
 * size should not exceed maxsize
 */
void
PPPstate.pppencode(PPPstate *s, usint protocol, byte *buf, int size, int lastpkt)
{
	/*
	 * we have to reserve space in case the packet becomes twice
	 * its size after hdlc coding
	 */
	PPPpkt	q;
	byte	buffer[sizeof(PPPpkt)*2];
	byte	*pktp;
	usint	fcs;
	int	n;	/* number of raw bytes sent after hdlc */
	int	nhdr;	/* number of bytes in the header */

	nhdr = 0;
	pktp = &q.addr;
	if(protocol == PRTCL_LCP ||
	   ((s->lcp.options & (1<<AddrCtlCompression)) == 0)) {
		*pktp++ = PPP_addr;
		*pktp++ = PPP_ctl;
		nhdr += sizeof(q.addr) + sizeof(q.ctl);
	}
	if(protocol <= 0xff && ((s->lcp.options & (1<<PrtclCompression)) != 0)) {
		*pktp++ = protocol;
		++nhdr;
	} else {
		hnputs(pktp, protocol);
		pktp += sizeof(q.protocol);
		nhdr += sizeof(q.protocol);
	}
	memcpy(pktp, buf, size);
	fcs = calcfcs(PPP_initfcs, &q.addr, size + nhdr);
	fcsputs(pktp + size, ~fcs);
	size += nhdr + PPP_FCSSZ;
	buffer[0] = PPP_flag;
	n = s->hdlccpy(buffer+PPP_FLAGSZ, &q.addr, size);
	*(buffer + size + n + PPP_FLAGSZ) = PPP_flag;
	if(write(s->pppfd, buffer, size + n + lastpkt + PPP_FLAGSZ) <= 0)
		s->killppp("pppfd write");
}

/*
 * parse a full packet and call the appropriate protocol handler
 * we assume that the packet has space for ETHER_HDR bytes before it
 * VJ compression assumes space for another TCPIP_HDR bytes
 */
void
PPPstate.pppgotpacket(PPPstate *s, PPPpkt *pkt, int size)
{
	int	ndata;
	usint	fcs, protocol;
	byte	*data, rejbuf[PPP_DATASZ];

	fcs = calcfcs(PPP_initfcs, &pkt->addr, size - PPP_FLAGSZ);
	if(fcs != PPP_goodfcs) {
		s->cksumerr++;
		s->tcpcomp->err = 1;
		s->ilcomp->err = 1;
		return;
	}
	ndata = size - PPP_FCSSZ - PPP_FLAGSZ;
	/*
	 * skip address and ctl if one exists
	 */
	if((s->lcp.options & (1<<AddrCtlCompression)) &&
	   ((pkt->addr != PPP_addr) || (pkt->ctl  != PPP_ctl)))
		data = &pkt->addr;
	else if((pkt->addr == PPP_addr) && (pkt->ctl == PPP_ctl)) {
		data = pkt->protocol;
		ndata -= 2;
	} else
		return;
	/*
	 * get protocol
	 */
	protocol = 0;
	do {
		protocol = (protocol<<8) | *data++;
		--ndata;
	} while((protocol & 1) == 0);

	DPRINT("Got a packet in prtcl %lux size %d\n", protocol, ndata);
	/*
	 * drop if we have not set the connection yet
	 */
	if((s->lcp.state != Popened || s->ipcp.state != Popened) &&
	   protocol != PRTCL_LCP && protocol != PRTCL_IPCP)
			return;
	switch(protocol) {
	case PRTCL_ILCOMPRSSD:
	case PRTCL_ILUNCOMPRSSD:
		(data, ndata) = iluncompress(s, data, ndata, protocol);
		goto dowrite;
	case PRTCL_TCPCOMPRSSD:
	case PRTCL_TCPUNCOMPRSSD:
		(data, ndata) = tcpuncompress(s, data, ndata, protocol);
		/* fall through */
	case PRTCL_IP:
	dowrite:
		if(data != nil)
			write(ipfd, data - ETHER_HDR, ndata + ETHER_HDR);
		return;	
	case PRTCL_LCP:
		s->lcp.ncpparse(s, (Ctlpkt *) data, ndata);
		if(s->lcp.state != Popened)
			s->ipcp.state = Pclosed;
		DPRINT("LCP layer is in state %s\n", statestr[s->lcp.state]);
		break;
	case PRTCL_IPCP:
		if(s->lcp.state != Popened)
			return;
		s->ipcp.ncpparse(s, (Ctlpkt *) data, ndata);
		if(s->ipcp.state == Popened)
			s->allset = 1;
		DPRINT("IPCP layer is in state %s\n", statestr[s->ipcp.state]);
		break;
	default:
		hnputs(rejbuf, protocol);
		memcpy(rejbuf + 2, data, min(ndata, s->lcp.maxsize - 2));
		s->lcp.ncpsenddata(s, ProtocolRej, rejbuf, ndata + 2, 0);
		return;
	}
	if(s->lcp.state != Popened || s->ipcp.state != Popened)
		if(s->allset) {
			s->allset = 0;
			placeonq(s);
		}
}

void
PPPstate.pppdecode(PPPstate *s)
{
#define	BUFSTART	(buffer+ETHER_HDR+TCPIP_HDR)
	byte		buffer[5*sizeof(PPPpkt)];
	byte		*curp, *endp, *pbegin, *pend, *p;
	sint		n;

	rfork(RFNOTEG);
	curp = BUFSTART;
	for(;;) {
		n = read(s->pppfd, curp, sizeof(buffer) - (curp - buffer));
		if(n == 0 && sizeof(buffer) - (curp - buffer) == 0) {
			/*
			 * Buffer filled up with line noise, discard
			 */
			curp = BUFSTART;
			continue;
		} else if(n <= 0)
			s->killppp("decode exits");
		endp = curp + n;
		p = pbegin = BUFSTART;
		while(p < endp) {
			/*
			 * skip over garbage until we get to a flag,
			 * skip multiple flags and find the end of the packet
			 */
			for(; *p != PPP_flag && p < endp; ++p);
			for(; *p == PPP_flag && p < endp; ++p);
			pbegin = p-1;
			for(; *p != PPP_flag && p < endp; ++p);
			if(p >= endp)
				break;
			pend = p;
			/*
			 * got a complete packet
			 */
			pend -= s->hdlcprune(pbegin, pend - pbegin);
			s->pppgotpacket((PPPpkt *) pbegin, pend - pbegin);
		}
		memmove(BUFSTART, pbegin, p - pbegin);
		curp = BUFSTART + (p - pbegin);
	}
}

void
Ncp.lcpinit(Ncp *ncp)
{
	ncp->state = Pclosed;
	ncp->prtcl = PRTCL_LCP;
	ncp->asyncctlmap = 0xffffffff;
	ncp->magicno = 0xfeedface;
	ncp->peermagicno = 0;
	ncp->maxsize = ncp->peermaxsize = PPP_DATASZ;
	ncp->options =  (1<<AsyncCtlCharMap) | (1<<MagicNo) |
			 (1<<PrtclCompression) | (1<<AddrCtlCompression);
	ncp->ident = 17; /* random */
}

void
Ncp.ipcpinit(Ncp *ncp, byte *myip, byte *peerip)
{
	ncp->state = Pclosed;
	ncp->prtcl = PRTCL_IPCP;
	ncp->ident = 111;
	ncp->options = (1<<IPCompressionPrtcl);
	memmove(ncp->peeripaddr, peerip, sizeof(ncp->peeripaddr));
	memmove(ncp->myipaddr, myip, sizeof(ncp->myipaddr));
}

void
PPPstate.initppp(PPPstate *s, int fd, byte *myip, byte *peerip)
{
	DPRINT("Starting fd %d peerip %d.%d.%d.%d\n",
		fd, peerip[0], peerip[1], peerip[2], peerip[3]);
	s->pppfd = fd;
	s->lcp.lcpinit();
	s->ipcp.ipcpinit(myip, peerip);
	s->allset = 0;
	s->cksumerr = 0;
	compress_init(s);
	placeonq(s);
	proc	s->pppdecode();
}

void
PPPstate.killppp(PPPstate *s, byte *str)
{
	if(s->lcp.canlock()) {
		s->lcp.ncpsenddata(s, TermReq, nil, 0, 0);
		s->lcp.state = Pclosing;
		s->lcp.unlock();
	}
	close(s->pppfd);
	fprint(2, "ppp:%s:%r\n", str);
	exits("died");
}

void
writestr(int fd, byte *str)
{
	int n;

	n = strlen(str);
	if(write(fd, str, n) != n)
		fatal("cannot write string");
}

void
placeonq(PPPstate *s)
{
	ql.lock();
	queues[(curq + 1) % 2].putq(s);
	ql.unlock();
}

void
timer(void *a, byte *c)
{
	PPPstate	*s;
	Queue		*q;

	USED(a);
	DPRINT("Got a note %s\n", c);
	if(strcmp(c, "alarm") == 0) {
		if(ql.canlock()) {
			q = &queues[curq];
			s = q->getq();
			if(s != nil) {
				if(s->lcp.canlock())
					s->lcp.ncptimeout(s);
				if(s->lcp.state == Popened && s->ipcp.canlock())
					s->ipcp.ncptimeout(s);
				if(s->ipcp.state != Popened || s->lcp.state != Popened)
					queues[(curq + 1) % 2].putq(s);
			} else
				curq = (curq + 1) % 2;
			ql.unlock();
		}
		alarm(4000);
		noted(NCONT);
	}
	noted(NDFLT);
}

void
doalarms(void)
{
	/*
	 * stay around and handle alarms
	 */
	rfork(RFNOTEG);
	notify(timer);
	alarm(3000);
	for(;;)
		sleep(4000);
}

void
ipconfig(byte *myip, int server)
{
	int ipctl;
	byte cmd[128];

	if(bind("#|", "/net", MREPL) < 0)
		fatal("binding pipe");
	ipctl = open("/net/ctl", ORDWR);
	if(ipctl < 0)
		fatal("opening pipe ipctl");
	writestr(ipctl, "push internet");
	sprint(cmd, "setip %d.%d.%d.%d 255.255.255.0",
			myip[0], myip[1], myip[2], myip[3]);
	writestr(ipctl, cmd);
	DPRINT("IPCONFIG: %s\n", cmd);
	sprint(cmd, "routing %d", server);
	writestr(ipctl, cmd);
	ipfd = open("/net/data1", ORDWR);
	if(ipfd < 0)
		fatal("opening pipe ipfd");
}
