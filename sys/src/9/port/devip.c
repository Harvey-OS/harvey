#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include 	"arp.h"
#include 	"../port/ipdat.h"

#include	"devtab.h"

enum
{
	Nrprotocol	= 4,	/* Number of protocols supported by this driver */
	Nipsubdir	= 4,	/* Number of subdirectory entries per connection */
	Nfrag		= 32,	/* Ip reassembly queue entries */
	Nifc		= 4,	/* max interfaces */
};

int 	udpsum = 1;
Ipifc	*ipifc[Nrprotocol+1];
QLock	ipalloc;			/* Protocol port allocation lock */
Ipconv	**tcpbase;

Streamput	udpstiput, udpstoput, tcpstiput, tcpstoput;
Streamput	iliput, iloput, bsdiput, bsdoput;
Streamopen	udpstopen, tcpstopen, ilopen, bsdopen;
Streamclose	udpstclose, tcpstclose, ilclose, bsdclose;

Qinfo tcpinfo = { tcpstiput, tcpstoput, tcpstopen, tcpstclose, "tcp", 0, 1 };
Qinfo udpinfo = { udpstiput, udpstoput, udpstopen, udpstclose, "udp" };
Qinfo ilinfo  = { iliput,    iloput,    ilopen,    ilclose,    "il"  };
Qinfo bsdinfo = { bsdiput,   bsdoput,	bsdopen,   bsdclose,   "bsd", 0, 1 };

Qinfo *protocols[] = { &tcpinfo, &udpinfo, &ilinfo, 0 };

void
ipinitifc(Ipifc *ifc, Qinfo *stproto)
{
	ifc->conv = xalloc(Nipconv * sizeof(Ipconv*));
	ifc->protop = stproto;
	ifc->nconv = Nipconv;
	ifc->devp = &ipconvinfo;
	if(stproto != &udpinfo)
		ifc->listen = iplisten;
	ifc->clone = ipclonecon;
	ifc->ninfo = 3;
	ifc->info[0].name = "remote";
	ifc->info[0].fill = ipremotefill;
	ifc->info[1].name = "local";
	ifc->info[1].fill = iplocalfill;
	ifc->info[2].name = "status";
	ifc->info[2].fill = ipstatusfill;
	ifc->name = stproto->name;
}

void
ipreset(void)
{
	int i;

	for(i = 0; protocols[i]; i++) {
		ipifc[i] = xalloc(sizeof(Ipifc));
		ipinitifc(ipifc[i], protocols[i]);
		newqinfo(protocols[i]);
	}

	initfrag(Nfrag);
}

void
ipinit(void)
{
}

Chan *
ipattach(char *spec)
{
	int i;
	Chan *c;

	if(ipd[0].q == 0)
		error("no ip multiplexor");

	for(i = 0; protocols[i]; i++) {
		if(strcmp(spec, protocols[i]->name) == 0) {
			c = devattach('I', spec);
			c->dev = i;

			return (c);
		}
	}

	error(Enoproto);
	return 0;		/* not reached */
}

Chan *
ipclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
ipwalk(Chan *c, char *name)
{
	return netwalk(c, name, ipifc[c->dev]);
}

void
ipstat(Chan *c, char *db)
{
	netstat(c, db, ipifc[c->dev]);
}

Chan *
ipopen(Chan *c, int omode)
{
	return netopen(c, omode, ipifc[c->dev]);
}

int
ipclonecon(Chan *c)
{
	Ipconv *new;

	new = ipincoming(ipifc[c->dev], 0);
	if(new == 0)
		error(Enodev);
	return new->id;
}

/*
 *  create a new conversation structure if none exists for this conversation slot
 */
Ipconv*
ipcreateconv(Ipifc *ifc, int id)
{
	Ipconv **p;
	Ipconv *new;

	p = &ifc->conv[id];
	if(*p)
		return *p;
	qlock(ifc);
	p = &ifc->conv[id];
	if(*p){
		qunlock(ifc);
		return *p;
	}
	if(waserror()){
		qunlock(ifc);
		nexterror();
	}
	new = smalloc(sizeof(Ipconv));
	new->ifc = ifc;
	netadd(ifc, new, p - ifc->conv);
	new->ref = 1;
	*p = new;
	qunlock(ifc);
	poperror();
	return new;
}

/*
 *  allocate a conversation structure.
 */
Ipconv*
ipincoming(Ipifc *ifc, Ipconv *from)
{
	Ipconv *new;
	Ipconv **p, **etab;

	/* look for an unused existing conversation */
	etab = &ifc->conv[Nipconv];
	for(p = ifc->conv; p < etab; p++) {
		new = *p;
		if(new == 0)
			break;
		if(new->ref == 0 && canqlock(new)) {
			if(new->ref || ipconbusy(new)) {
				qunlock(new);
				continue;
			}
			if(from)	/* copy ownership from listening channel */
				netown(new, from->owner, 0);
			else		/* current user becomes owner */
				netown(new, u->p->user, 0);

			new->ref = 1;
			qunlock(new);
			return new;
		}	
	}

	/* create one */
	qlock(ifc);
	etab = &ifc->conv[Nipconv];
	for(p = ifc->conv; ; p++){
		if(p == etab){
			qunlock(ifc);
			return 0;
		}
		if(*p == 0)
			break;
	}
	if(waserror()){
		qunlock(ifc);
		nexterror();
	}
	new = smalloc(sizeof(Ipconv));
	new->ifc = ifc;
	netadd(ifc, new, p - ifc->conv);
	qlock(new);
	*p = new;
	qunlock(ifc);
	poperror();
	if(from)	/* copy ownership from listening channel */
		netown(new, from->owner, 0);
	else		/* current user becomes owner */
		netown(new, u->p->user, 0);
	new->ref = 1;
	qunlock(new);
	return new;
}

void
ipcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
ipremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
ipwstat(Chan *c, char *dp)
{
	netwstat(c, dp, ipifc[c->dev]);
}

void
ipclose(Chan *c)
{
	if(c->stream)
		streamclose(c);
}

long
ipread(Chan *c, void *a, long n, ulong offset)
{
	return netread(c, a, n, offset, ipifc[c->dev]);
}

long
ipwrite(Chan *c, char *a, long n, ulong offset)
{
	int 	m, backlog, type, priv;
	char 	*field[5], *ctlarg[5], buf[256];
	Port	port;
	Ipconv  *cp;
	uchar	dst[4];

	USED(offset);
	type = STREAMTYPE(c->qid.path);
	if (type == Sdataqid)
		return streamwrite(c, a, n, 0); 

	if (type != Sctlqid)
		error(Eperm);

	cp = ipcreateconv(ipifc[c->dev], STREAMID(c->qid.path));

	m = n;
	if(m > sizeof(buf)-1)
		m = sizeof(buf)-1;
	strncpy(buf, a, m);
	buf[m] = '\0';

	m = getfields(buf, field, 5, " ");
	if(m < 1)
		error(Ebadarg);

	if(strncmp(field[0], "connect", 7) == 0) {
		if(ipconbusy(cp))
			error(Enetbusy);

		if(m < 2)
			error(Ebadarg);

		switch(getfields(field[1], ctlarg, 5, "!")) {
		default:
			error(Eneedservice);
		case 2:
			priv = 0;
			break;
		case 3:
			if(strcmp(ctlarg[2], "r") != 0)
				error(Eperm);
			priv = 1;
			break;
		}
		cp->dst = ipparse(ctlarg[0]);
		hnputl(dst, cp->dst);
		cp->src = ipgetsrc(dst);
		cp->pdst = atoi(ctlarg[1]);

		/* If we have no local port assign one */
		qlock(&ipalloc);
		if(m == 3){
			port = atoi(field[2]);
			if(portused(ipifc[c->dev], cp->psrc)){
				qunlock(&ipalloc);	
				error(Einuse);
			}
			cp->psrc = port;
		}
		if(cp->psrc == 0)
			cp->psrc = nextport(ipifc[c->dev], priv);
		qunlock(&ipalloc);

		if(cp->ifc->protop == &tcpinfo)
			tcpstart(cp, TCP_ACTIVE, Streamhi, 0);
		else if(cp->ifc->protop == &ilinfo)
			ilstart(cp, IL_ACTIVE, 20);

		/*
		 *  stupid hack for BSD port's 512, 513, & 514
		 *  to make it harder for user to lie about his
		 *  identity. -- presotto
		 */
		switch(cp->pdst){
		case 512:
		case 513:
		case 514:
			pushq(c->stream, &bsdinfo);
			break;
		}

		memmove(cp->text, u->p->text, NAMELEN);
	}
	else if(strncmp(field[0], "disconnect", 10) == 0) {
		if(cp->ifc->protop != &udpinfo)
			error(Eperm);

		cp->dst = 0;
		cp->pdst = 0;
	}
	else if(strncmp(field[0], "bind", 4) == 0) {
		if(ipconbusy(cp))
			error(Enetbusy);

		port = atoi(field[1]);

		if(port){
			qlock(&ipalloc);
			if(portused(ipifc[c->dev], port)) {
				qunlock(&ipalloc);	
				error(Einuse);
			}
			cp->psrc = port;
			qunlock(&ipalloc);
		} else if(*field[1] != '*'){
			qlock(&ipalloc);
			cp->psrc = nextport(ipifc[c->dev], 0);
			qunlock(&ipalloc);
		} else
			cp->psrc = 0;
	}
	else if(strncmp(field[0], "announce", 8) == 0) {
		if(ipconbusy(cp))
			error(Enetbusy);

		if(m != 2)
			error(Ebadarg);

		port = atoi(field[1]);

		if(port){
			qlock(&ipalloc);
			if(portused(ipifc[c->dev], port)) {
				qunlock(&ipalloc);	
				error(Einuse);
			}
			cp->psrc = port;
			qunlock(&ipalloc);
		} else if(*field[1] != '*'){
			qlock(&ipalloc);
			cp->psrc = nextport(ipifc[c->dev], 0);
			qunlock(&ipalloc);
		} else
			cp->psrc = 0;

		if(cp->ifc->protop == &tcpinfo)
			tcpstart(cp, TCP_PASSIVE, Streamhi, 0);
		else if(cp->ifc->protop == &ilinfo)
			ilstart(cp, IL_PASSIVE, 10);

		if(cp->backlog == 0)
			cp->backlog = 3;

		memmove(cp->text, u->p->text, NAMELEN);
	}
	else if(strncmp(field[0], "backlog", 7) == 0) {
		if(m != 2)
			error(Ebadarg);
		backlog = atoi(field[1]);
		if(backlog == 0)
			error(Ebadarg);
		if(backlog > 5)
			backlog = 5;
		cp->backlog = backlog;
	}
	else if(strncmp(field[0], "headers", 7) == 0) {
		cp->headers = 1;	/* include addr/port in user packet */
	}
	else
		return streamwrite(c, a, n, 0);

	return n;
}

int
ipconbusy(Ipconv  *cp)
{
	if(cp->ifc->protop == &tcpinfo)
	if(cp->tcpctl.state != Closed)
		return 1;

	if(cp->ifc->protop == &ilinfo)
	if(cp->ilctl.state != Ilclosed)
		return 1;

	return 0;
}

void
udpstiput(Queue *q, Block *bp)
{
	PUTNEXT(q, bp);
}

/*
 * udprcvmsg - called by stip to multiplex udp ports onto conversations
 */
void
udprcvmsg(Ipifc *ifc, Block *bp)
{
	Ipconv *cp, **p, **etab;
	Udphdr *uh;
	Port   dport, sport;
	ushort sum, len;
	Ipaddr addr;
	Block *nbp;

	uh = (Udphdr *)(bp->rptr);

	/* Put back pseudo header for checksum */
	uh->Unused = 0;
	len = nhgets(uh->udplen);
	hnputs(uh->udpplen, len);

	addr = nhgetl(uh->udpsrc);

	if(udpsum && nhgets(uh->udpcksum)) {
		if(sum = ptcl_csum(bp, UDP_EHSIZE, len+UDP_PHDRSIZE)) {
			print("udp: checksum error %x (%d.%d.%d.%d)\n",
			      sum, fmtaddr(addr));
			
			freeb(bp);
			return;
		}
	}

	dport = nhgets(uh->udpdport);
	sport = nhgets(uh->udpsport);

	/* Look for a conversation structure for this port */
	etab = &ifc->conv[Nipconv];
	for(p = ifc->conv; p < etab; p++) {
		cp = *p;
		if(cp == 0)
			break;
		if(cp->ref)
		if(cp->psrc == dport)
		if(cp->pdst == 0 || cp->pdst == sport) {
			/* Trim the packet down to data size */
			len = len - (UDP_HDRSIZE-UDP_PHDRSIZE);
			bp = btrim(bp, UDP_EHSIZE+UDP_HDRSIZE, len);
			if(bp == 0)
				return;

			if(cp->headers){
				/* pass the src address to the stream head */
				nbp = allocb(Udphdrsize);
				nbp->next = bp;
				bp = nbp;
				hnputl(bp->wptr, addr);
				bp->wptr += 4;
				hnputs(bp->wptr, sport);
				bp->wptr += 2;
			} else {
				/* save the src address in the conversation struct */
			 	cp->dst = addr;
				cp->pdst = sport;
			}
			cp->src = 0;
			PUTNEXT(cp->readq, bp);
			return;
		}
	}

	freeb(bp);
}

void
udpstoput(Queue *q, Block *bp)
{
	Ipconv *cp;
	Udphdr *uh;
	int dlen, ptcllen, newlen;
	Ipaddr addr;
	Port port;
	if(bp->type == M_CTL) {
		PUTNEXT(q, bp);
		return;
	}

	cp = (Ipconv *)(q->ptr);
	if(cp->psrc == 0){
		freeb(bp);
		error(Enoport);
	}

	if(bp->type != M_DATA) {
		freeb(bp);
		error(Ebadctl);
	}

	/* Only allow atomic udp writes to form datagrams */
	if(!(bp->flags & S_DELIM)) {
		freeb(bp);
		error(Emsgsize);
	}

	/*
	 *  if we're in header mode, rip off the first 64 bytes as the
	 *  destination.  The destination is in ascii in the form
	 *	%d.%d.%d.%d!%d
	 */
	if(cp->headers){
		/* get user specified addresses */
		bp = pullup(bp, Udphdrsize);
		if(bp == 0){
			freeb(bp);
			error(Emsgsize);
		}
		addr = nhgetl(bp->rptr);
		bp->rptr += 4;
		port = nhgets(bp->rptr);
		bp->rptr += 2;
	} else
		addr = port = 0;

	/* Round packet up to even number of bytes and check we can
	 * send it
	 */
	dlen = blen(bp);
	if(dlen > UDP_DATMAX) {
		freeb(bp);
		error(Emsgsize);
	}
	newlen = dlen /*bround(bp, 1)*/;

	/* Make space to fit udp & ip & ethernet header */
	bp = padb(bp, UDP_EHSIZE + UDP_HDRSIZE);

	uh = (Udphdr *)(bp->rptr);

	ptcllen = dlen + (UDP_HDRSIZE-UDP_PHDRSIZE);
	uh->Unused = 0;
	uh->udpproto = IP_UDPPROTO;
	uh->frag[0] = 0;
	uh->frag[1] = 0;
	hnputs(uh->udpplen, ptcllen);
	hnputs(uh->udpsport, cp->psrc);
	if(cp->headers) {
		hnputl(uh->udpdst, addr);
		hnputs(uh->udpdport, port);
	}
	else {
		hnputl(uh->udpdst, cp->dst);
		hnputs(uh->udpdport, cp->pdst);
	}
	if(cp->src == 0)
		cp->src = ipgetsrc(uh->udpdst);
	hnputl(uh->udpsrc, cp->src);
	hnputs(uh->udplen, ptcllen);
	uh->udpcksum[0] = 0;
	uh->udpcksum[1] = 0;

	hnputs(uh->udpcksum, ptcl_csum(bp, UDP_EHSIZE, newlen+UDP_HDRSIZE));
	PUTNEXT(q, bp);
}

void
udpstclose(Queue *q)
{
	Ipconv *ipc;

	ipc = (Ipconv *)(q->ptr);

	ipc->headers = 0;
	ipc->psrc = 0;
	ipc->pdst = 0;
	ipc->dst = 0;
}

void
udpstopen(Queue *q, Stream *s)
{
	Ipconv *ipc;

	ipc = ipcreateconv(ipifc[s->dev], s->id);
	initipifc(ipifc[s->dev], IP_UDPPROTO, udprcvmsg);

	ipc->readq = RD(q);	
	RD(q)->ptr = (void *)ipc;
	WR(q)->next->ptr = (void *)ipc->ifc;
	WR(q)->ptr = (void *)ipc;
}

void
tcpstiput(Queue *q, Block *bp)
{
	PUTNEXT(q, bp);
}

tcproominq(void *a)
{
	return !((Tcpctl *)a)->sndfull;
}

void
tcpstoput(Queue *q, Block *bp)
{
	Ipconv *s;
	Tcpctl *tcb; 
	Block *f;

	s = (Ipconv *)(q->ptr);
	tcb = &s->tcpctl;

	if(bp->type == M_CTL) {
		PUTNEXT(q, bp);
		return;
	}

	if(s->psrc == 0)
		error(Enoport);

	/* Report asynchronous errors */
	if(s->err)
		error(s->err);

	switch(tcb->state) {
	case Listen:
		tcb->flags |= ACTIVE;
		tcpsndsyn(tcb);
		tcpsetstate(s, Syn_sent);

		/* No break */
	case Syn_sent:
	case Syn_received:
	case Established:
		/*
		 * Process flow control
	 	 */
		if(tcb->sndfull){
			qlock(&tcb->sndrlock);
			if(waserror()) {
				qunlock(&tcb->sndrlock);
				freeb(bp);
				nexterror();
			}
			sleep(&tcb->sndr, tcproominq, tcb);
			poperror();
			qunlock(&tcb->sndrlock);
		}

		/*
		 * Push data
		 */
		qlock(tcb);
		if(waserror()) {
			qunlock(tcb);
			nexterror();
		}

		/* make sure we don't queue onto something that just closed */
		switch(tcb->state) {
		case Syn_sent:
		case Syn_received:
		case Established:
			break;
		default:
			freeb(bp);
			error(Ehungup);
		}

		tcb->sndcnt += blen(bp);
		if(tcb->sndcnt > Streamhi)
			tcb->sndfull = 1;
		if(tcb->sndq == 0)
			tcb->sndq = bp;
		else {
			for(f = tcb->sndq; f->next; f = f->next)
				;
			f->next = bp;
		}
		tcprcvwin(s);
		tcpoutput(s);
		poperror();
		qunlock(tcb);
		break;

	case Close_wait:
	default:
		freeb(bp);
		error(Ehungup);
	}	
}

void
tcpstopen(Queue *q, Stream *s)
{
	Ipconv *ipc;
	Ipifc *ifc;
	Tcpctl *tcb;
	Block *bp;	
	static int tcpkprocs;

	/* Flow control and tcp timer processes */
	if(tcpkprocs == 0) {
		tcpkprocs = 1;
		kproc("tcpack", tcpackproc, 0);
		kproc("tcpflow", tcpflow, ipifc[s->dev]);

	}

	if(tcpbase == 0)
		tcpbase = ipifc[s->dev]->conv;
	ifc = ipifc[s->dev];
	initipifc(ifc, IP_TCPPROTO, tcpinput);
	ipc = ipcreateconv(ifc, s->id);

	ipc->readq = RD(q);
	ipc->readq->rp = &tcpflowr;
	ipc->err = 0;

	RD(q)->ptr = (void *)ipc;
	WR(q)->ptr = (void *)ipc;

	/* pass any waiting data upstream */
	tcb = &ipc->tcpctl;
	qlock(tcb);
	while(bp = getb(&tcb->rcvq))
		PUTNEXT(ipc->readq, bp);
	qunlock(tcb);
}

void
ipremotefill(Chan *c, char *buf, int len)
{
	Ipconv *cp;

	if(len < 24)
		error(Ebadarg);
	cp = ipcreateconv(ipifc[c->dev], STREAMID(c->qid.path));
	sprint(buf, "%d.%d.%d.%d!%d\n", fmtaddr(cp->dst), cp->pdst);
}

void
iplocalfill(Chan *c, char *buf, int len)
{
	Ipconv *cp;

	if(len < 24)
		error(Ebadarg);
	cp = ipcreateconv(ipifc[c->dev], STREAMID(c->qid.path));
	sprint(buf, "%d.%d.%d.%d!%d\n", fmtaddr(ipd[0].Myip[Myself]), cp->psrc);
}

void
ipstatusfill(Chan *c, char *buf, int len)
{
	Ipconv *cp;
	int connection;

	if(len < 64)
		error(Ebadarg);
	connection = STREAMID(c->qid.path);
	cp = ipcreateconv(ipifc[c->dev], connection);
	if(cp->ifc->protop == &tcpinfo)
		sprint(buf, "tcp/%d %d %s %s %s %d+%d\n", connection, cp->ref,
			tcpstate[cp->tcpctl.state],
			cp->tcpctl.flags & CLONE ? "listen" : "connect",
			cp->text,
			cp->tcpctl.srtt, cp->tcpctl.mdev);
	else if(cp->ifc->protop == &ilinfo)
		sprint(buf, "il/%d %d %s rtt %d ms %d csum\n", connection, cp->ref,
			ilstate[cp->ilctl.state], cp->ilctl.rtt,
			cp->ifc ? cp->ifc->chkerrs : 0);
	else
		sprint(buf, "%s/%d %d Datagram\n",
				cp->ifc->protop->name, connection, cp->ref);
}

int
iphavecon(Ipconv *s)
{
	return s->curlog;
}

int
iplisten(Chan *c)
{
	Ipconv *s;
	int connection;
	Ipconv **p, **etab, *new;

	connection = STREAMID(c->qid.path);
	s = ipcreateconv(ipifc[c->dev], connection);

	if(s->ifc->protop == &tcpinfo)
	if(s->tcpctl.state != Listen)
		error(Enolisten);

	if(s->ifc->protop == &ilinfo)
	if(s->ilctl.state != Illistening)
		error(Enolisten);

	for(;;) {
		qlock(&s->listenq);	/* single thread for the sleep */
		if(waserror()) {
			qunlock(&s->listenq);
			nexterror();
		}
		sleep(&s->listenr, iphavecon, s);
		poperror();
		etab = &ipifc[c->dev]->conv[Nipconv];
		for(p = ipifc[c->dev]->conv; p < etab; p++) {
			new = *p;
			if(new == 0)
				break;
			if(new->newcon == s) {
				qlock(s);
				s->curlog--;
				qunlock(s);
				new->newcon = 0;
				qunlock(&s->listenq);
				return new->id;
			}
		}
		qunlock(&s->listenq);
		print("iplisten: no newcon\n");
	}
	return -1;		/* not reached */
}

void
tcpstclose(Queue *q)
{
	Ipconv *s, *new;
	Ipconv **etab, **p;
	Tcpctl *tcb;

	s = (Ipconv *)(q->ptr);
	tcb = &s->tcpctl;

	/* Not interested in data anymore */
	qlock(s);
	s->readq = 0;
	qunlock(s);

	switch(tcb->state){
	case Listen:
		/*
		 *  reset any incoming calls to this listener
		 */
		qlock(s);
		s->backlog = 0;
		s->curlog = 0;
		etab = &tcpbase[Nipconv];
		for(p = tcpbase; p < etab; p++){
			new = *p;
			if(new == 0)
				break;
			if(new->newcon == s){
				new->newcon = 0;
				tcpflushincoming(new);
				new->ref = 0;
			}
		}
		qunlock(s);

		qlock(tcb);
		localclose(s, 0);
		qunlock(tcb);
		break;

	case Closed:
	case Syn_sent:
		qlock(tcb);
		localclose(s, 0);
		qunlock(tcb);
		break;

	case Syn_received:
	case Established:
		tcb->sndcnt++;
		tcb->snd.nxt++;
		tcpsetstate(s, Finwait1);
		goto output;

	case Close_wait:
		tcb->sndcnt++;
		tcb->snd.nxt++;
		tcpsetstate(s, Last_ack);
	output:
		qlock(tcb);
		if(waserror()) {
			qunlock(tcb);
			nexterror();
		}
		tcpoutput(s);
		poperror();
		qunlock(tcb);
		break;
	}
}


static	short	endian	= 1;
static	char*	aendian	= (char*)&endian;
#define	LITTLE	*aendian

ushort
ptcl_bsum(uchar *addr, int len)
{
	ulong losum, hisum, mdsum, x;
	ulong t1, t2;

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

	return losum & 0xffff;
}

ushort
ptcl_csum(Block *bp, int offset, int len)
{
	uchar *addr;
	ulong losum, hisum;
	ushort csum;
	int odd, blen, x;

	/* Correct to front of data area */
	while(bp && offset && offset >= BLEN(bp)) {
		offset -= BLEN(bp);
		bp = bp->next;
	}
	if(bp == 0)
		return 0;

	addr = bp->rptr + offset;
	blen = BLEN(bp) - offset;

	if(bp->next == 0)
		return ~ptcl_bsum(addr, MIN(len, blen)) & 0xffff;

	losum = 0;
	hisum = 0;

	odd = 0;
	while(len) {
		x = MIN(len, blen);
		csum = ptcl_bsum(addr, x);
		if(odd)
			hisum += csum;
		else
			losum += csum;
		odd = (odd+x) & 1;
		len -= x;

		bp = bp->next;
		if(bp == 0)
			break;
		blen = BLEN(bp);
		addr = bp->rptr;
	}

	losum += hisum>>8;
	losum += (hisum&0xff)<<8;
	while((csum = losum>>16) != 0)
		losum = csum + (losum & 0xffff);

	return ~losum & 0xffff;
}

Block *
btrim(Block *bp, int offset, int len)
{
	Block *nb, *startb;
	ulong l;

	if(blen(bp) < offset+len) {
		freeb(bp);
		return 0;
	}

	while((l = BLEN(bp)) < offset) {
		offset -= l;
		nb = bp->next;
		bp->next = 0;
		freeb(bp);
		bp = nb;
	}

	startb = bp;
	bp->rptr += offset;

	while((l = BLEN(bp)) < len) {
		len -= l;
		bp = bp->next;
	}

	bp->wptr -= (BLEN(bp) - len);
	bp->flags |= S_DELIM;

	if(bp->next) {
		freeb(bp->next);
		bp->next = 0;
	}

	return(startb);
}

Ipconv *
portused(Ipifc *ifc, Port port)
{
	Ipconv **p, **etab;
	Ipconv *cp;

	if(port == 0)
		return 0;

	etab = &ifc->conv[Nipconv];
	for(p = ifc->conv; p < etab; p++){
		cp = *p;
		if(cp == 0)
			break;
		if(cp->psrc == port) 
			return cp;
	}

	return 0;
}

static Port lastport[2] = { PORTALLOC-1, PRIVPORTALLOC-1 };

Port
nextport(Ipifc *ifc, int priv)
{
	Port base;
	Port max;
	Port *p;
	Port i;

	if(priv){
		base = PRIVPORTALLOC;
		max = UNPRIVPORTALLOC;
		p = &lastport[1];
	} else {
		base = PORTALLOC;
		max = PORTMAX;
		p = &lastport[0];
	}
	
	for(i = *p + 1; i < max; i++)
		if(!portused(ifc, i))
			return(*p = i);
	for(i = base ; i <= *p; i++)
		if(!portused(ifc, i))
			return(*p = i);

	return(0);
}

/* NEEDS HASHING ! */

Ipconv*
ip_conn(Ipifc *ifc, Port dst, Port src, Ipaddr dest)
{
	Ipconv **p, *s, **etab;

	/* Look for a conversation structure for this port */
	etab = &ifc->conv[Nipconv];
	for(p = ifc->conv; p < etab; p++) {
		s = *p;
		if(s == 0)
			break;
		if(s->psrc == dst)
		if(s->pdst == src)
		if(s->dst == dest || dest == 0)
			return s;
	}

	return 0;
}

/*
 *
 *  BSD authentication protocol, used on ports 512, 513, & 514.
 *  This makes sure that a user can only write the REAL user id.
 *
 *  q->ptr is number of nulls seen
 */
void
bsdopen(Queue *q, Stream *s)
{
	USED(s);
	RD(q)->ptr = q;
	WR(q)->ptr = q;
}
void
bsdclose(Queue *q)
{
	Block *bp;

	bp = allocb(0);
	bp->type = M_HANGUP;
	PUTNEXT(q->other, bp);
}
void
bsdiput(Queue *q, Block *bp)
{
	PUTNEXT(q, bp);
}
void
bsdoput(Queue *q, Block *bp)
{
	uchar *luser;
	Block *nbp;

	/* just pass it on if we've done authentication */
	if(q->ptr == 0 || bp->type != M_DATA){
		PUTNEXT(q, bp);
		return;
	}

	/* collect into a single block */
	qlock(&q->rlock);
	if(q->first == 0)
		q->first = pullup(bp, blen(bp));
	else{
		nbp = q->first;
		nbp->next = bp;
		q->first = pullup(nbp, blen(nbp));
	}
	bp = q->first;
	if(bp == 0){
		qunlock(&q->rlock);
		bsdclose(q);
		return;
	}

	/* look for 2 nulls to indicate stderr port and local user */
	luser = memchr(bp->rptr, 0, BLEN(bp));
	if(luser == 0){
		qunlock(&q->rlock);
		return;
	}
	luser++;
	if(memchr(luser, 0, bp->wptr - luser) == 0){
		qunlock(&q->rlock);
		return;
	}

	/* if luser is a lie, hangup */
	if(memcmp(luser, u->p->user, strlen(u->p->user)+1) != 0)
		bsdclose(q);

	/* mark queue as authenticated and pass data to remote side */
	q->ptr = 0;
	q->first = 0;
	bp->flags |= S_DELIM;
	PUTNEXT(q, bp);
	qunlock(&q->rlock);
}
