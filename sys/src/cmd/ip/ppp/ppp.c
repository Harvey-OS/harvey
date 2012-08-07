/*
 * ppp - point-to-point protocol, rfc1331
 */
#include <u.h>
#include <libc.h>
#include <auth.h>
#include <bio.h>
#include <ip.h>
#include <libsec.h>
#include <ndb.h>
#include "ppp.h"

#define PATH 128

static	int	baud;
static	int	nocompress;
static 	int	pppframing = 1;
static	int	noipcompress;
static	int	server;
static	int noauth;
static	int	nip;		/* number of ip interfaces */
static	int	dying;		/* flag to signal to all threads its time to go */
static	int	primary;	/* this is the primary IP interface */
static	char	*chatfile;

int	debug;
char*	LOG = "ppp";
char*	keyspec = "";

enum
{
	Rmagic=	0x12345
};

/*
 * Calculate FCS - rfc 1331
 */
ushort fcstab[256] =
{
      0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
      0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
      0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
      0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
      0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
      0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
      0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
      0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
      0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
      0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
      0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
      0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
      0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
      0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
      0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
      0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
      0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
      0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
      0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
      0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
      0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
      0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
      0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
      0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
      0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
      0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
      0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
      0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
      0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
      0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
      0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
      0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

static char *snames[] =
{
	"Sclosed",
	"Sclosing",
	"Sreqsent",
	"Sackrcvd",
	"Sacksent",
	"Sopened",
};

static	void		authtimer(PPP*);
static	void		chapinit(PPP*);
static	void		config(PPP*, Pstate*, int);
static	uchar*		escapeuchar(PPP*, ulong, uchar*, ushort*);
static	void		getchap(PPP*, Block*);
static	Block*		getframe(PPP*, int*);
static	void		getlqm(PPP*, Block*);
static	int		getopts(PPP*, Pstate*, Block*);
static	void		getpap(PPP*, Block*);
static	void		init(PPP*);
static	void		invalidate(Ipaddr);
static	void		ipinproc(PPP*);
static	char*		ipopen(PPP*);
static	void		mediainproc(PPP*);
static	void		newstate(PPP*, Pstate*, int);
static	int		nipifcs(char*);
static	void		papinit(PPP*);
static	void		pinit(PPP*, Pstate*);
static	void		ppptimer(PPP*);
static	void		printopts(Pstate*, Block*, int);
static	void		ptimer(PPP*, Pstate*);
static	int		putframe(PPP*, int, Block*);
static	void		putlqm(PPP*);
static	void		putndb(PPP*, char*);
static	void		putpaprequest(PPP*);
static	void		rcv(PPP*, Pstate*, Block*);
static	void		rejopts(PPP*, Pstate*, Block*, int);
static	void		sendechoreq(PPP*, Pstate*);
static	void		sendtermreq(PPP*, Pstate*);
static	void		setphase(PPP*, int);
static	void		terminate(PPP*, int);
static	int		validv4(Ipaddr);
static  void		dmppkt(char *s, uchar *a, int na);
static	void		getauth(PPP*);

void
pppopen(PPP *ppp, int mediain, int mediaout, char *net,
	Ipaddr ipaddr, Ipaddr remip,
	int mtu, int framing)
{
	ppp->ipfd = -1;
	ppp->ipcfd = -1;
	invalidate(ppp->remote);
	invalidate(ppp->local);
	invalidate(ppp->curremote);
	invalidate(ppp->curlocal);
	invalidate(ppp->dns[0]);
	invalidate(ppp->dns[1]);
	invalidate(ppp->wins[0]);
	invalidate(ppp->wins[1]);

	ppp->mediain = mediain;
	ppp->mediaout = mediaout;
	if(validv4(remip)){
		ipmove(ppp->remote, remip);
		ppp->remotefrozen = 1;
	}
	if(validv4(ipaddr)){
		ipmove(ppp->local, ipaddr);
		ppp->localfrozen = 1;
	}
	ppp->mtu = Defmtu;
	ppp->mru = mtu;
	ppp->framing = framing;
	ppp->net = net;

	init(ppp);
	switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
	case -1:
		sysfatal("forking mediainproc");
	case 0:
		mediainproc(ppp);
		terminate(ppp, 1);
		_exits(0);
	}
}

static void
init(PPP* ppp)
{
	if(ppp->inbuf == nil){
		ppp->inbuf = allocb(4096);
		if(ppp->inbuf == nil)
			abort();

		ppp->outbuf = allocb(4096);
		if(ppp->outbuf == nil)
			abort();

		ppp->lcp = mallocz(sizeof(*ppp->lcp), 1);
		if(ppp->lcp == nil)
			abort();
		ppp->lcp->proto = Plcp;
		ppp->lcp->state = Sclosed;

		ppp->ccp = mallocz(sizeof(*ppp->ccp), 1);
		if(ppp->ccp == nil)
			abort();
		ppp->ccp->proto = Pccp;
		ppp->ccp->state = Sclosed;

		ppp->ipcp = mallocz(sizeof(*ppp->ipcp), 1);
		if(ppp->ipcp == nil)
			abort();
		ppp->ipcp->proto = Pipcp;
		ppp->ipcp->state = Sclosed;

		ppp->chap = mallocz(sizeof(*ppp->chap), 1);
		if(ppp->chap == nil)
			abort();
		ppp->chap->proto = APmschap;
		ppp->chap->state = Cunauth;
		auth_freechal(ppp->chap->cs);
		ppp->chap->cs = nil;

		switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
		case -1:
			sysfatal("forking ppptimer");
		case 0:
			ppptimer(ppp);
			_exits(0);
		}
	}

	ppp->ctcp = compress_init(ppp->ctcp);
	pinit(ppp, ppp->lcp);
	setphase(ppp, Plink);
}

static void
setphase(PPP *ppp, int phase)
{
	int oldphase;

	oldphase = ppp->phase;

	ppp->phase = phase;
	switch(phase){
	default:
		sysfatal("ppp: unknown phase %d", phase);
	case Pdead:
		/* restart or exit? */
		pinit(ppp, ppp->lcp);
		setphase(ppp, Plink);
		break;
	case Plink:
		/* link down */
		switch(oldphase) {
		case Pauth:
			auth_freechal(ppp->chap->cs);
			ppp->chap->cs = nil;
			ppp->chap->state = Cunauth;
			break;
		case Pnet:
			auth_freechal(ppp->chap->cs);
			ppp->chap->cs = nil;
			ppp->chap->state = Cunauth;
			newstate(ppp, ppp->ccp, Sclosed);
			newstate(ppp, ppp->ipcp, Sclosed);
		}
		break;
	case Pauth:
		if(server)
			chapinit(ppp);
		else if(ppp->chap->proto == APpasswd)
			papinit(ppp);
		else
			setphase(ppp, Pnet);
		break;
	case Pnet:
		pinit(ppp, ppp->ccp);
		pinit(ppp, ppp->ipcp);
		break;
	case Pterm:
		/* what? */
		break;
	}
}

static void
pinit(PPP *ppp, Pstate *p)
{
	p->timeout = 0;

	switch(p->proto){
	case Plcp:
		ppp->magic = truerand();
		ppp->xctlmap = 0xffffffff;
		ppp->period = 0;
		p->optmask = 0xffffffff;
		if(!server)
			p->optmask &=  ~(Fauth|Fmtu);
		ppp->rctlmap = 0;
		ppp->ipcp->state = Sclosed;
		ppp->ipcp->optmask = 0xffffffff;

		p->echotimeout = 0;

		/* quality goo */
		ppp->timeout = 0;
		memset(&ppp->in, 0, sizeof(ppp->in));
		memset(&ppp->out, 0, sizeof(ppp->out));
		memset(&ppp->pin, 0, sizeof(ppp->pin));
		memset(&ppp->pout, 0, sizeof(ppp->pout));
		memset(&ppp->sin, 0, sizeof(ppp->sin));
		break;
	case Pccp:
		if(nocompress)
			p->optmask = 0;
		else
			p->optmask = Fcmppc;

		if(ppp->ctype != nil)
			(*ppp->ctype->fini)(ppp->cstate);
		ppp->ctype = nil;
		ppp->ctries = 0;
		ppp->cstate = nil;

		if(ppp->unctype)
			(*ppp->unctype->fini)(ppp->uncstate);
		ppp->unctype = nil;
		ppp->uncstate = nil;
		break;
	case Pipcp:
		p->optmask = 0xffffffff;
		ppp->ctcp = compress_init(ppp->ctcp);
		break;
	}
	p->confid = p->rcvdconfid = -1;
	config(ppp, p, 1);
	newstate(ppp, p, Sreqsent);
}

/*
 *  change protocol to a new state.
 */
static void
newstate(PPP *ppp, Pstate *p, int state)
{
	char *err;

	netlog("ppp: %ux %s->%s ctlmap %lux/%lux flags %lux mtu %ld mru %ld\n",
		p->proto, snames[p->state], snames[state], ppp->rctlmap,
		ppp->xctlmap, p->flags,
		ppp->mtu, ppp->mru);
	syslog(0, "ppp", "%ux %s->%s ctlmap %lux/%lux flags %lux mtu %ld mru %ld",
		p->proto, snames[p->state], snames[state], ppp->rctlmap,
		ppp->xctlmap, p->flags,
		ppp->mtu, ppp->mru);

	if(p->proto == Plcp) {
		if(state == Sopened)
			setphase(ppp, noauth? Pnet : Pauth);
		else if(state == Sclosed)
			setphase(ppp, Pdead);
		else if(p->state == Sopened)
			setphase(ppp, Plink);
	}

	if(p->proto == Pccp && state == Sopened) {
		if(ppp->unctype)
			(*ppp->unctype->fini)(ppp->uncstate);
		ppp->unctype = nil;
		ppp->uncstate = nil;
		if(p->optmask & Fcmppc) {
			ppp->unctype = &uncmppc;
			ppp->uncstate = (*uncmppc.init)(ppp);
		}
		if(p->optmask & Fcthwack){
			ppp->unctype = &uncthwack;
			ppp->uncstate = (*uncthwack.init)(ppp);
		}
	}

	if(p->proto == Pipcp && state == Sopened) {
		if(server && !noauth && ppp->chap->state != Cauthok)
			abort();

		err = ipopen(ppp);
		if(err != nil)
			sysfatal("%s", err);
	}

	p->state = state;
}

/* returns (protocol, information) */
static Block*
getframe(PPP *ppp, int *protop)
{
	uchar *p, *from, *to;
	int n, len, proto;
	ulong c;
	ushort fcs;
	Block *buf, *b;

	*protop = 0;
	if(ppp->framing == 0) {
		/* assume data is already framed */
		b = allocb(2000);
		len = b->lim - b->wptr;
		n = read(ppp->mediain, b->wptr, len);
 		dmppkt("RX", b->wptr, n);
		if(n <= 0 || n == len){
			freeb(b);

			return nil;
		}
		b->wptr += n;

		/* should probably copy to another block if small */

		if(pppframing && b->rptr[0] == PPP_addr && b->rptr[1] == PPP_ctl)
			b->rptr += 2;
		proto = *b->rptr++;
		if((proto & 0x1) == 0)
			proto = (proto<<8) | *b->rptr++;

		if(b->rptr >= b->wptr){
			freeb(b);
			return nil;
		}

		ppp->in.uchars += n;
		ppp->in.packets++;
		*protop = proto;
		netlog("getframe 0x%x\n", proto);
		return b;
	}

	buf = ppp->inbuf;
	for(;;){
		/* read till we hit a frame uchar or run out of room */
		for(p = buf->rptr; buf->wptr < buf->lim;){
			for(; p < buf->wptr; p++)
				if(*p == HDLC_frame)
					break;
			if(p != buf->wptr)
				break;

			len = buf->lim - buf->wptr;
			n = read(ppp->mediain, buf->wptr, len);
			if(n <= 0){
				syslog(0, LOG, "medium read returns %d: %r", n);
				buf->wptr = buf->rptr;
				return nil;
			}
 			dmppkt("RX", buf->wptr, n);
			buf->wptr += n;
		}

		/* copy into block, undoing escapes, and caculating fcs */
		fcs = PPP_initfcs;
		b = allocb(p - buf->rptr);
		to = b->wptr;
		for(from = buf->rptr; from != p;){
			c = *from++;
			if(c == HDLC_esc){
				if(from == p)
					break;
				c = *from++ ^ 0x20;
			} else if((c < 0x20) && (ppp->rctlmap & (1 << c)))
				continue;
			*to++ = c;
			fcs = (fcs >> 8) ^ fcstab[(fcs ^ c) & 0xff];
		}

		/* copy down what's left in buffer */
		p++;
		memmove(buf->rptr, p, buf->wptr - p);
		n = p - buf->rptr;
		buf->wptr -= n;
		b->wptr = to - 2;

		/* return to caller if checksum matches */
		if(fcs == PPP_goodfcs){
			if(b->rptr[0] == PPP_addr && b->rptr[1] == PPP_ctl)
				b->rptr += 2;
			proto = *b->rptr++;
			if((proto & 0x1) == 0)
				proto = (proto<<8) | *b->rptr++;
			if(b->rptr < b->wptr){
				ppp->in.uchars += n;
				ppp->in.packets++;
				*protop = proto;
				netlog("getframe 0x%x\n", proto);
				return b;
			}
		} else if(BLEN(b) > 0){
			if(ppp->ctcp)
				compress_error(ppp->ctcp);
			ppp->in.discards++;
			netlog("ppp: discard len %ld/%ld cksum %ux (%ux %ux %ux %ux)\n",
				BLEN(b), BLEN(buf), fcs, b->rptr[0],
				b->rptr[1], b->rptr[2], b->rptr[3]);
		}

		freeb(b);
	}
}

/* send a PPP frame */
static int
putframe(PPP *ppp, int proto, Block *b)
{
	Block *buf;
	uchar *to, *from;
	ushort fcs;
	ulong ctlmap;
	uchar c;
	Block *bp;

	ppp->out.packets++;

	if(proto == Plcp)
		ctlmap = 0xffffffff;
	else
		ctlmap = ppp->xctlmap;

	/* make sure we have head room */
	if(b->rptr - b->base < 4){
		b = padb(b, 4);
		b->rptr += 4;
	}

	netlog("ppp: putframe 0x%ux %ld\n", proto, b->wptr-b->rptr);

	/* add in the protocol and address, we'd better have left room */
	from = b->rptr;
	*--from = proto;
	if(!(ppp->lcp->flags&Fpc) || proto > 0x100 || proto == Plcp)
		*--from = proto>>8;
	if(pppframing && (!(ppp->lcp->flags&Fac) || proto == Plcp)){
		*--from = PPP_ctl;
		*--from = PPP_addr;
	}

	qlock(&ppp->outlock);
	buf = ppp->outbuf;

	if(ppp->framing == 0) {
		to = buf->rptr;
		for(bp = b; bp; bp = bp->next){
			if(bp != b)
				from = bp->rptr;
			memmove(to, from, bp->wptr-from);
			to += bp->wptr-from;
		}
	} else {
		/* escape and checksum the body */
		fcs = PPP_initfcs;
		to = buf->rptr;
	
		/* add frame marker */
		*to++ = HDLC_frame;

		for(bp = b; bp; bp = bp->next){
			if(bp != b)
				from = bp->rptr;
			for(; from < bp->wptr; from++){
				c = *from;
				if(c == HDLC_frame || c == HDLC_esc
				   || (c < 0x20 && ((1<<c) & ctlmap))){
					*to++ = HDLC_esc;
					*to++ = c ^ 0x20;
				} else 
					*to++ = c;
				fcs = (fcs >> 8) ^ fcstab[(fcs ^ c) & 0xff];
			}
		}

		/* add on and escape the checksum */
		fcs = ~fcs;
		c = fcs;
		if(c == HDLC_frame || c == HDLC_esc
		   || (c < 0x20 && ((1<<c) & ctlmap))){
			*to++ = HDLC_esc;
			*to++ = c ^ 0x20;
		} else 
			*to++ = c;
		c = fcs>>8;
		if(c == HDLC_frame || c == HDLC_esc
		   || (c < 0x20 && ((1<<c) & ctlmap))){
			*to++ = HDLC_esc;
			*to++ = c ^ 0x20;
		} else 
			*to++ = c;
	
		/* add frame marker */
		*to++ = HDLC_frame;
	}

	/* send */
	buf->wptr = to;
 	dmppkt("TX", buf->rptr, BLEN(buf));
	if(write(ppp->mediaout, buf->rptr, BLEN(buf)) < 0){
		qunlock(&ppp->outlock);
		return -1;
	}
	ppp->out.uchars += BLEN(buf);

	qunlock(&ppp->outlock);
	return 0;
}

Block*
alloclcp(int code, int id, int len, Lcpmsg **mp)
{
	Block *b;
	Lcpmsg *m;

	/*
	 *  leave room for header
	 */
	b = allocb(len);

	m = (Lcpmsg*)b->wptr;
	m->code = code;
	m->id = id;
	b->wptr += 4;

	*mp = m;
	return b;
}


static void
putlo(Block *b, int type, ulong val)
{
	*b->wptr++ = type;
	*b->wptr++ = 6;
	hnputl(b->wptr, val);
	b->wptr += 4;
}

static void
putv4o(Block *b, int type, Ipaddr val)
{
	*b->wptr++ = type;
	*b->wptr++ = 6;
	v6tov4(b->wptr, val);
	b->wptr += 4;
}

static void
putso(Block *b, int type, ulong val)
{
	*b->wptr++ = type;
	*b->wptr++ = 4;
	hnputs(b->wptr, val);
	b->wptr += 2;
}

static void
puto(Block *b, int type)
{
	*b->wptr++ = type;
	*b->wptr++ = 2;
}

/*
 *  send configuration request
 */
static void
config(PPP *ppp, Pstate *p, int newid)
{
	Block *b;
	Lcpmsg *m;
	int id;

	if(newid){
		id = p->id++;
		p->confid = id;
		p->timeout = Timeout;
	} else
		id = p->confid;
	b = alloclcp(Lconfreq, id, 256, &m);
	USED(m);

	switch(p->proto){
	case Plcp:
		if(p->optmask & Fctlmap)
			putlo(b, Octlmap, 0);	/* we don't want anything escaped */
		if(p->optmask & Fmagic)
			putlo(b, Omagic, ppp->magic);
		if(p->optmask & Fmtu)
			putso(b, Omtu, ppp->mru);
		if(p->optmask & Fauth) {
			*b->wptr++ = Oauth;
			*b->wptr++ = 5;
			hnputs(b->wptr, Pchap);
			b->wptr += 2;
			*b->wptr++ = ppp->chap->proto;
		}
		if(p->optmask & Fpc)
			puto(b, Opc);
		if(p->optmask & Fac)
			puto(b, Oac);
		break;
	case Pccp:
		if(p->optmask & Fcthwack)
			puto(b, Octhwack);
		else if(p->optmask & Fcmppc) {
			*b->wptr++ = Ocmppc;
			*b->wptr++ = 6;
			*b->wptr++ = 0;
			*b->wptr++ = 0;
			*b->wptr++ = 0;
			*b->wptr++ = 0x41;
		}
		break;
	case Pipcp:
		if(p->optmask & Fipaddr)
{syslog(0, "ppp", "requesting %I", ppp->local);
			putv4o(b, Oipaddr, ppp->local);
}
		if(primary && (p->optmask & Fipdns))
			putv4o(b, Oipdns, ppp->dns[0]);
		if(primary && (p->optmask & Fipdns2))
			putv4o(b, Oipdns2, ppp->dns[1]);
		if(primary && (p->optmask & Fipwins))
			putv4o(b, Oipwins, ppp->wins[0]);
		if(primary && (p->optmask & Fipwins2))
			putv4o(b, Oipwins2, ppp->wins[1]);
		/*
		 * don't ask for header compression while data compression is still pending.
		 * perhaps we should restart ipcp negotiation if compression negotiation fails.
		 */
		if(!noipcompress && !ppp->ccp->optmask && (p->optmask & Fipcompress)) {
			*b->wptr++ = Oipcompress;
			*b->wptr++ = 6;
			hnputs(b->wptr, Pvjctcp);
			b->wptr += 2;
			*b->wptr++ = MAX_STATES-1;
			*b->wptr++ = 1;
		}
		break;
	}
	hnputs(m->len, BLEN(b));
	printopts(p, b, 1);
	putframe(ppp, p->proto, b);
	freeb(b);
}

static void
getipinfo(PPP *ppp)
{
	char *av[3];
	int ndns, nwins;
	char ip[64];
	Ndbtuple *t, *nt;

	if(!validv4(ppp->local))
		return;

	av[0] = "dns";
	av[1] = "wins";
	sprint(ip, "%I", ppp->local);
	t = csipinfo(ppp->net, "ip", ip, av, 2);
	ndns = nwins = 0;
	for(nt = t; nt != nil; nt = nt->entry){
		if(strcmp(nt->attr, "dns") == 0){
			if(ndns < 2)
				parseip(ppp->dns[ndns++], nt->val);
		} else if(strcmp(nt->attr, "wins") == 0){
			if(nwins < 2)
				parseip(ppp->wins[nwins++], nt->val);
		}
	}
	if(t != nil)
		ndbfree(t);
}

/*
 *  parse configuration request, sends an ack or reject packet
 *
 *	returns:	-1 if request was syntacticly incorrect
 *			 0 if packet was accepted
 *			 1 if packet was rejected
 */
static int
getopts(PPP *ppp, Pstate *p, Block *b)
{
	Lcpmsg *m, *repm;	
	Lcpopt *o;
	uchar *cp, *ap;
	ulong rejecting, nacking, flags, proto, chapproto;
	ulong mtu, ctlmap, period;
	ulong x;
	Block *repb;
	Comptype *ctype;
	Ipaddr ipaddr;

	rejecting = 0;
	nacking = 0;
	flags = 0;

	/* defaults */
	invalidate(ipaddr);
	mtu = ppp->mtu;
	ctlmap = 0xffffffff;
	period = 0;
	ctype = nil;
	chapproto = 0;

	m = (Lcpmsg*)b->rptr;
	repb = alloclcp(Lconfack, m->id, BLEN(b), &repm);

	/* copy options into ack packet */
	memmove(repm->data, m->data, b->wptr - m->data);
	repb->wptr += b->wptr - m->data;

	/* look for options we don't recognize or like */
	for(cp = m->data; cp < b->wptr; cp += o->len){
		o = (Lcpopt*)cp;
		if(cp + o->len > b->wptr || o->len==0){
			freeb(repb);
			netlog("ppp: bad option length %ux\n", o->type);
			return -1;
		}

		switch(p->proto){
		case Plcp:
			switch(o->type){
			case Oac:
				flags |= Fac;
				continue;
			case Opc:
				flags |= Fpc;
				continue;
			case Omtu:
				mtu = nhgets(o->data);
				continue;
			case Omagic:
				if(ppp->magic == nhgetl(o->data))
					netlog("ppp: possible loop\n");
				continue;
			case Octlmap:
				ctlmap = nhgetl(o->data);
				continue;
			case Oquality:
				proto = nhgets(o->data);
				if(proto != Plqm)
					break;
				x = nhgetl(o->data+2)*10;
				period = (x+Period-1)/Period;
				continue;
			case Oauth:
				proto = nhgets(o->data);
				if(proto == Ppasswd && !server){
					chapproto = APpasswd;
					continue;
				}
				if(proto != Pchap)
					break;
				if(o->data[2] != APmd5 && o->data[2] != APmschap)
					break;
				chapproto = o->data[2];
				continue;
			}
			break;
		case Pccp:
			if(nocompress)
				break;
			switch(o->type){
			case Octhwack:
				break;
			/*
				if(o->len == 2){
					ctype = &cthwack;
					continue;
				}
				if(!nacking){
					nacking = 1;
					repb->wptr = repm->data;
					repm->code = Lconfnak;
				}
				puto(repb, Octhwack);
				continue;
			*/
			case Ocmppc:
				x = nhgetl(o->data);

				// hack for Mac
				// if(x == 0)
				//	continue;

				/* stop ppp loops */
				if((x&0x41) == 0 || ppp->ctries++ > 5) {
					/*
					 * turn off requests as well - I don't think this
					 * is needed in the standard
					 */
					p->optmask &= ~Fcmppc;
					break;
				}
				if(rejecting)
					continue;
				if(x & 1) {
					ctype = &cmppc;
					ppp->sendencrypted = (o->data[3]&0x40) == 0x40;
					continue;
				}
				if(!nacking){
					nacking = 1;
					repb->wptr = repm->data;
					repm->code = Lconfnak;
				}
				*repb->wptr++ = Ocmppc;
				*repb->wptr++ = 6;
				*repb->wptr++ = 0;
				*repb->wptr++ = 0;
				*repb->wptr++ = 0;
				*repb->wptr++ = 0x41;
				continue;
			}
			break;
		case Pipcp:
			switch(o->type){
			case Oipaddr:	
				v4tov6(ipaddr, o->data);
				if(!validv4(ppp->remote))
					continue;
				if(!validv4(ipaddr) && !rejecting){
					/* other side requesting an address */
					if(!nacking){
						nacking = 1;
						repb->wptr = repm->data;
						repm->code = Lconfnak;
					}
					putv4o(repb, Oipaddr, ppp->remote);
				}
				continue;
			case Oipdns:
				ap = ppp->dns[0];
				goto ipinfo;
			case Oipdns2:	
				ap = ppp->dns[1];
				goto ipinfo;
			case Oipwins:	
				ap = ppp->wins[0];
				goto ipinfo;
			case Oipwins2:
				ap = ppp->wins[1];
				goto ipinfo;
			ipinfo:
				if(!validv4(ap))
					getipinfo(ppp);
				if(!validv4(ap))
					break;
				v4tov6(ipaddr, o->data);
				if(!validv4(ipaddr) && !rejecting){
					/* other side requesting an address */
					if(!nacking){
						nacking = 1;
						repb->wptr = repm->data;
						repm->code = Lconfnak;
					}
					putv4o(repb, o->type, ap);
				}
				continue;
			case Oipcompress:
				/*
				 * don't compress tcp header if we've negotiated data compression.
				 * tcp header compression has very poor performance if there is an error.
				 */
				proto = nhgets(o->data);
				if(noipcompress || proto != Pvjctcp || ppp->ctype != nil)
					break;
				if(compress_negotiate(ppp->ctcp, o->data+2) < 0)
					break;
				flags |= Fipcompress;
				continue;
			}
			break;
		}

		/* come here if option is not recognized */
		if(!rejecting){
			rejecting = 1;
			repb->wptr = repm->data;
			repm->code = Lconfrej;
		}
		netlog("ppp: bad %ux option %d\n", p->proto, o->type);
		memmove(repb->wptr, o, o->len);
		repb->wptr += o->len;
	}

	/* permanent changes only after we know that we liked the packet */
	if(!rejecting && !nacking){
		switch(p->proto){
		case Plcp:
			ppp->period = period;
			ppp->xctlmap = ctlmap;
			if(mtu > Maxmtu)
				mtu = Maxmtu;
			if(mtu < Minmtu)
				mtu = Minmtu;
			ppp->mtu = mtu;
			if(chapproto)
				ppp->chap->proto = chapproto;
			
			break;
		case Pccp:
			if(ppp->ctype != nil){
				(*ppp->ctype->fini)(ppp->cstate);
				ppp->cstate = nil;
			}
			ppp->ctype = ctype;
			if(ctype)
				ppp->cstate = (*ctype->init)(ppp);
			break;
		case Pipcp:
			if(validv4(ipaddr) && ppp->remotefrozen == 0)
 				ipmove(ppp->remote, ipaddr);
			break;
		}
		p->flags = flags;
	}

	hnputs(repm->len, BLEN(repb));
	printopts(p, repb, 1);
	putframe(ppp, p->proto, repb);
	freeb(repb);

	return rejecting || nacking;
}
static void
dmppkt(char *s, uchar *a, int na)
{
	int i;

	if (debug < 3)
		return;

	fprint(2, "%s", s);
	for(i = 0; i < na; i++)
		fprint(2, " %.2ux", a[i]);
	fprint(2, "\n");
}

static void
dropoption(Pstate *p, Lcpopt *o)
{
	unsigned n = o->type;

	switch(n){
	case Oipaddr:
		break;
	case Oipdns:
		p->optmask &= ~Fipdns;
		break;
	case Oipwins:
		p->optmask &= ~Fipwins;
		break;
	case Oipdns2:
		p->optmask &= ~Fipdns2;
		break;
	case Oipwins2:
		p->optmask &= ~Fipwins2;
		break;
	default:
		if(o->type < 8*sizeof(p->optmask))
			p->optmask &= ~(1<<o->type);
		break;
	}
}

/*
 *  parse configuration rejection, just stop sending anything that they
 *  don't like (except for ipcp address nak).
 */
static void
rejopts(PPP *ppp, Pstate *p, Block *b, int code)
{
	Lcpmsg *m;
	Lcpopt *o;
	uchar newip[IPaddrlen];

	/* just give up trying what the other side doesn't like */
	m = (Lcpmsg*)b->rptr;
	for(b->rptr = m->data; b->rptr < b->wptr; b->rptr += o->len){
		o = (Lcpopt*)b->rptr;
		if(b->rptr + o->len > b->wptr){
			netlog("ppp: bad roption length %ux\n", o->type);
			return;
		}

		if(code == Lconfrej){
			dropoption(p, o);
			netlog("ppp: %ux rejecting %d\n",
					p->proto, o->type);
			continue;
		}

		switch(p->proto){
		case Plcp:
			switch(o->type){
			case Octlmap:
				ppp->rctlmap = nhgetl(o->data);
				break;
			case Oauth:
				/* don't allow client to request no auth */
				/* could try different auth protocol here */
				fprint(2, "ppp: can not reject CHAP\n");
				exits("ppp: CHAP");
				break;
			default:
				if(o->type < 8*sizeof(p->optmask))
					p->optmask &= ~(1<<o->type);
				break;
			};
			break;
		case Pccp:
			switch(o->type){
			default:
				dropoption(p, o);
				break;
			}
			break;
		case Pipcp:
			switch(o->type){
			case Oipaddr:
syslog(0, "ppp", "rejected addr %I with %V", ppp->local, o->data);
				/* if we're a server, don't let other end change our addr */
				if(ppp->localfrozen){
					dropoption(p, o);
					break;
				}

				/* accept whatever server tells us */
				if(!validv4(ppp->local)){
					v4tov6(ppp->local, o->data);
					dropoption(p, o);
					break;
				}

				/* if he didn't like our addr, ask for a generic one */
				v4tov6(newip, o->data);
				if(!validv4(newip)){
					invalidate(ppp->local);
					break;
				}

				/* if he gives us something different, use it anyways */
				v4tov6(ppp->local, o->data);
				dropoption(p, o);
				break;
			case Oipdns:
				if (!validv4(ppp->dns[0])){
					v4tov6(ppp->dns[0], o->data);
					dropoption(p, o);
					break;
				}
				v4tov6(newip, o->data);
				if(!validv4(newip)){
					invalidate(ppp->dns[0]);
					break;
				}
				v4tov6(ppp->dns[0], o->data);
				dropoption(p, o);
				break;
			case Oipwins:
				if (!validv4(ppp->wins[0])){
					v4tov6(ppp->wins[0], o->data);
					dropoption(p, o);
					break;
				}
				v4tov6(newip, o->data);
				if(!validv4(newip)){
					invalidate(ppp->wins[0]);
					break;
				}
				v4tov6(ppp->wins[0], o->data);
				dropoption(p, o);
				break;
			case Oipdns2:
				if (!validv4(ppp->dns[1])){
					v4tov6(ppp->dns[1], o->data);
					dropoption(p, o);
					break;
				}
				v4tov6(newip, o->data);
				if(!validv4(newip)){
					invalidate(ppp->dns[1]);
					break;
				}
				v4tov6(ppp->dns[1], o->data);
				dropoption(p, o);
				break;
			case Oipwins2:
				if (!validv4(ppp->wins[1])){
					v4tov6(ppp->wins[1], o->data);
					dropoption(p, o);
					break;
				}
				v4tov6(newip, o->data);
				if(!validv4(newip)){
					invalidate(ppp->wins[1]);
					break;
				}
				v4tov6(ppp->wins[1], o->data);
				dropoption(p, o);
				break;
			default:
				dropoption(p, o);
				break;
			}
			break;
		}
	}
}


/*
 *  put a messages through the lcp or ipcp state machine.  They are
 *  very similar.
 */
static void
rcv(PPP *ppp, Pstate *p, Block *b)
{
	ulong len;
	int err;
	Lcpmsg *m;
	int proto;

	if(BLEN(b) < 4){
		netlog("ppp: short lcp message\n");
		freeb(b);
		return;
	}
	m = (Lcpmsg*)b->rptr;
	len = nhgets(m->len);
	if(BLEN(b) < len){
		netlog("ppp: short lcp message\n");
		freeb(b);
		return;
	}

	netlog("ppp: %ux rcv %d len %ld id %d/%d/%d\n",
		p->proto, m->code, len, m->id, p->confid, p->id);

	if(p->proto != Plcp && ppp->lcp->state != Sopened){
		netlog("ppp: non-lcp with lcp not open\n");
		freeb(b);
		return;
	}

	qlock(ppp);
	switch(m->code){
	case Lconfreq:
		printopts(p, b, 0);
		err = getopts(ppp, p, b);
		if(err < 0)
			break;

		if(m->id == p->rcvdconfid)
			break;			/* don't change state for duplicates */

		switch(p->state){
		case Sackrcvd:
			if(err)
				break;
			newstate(ppp, p, Sopened);
			break;
		case Sclosed:
		case Sopened:
			config(ppp, p, 1);
			if(err == 0)
				newstate(ppp, p, Sacksent);
			else
				newstate(ppp, p, Sreqsent);
			break;
		case Sreqsent:
		case Sacksent:
			if(err == 0)
				newstate(ppp, p, Sacksent);
			else
				newstate(ppp, p, Sreqsent);
			break;
		}
		break;
	case Lconfack:
		if(p->confid != m->id){
			/* ignore if it isn't the message we're sending */
			netlog("ppp: dropping confack\n");
			break;
		}
		p->confid = -1;		/* ignore duplicates */
		p->id++;		/* avoid sending duplicates */

		netlog("ppp: recv confack\n");
		switch(p->state){
		case Sopened:
		case Sackrcvd:
			config(ppp, p, 1);
			newstate(ppp, p, Sreqsent);
			break;
		case Sreqsent:
			newstate(ppp, p, Sackrcvd);
			break;
		case Sacksent:
			newstate(ppp, p, Sopened);
			break;
		}
		break;
	case Lconfrej:
	case Lconfnak:
		if(p->confid != m->id) {
			/* ignore if it isn't the message we're sending */
			netlog("ppp: dropping confrej or confnak\n");
			break;
		}
		p->confid = -1;		/* ignore duplicates */
		p->id++;		/* avoid sending duplicates */

		switch(p->state){
		case Sopened:
		case Sackrcvd:
			config(ppp, p, 1);
			newstate(ppp, p, Sreqsent);
			break;
		case Sreqsent:
		case Sacksent:
			printopts(p, b, 0);
			rejopts(ppp, p, b, m->code);
			config(ppp, p, 1);
			break;
		}
		break;
	case Ltermreq:
		m->code = Ltermack;
		putframe(ppp, p->proto, b);

		switch(p->state){
		case Sackrcvd:
		case Sacksent:
			newstate(ppp, p, Sreqsent);
			break;
		case Sopened:
			newstate(ppp, p, Sclosing);
			break;
		}
		break;
	case Ltermack:
		if(p->termid != m->id)	/* ignore if it isn't the message we're sending */
			break;

		if(p->proto == Plcp)
			ppp->ipcp->state = Sclosed;
		switch(p->state){
		case Sclosing:
			newstate(ppp, p, Sclosed);
			break;
		case Sackrcvd:
			newstate(ppp, p, Sreqsent);
			break;
		case Sopened:
			config(ppp, p, 0);
			newstate(ppp, p, Sreqsent);
			break;
		}
		break;
	case Lcoderej:
		//newstate(ppp, p, Sclosed);
		syslog(0, LOG, "code reject %d\n", m->data[0]);
		break;
	case Lprotorej:
		proto = nhgets(m->data);
		netlog("ppp: proto reject %ux\n", proto);
		if(proto == Pccp)
			newstate(ppp, ppp->ccp, Sclosed);
		break;
	case Lechoreq:
		if(BLEN(b) < 8){
			netlog("ppp: short lcp echo request\n");
			freeb(b);
			return;
		}
		m->code = Lechoack;
		hnputl(m->data, ppp->magic);
		putframe(ppp, p->proto, b);
		break;
	case Lechoack:
		p->echoack = 1;
		break;
	case Ldiscard:
		/* nothing to do */
		break;
	case Lresetreq:
		if(p->proto != Pccp)
			break;
		ppp->stat.compreset++;
		if(ppp->ctype != nil)
			b = (*ppp->ctype->resetreq)(ppp->cstate, b);
		if(b != nil) {
			m = (Lcpmsg*)b->rptr;
			m->code = Lresetack;
			putframe(ppp, p->proto, b);
		}
		break;
	case Lresetack:
		if(p->proto != Pccp)
			break;
		if(ppp->unctype != nil)
			(*ppp->unctype->resetack)(ppp->uncstate, b);
		break;
	}

	qunlock(ppp);
	freeb(b);
}

/*
 *  timer for protocol state machine
 */
static void
ptimer(PPP *ppp, Pstate *p)
{
	if(p->state == Sopened || p->state == Sclosed)
		return;

	p->timeout--;
	switch(p->state){
	case Sclosing:
		sendtermreq(ppp, p);
		break;
	case Sreqsent:
	case Sacksent:
		if(p->timeout <= 0)
			newstate(ppp, p, Sclosed);
		else {
			config(ppp, p, 0);
		}
		break;
	case Sackrcvd:
		if(p->timeout <= 0)
			newstate(ppp, p, Sclosed);
		else {
			config(ppp, p, 0);
			newstate(ppp, p, Sreqsent);
		}
		break;
	}
}

/* paptimer -- pap timer event handler
 *
 * If PAP authorization hasn't come through, resend an authreqst.  If
 * the maximum number of requests have been sent (~ 30 seconds), give
 * up.
 *
 */
static void
authtimer(PPP* ppp)
{
	if(ppp->chap->proto != APpasswd)
		return;

	if(ppp->chap->id < 21)
		putpaprequest(ppp);
	else {
		terminate(ppp, 0);
		netlog("ppp: pap timed out--not authorized\n");
	}
}


/*
 *  timer for ppp
 */
static void
ppptimer(PPP *ppp)
{
	while(!dying){
		sleep(Period);
		qlock(ppp);

		netlog("ppp: ppptimer\n");
		ptimer(ppp, ppp->lcp);
		if(ppp->lcp->state == Sopened) {
			switch(ppp->phase){
			case Pnet:
				ptimer(ppp, ppp->ccp);
				ptimer(ppp, ppp->ipcp);
				break;
			case Pauth:
				authtimer(ppp);
				break;
			}
		}

		/* link quality measurement */
		if(ppp->period && --(ppp->timeout) <= 0){
			ppp->timeout = ppp->period;
			putlqm(ppp);
		}

		qunlock(ppp);
	}
}

static void
setdefroute(char *net, Ipaddr gate)
{
	int fd;
	char path[128];

	snprint(path, sizeof path, "%s/iproute", net);
	fd = open(path, ORDWR);
	if(fd < 0)
		return;
	fprint(fd, "add 0 0 %I", gate);
	close(fd);
}

enum
{
	Mofd=	32,
};
struct
{
	Lock;

	int	fd[Mofd];
	int	cfd[Mofd];
	int	n;
} old;

static char*
ipopen(PPP *ppp)
{
	static int ipinprocpid;
	int n, cfd, fd;
	char path[128];
	char buf[128];

	if(ipinprocpid <= 0){
		snprint(path, sizeof path, "%s/ipifc/clone", ppp->net);
		cfd = open(path, ORDWR);
		if(cfd < 0)
			return "can't open ip interface";

		n = read(cfd, buf, sizeof(buf) - 1);
		if(n <= 0){
			close(cfd);
			return "can't open ip interface";
		}
		buf[n] = 0;

		netlog("ppp: setting up IP interface local %I remote %I (valid %d)\n",
			ppp->local, ppp->remote, validv4(ppp->remote));
		if(!validv4(ppp->remote))
			ipmove(ppp->remote, ppp->local);

		snprint(path, sizeof path, "%s/ipifc/%s/data", ppp->net, buf);
		fd = open(path, ORDWR);
		if(fd < 0){
			close(cfd);
			return "can't open ip interface";
		}

		if(fprint(cfd, "bind pkt") < 0)
			return "binding pkt to ip interface";
		if(fprint(cfd, "add %I 255.255.255.255 %I %lud proxy", ppp->local,
			ppp->remote, ppp->mtu-10) < 0){
			close(cfd);
			return "can't set addresses";
		}
		if(primary)
			setdefroute(ppp->net, ppp->remote);
		ppp->ipfd = fd;
		ppp->ipcfd = cfd;

		/* signal main() that ip is configured */
		rendezvous((void*)Rmagic, 0);

		switch(ipinprocpid = rfork(RFPROC|RFMEM|RFNOWAIT)){
		case -1:
			sysfatal("forking ipinproc");
		case 0:
			ipinproc(ppp);
			terminate(ppp, 1);
			_exits(0);
		}
	} else {
		/* we may have changed addresses */
		if(ipcmp(ppp->local, ppp->curlocal) != 0 ||
		   ipcmp(ppp->remote, ppp->curremote) != 0){
			snprint(buf, sizeof buf, "remove %I 255.255.255.255 %I",
			    ppp->curlocal, ppp->curremote);
			if(fprint(ppp->ipcfd, "%s", buf) < 0)
				syslog(0, "ppp", "can't %s: %r", buf);
			snprint(buf, sizeof buf, "add %I 255.255.255.255 %I %lud proxy",
			    ppp->local, ppp->remote, ppp->mtu-10);
			if(fprint(ppp->ipcfd, "%s", buf) < 0)
				syslog(0, "ppp", "can't %s: %r", buf);
		}
		syslog(0, "ppp", "%I/%I -> %I/%I", ppp->curlocal, ppp->curremote,
		   ppp->local, ppp->remote);
	}
	ipmove(ppp->curlocal, ppp->local);
	ipmove(ppp->curremote, ppp->remote);

	return nil;
}

/* return next input IP packet */
Block*
pppread(PPP *ppp)
{
	Block *b, *reply;
	int proto, len;
	Lcpmsg *m;

	while(!dying){
		b = getframe(ppp, &proto);
		if(b == nil)
			return nil;

Again:
		switch(proto){
		case Plcp:
			rcv(ppp, ppp->lcp, b);
			break;
		case Pccp:
			rcv(ppp, ppp->ccp, b);
			break;
		case Pipcp:
			rcv(ppp, ppp->ipcp, b);
			break;
		case Pip:
			if(ppp->ipcp->state == Sopened)
				return b;
			netlog("ppp: IP recved: link not up\n");
			freeb(b);
			break;
		case Plqm:
			getlqm(ppp, b);
			break;
		case Pchap:
			getchap(ppp, b);
			break;
		case Ppasswd:
			getpap(ppp, b);
			break;
		case Pvjctcp:
		case Pvjutcp:
			if(ppp->ipcp->state != Sopened){
				netlog("ppp: VJ tcp recved: link not up\n");
				freeb(b);
				break;
			}
			ppp->stat.vjin++;
			b = tcpuncompress(ppp->ctcp, b, proto);
			if(b != nil)
				return b;
			ppp->stat.vjfail++;
			break;
		case Pcdata:
			ppp->stat.uncomp++;
			if(ppp->ccp->state != Sopened){
				netlog("ppp: compressed data recved: link not up\n");
				freeb(b);
				break;
			}
			if(ppp->unctype == nil) {
				netlog("ppp: compressed data recved: no compression\n");
				freeb(b);
				break;
			}
			len = BLEN(b);
			b = (*ppp->unctype->uncompress)(ppp, b, &proto, &reply);
			if(reply != nil){
				/* send resetreq */
				ppp->stat.uncompreset++;
				putframe(ppp, Pccp, reply);
				freeb(reply);
			}
			if(b == nil)
				break;
			ppp->stat.uncompin += len;
			ppp->stat.uncompout += BLEN(b);
/* netlog("ppp: uncompressed frame %ux %d %d (%d uchars)\n", proto, b->rptr[0], b->rptr[1], BLEN(b)); /* */
			goto Again;	
		default:
			syslog(0, LOG, "unknown proto %ux", proto);
			if(ppp->lcp->state == Sopened){
				/* reject the protocol */
				b->rptr -= 6;
				m = (Lcpmsg*)b->rptr;
				m->code = Lprotorej;
				m->id = ++ppp->lcp->id;
				hnputs(m->data, proto);
				hnputs(m->len, BLEN(b));
				putframe(ppp, Plcp, b);
			}
			freeb(b);
			break;
		}
	}
	return nil;
}

/* transmit an IP packet */
int
pppwrite(PPP *ppp, Block *b)
{
	int proto;
	int len;

	qlock(ppp);
	/* can't send ip packets till we're established */
	if(ppp->ipcp->state != Sopened) {
		qunlock(ppp);
		syslog(0, LOG, "IP write: link not up");
		len = blen(b);
		freeb(b);
		return len;
	}

	proto = Pip;
	ppp->stat.ipsend++;

	if(ppp->ipcp->flags & Fipcompress){
		b = compress(ppp->ctcp, b, &proto);
		if(b == nil){
			qunlock(ppp);
			return 0;
		}
		if(proto != Pip)
			ppp->stat.vjout++;
	}

	if(ppp->ctype != nil) {
		len = blen(b);
		b = (*ppp->ctype->compress)(ppp, proto, b, &proto);
		if(proto == Pcdata) {
			ppp->stat.comp++;
			ppp->stat.compin += len;
			ppp->stat.compout += blen(b);
		}
	} 

	if(putframe(ppp, proto, b) < 0) {
		qunlock(ppp);
		freeb(b);
		return -1;
	}
	qunlock(ppp);

	len = blen(b);
	freeb(b);
	return len;
}

static void
terminate(PPP *ppp, int kill)
{
	close(ppp->ipfd);
	ppp->ipfd = -1;
	close(ppp->ipcfd);
	ppp->ipcfd = -1;
	close(ppp->mediain);
	close(ppp->mediaout);
	ppp->mediain = -1;
	ppp->mediaout = -1;
	dying = 1;

	if(kill)
		postnote(PNGROUP, getpid(), "die");
}

typedef struct Iphdr Iphdr;
struct Iphdr
{
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */
	uchar	ttl;		/* Time to live */
	uchar	proto;		/* Protocol */
	uchar	cksum[2];	/* Header checksum */
	uchar	src[4];		/* Ip source (uchar ordering unimportant) */
	uchar	dst[4];		/* Ip destination (uchar ordering unimportant) */
};

static void
ipinproc(PPP *ppp)
{
	Block *b;
	int m, n;
	Iphdr *ip;

	while(!dying){

		b = allocb(Buflen);
		n = read(ppp->ipfd, b->wptr, b->lim-b->wptr);
		if(n < 0)
			break;

		/* trim packet if there's padding (e.g. from ether) */
		ip = (Iphdr*)b->rptr;
		m = nhgets(ip->length);
		if(m < n && m > 0)
			n = m;
		b->wptr += n;

		if(pppwrite(ppp, b) < 0)
			break;
	}
}

static void
catchdie(void*, char *msg)
{
	if(strstr(msg, "die") != nil)
		noted(NCONT);
	else
		noted(NDFLT);
}

static void
hexdump(uchar *a, int na)
{
	int i;
	char buf[80];

	fprint(2, "dump %p %d\n", a, na);
	buf[0] = '\0';
	for(i=0; i<na; i++){
		sprint(buf+strlen(buf), " %.2ux", a[i]);
		if(i%16 == 7)
			sprint(buf+strlen(buf), " --");
		if(i%16==15){
			sprint(buf+strlen(buf), "\n");
			write(2, buf, strlen(buf));
			buf[0] = '\0';
		}
	}
	if(i%16){
		sprint(buf+strlen(buf), "\n");
		write(2, buf, strlen(buf));
	}
}

static void
mediainproc(PPP *ppp)
{
	Block *b;
	Ipaddr remote;

	notify(catchdie);
	while(!dying){
		b = pppread(ppp);
		if(b == nil){
			syslog(0, LOG, "pppread return nil");
			break;
		}
		ppp->stat.iprecv++;
		if(ppp->ipcp->state != Sopened) {
			ppp->stat.iprecvnotup++;
			freeb(b);
			continue;
		}

		if(server) {
			v4tov6(remote, b->rptr+12);
			if(ipcmp(remote, ppp->remote) != 0) {
				ppp->stat.iprecvbadsrc++;
				freeb(b);
				continue;
			}
		}
		if(debug > 1){
			netlog("ip write pkt %p %d\n", b->rptr, blen(b));
			hexdump(b->rptr, blen(b));
		}
		if(write(ppp->ipfd, b->rptr, blen(b)) < 0) {
			syslog(0, LOG, "error writing to pktifc");
			freeb(b);
			break;
		}

		freeb(b);
	}

	netlog(": remote=%I: ppp shutting down\n", ppp->remote);
	syslog(0, LOG, ": remote=%I: ppp shutting down", ppp->remote);
	syslog(0, LOG, "\t\tppp send = %lud/%lud recv= %lud/%lud",
		ppp->out.packets, ppp->out.uchars,
		ppp->in.packets, ppp->in.uchars);
	syslog(0, LOG, "\t\tip send=%lud", ppp->stat.ipsend);
	syslog(0, LOG, "\t\tip recv=%lud notup=%lud badsrc=%lud",
		ppp->stat.iprecv, ppp->stat.iprecvnotup, ppp->stat.iprecvbadsrc);
	syslog(0, LOG, "\t\tcompress=%lud in=%lud out=%lud reset=%lud",
		ppp->stat.comp, ppp->stat.compin, ppp->stat.compout, ppp->stat.compreset);
	syslog(0, LOG, "\t\tuncompress=%lud in=%lud out=%lud reset=%lud",
		ppp->stat.uncomp, ppp->stat.uncompin, ppp->stat.uncompout,
		ppp->stat.uncompreset);
	syslog(0, LOG, "\t\tvjin=%lud vjout=%lud vjfail=%lud", 
		ppp->stat.vjin, ppp->stat.vjout, ppp->stat.vjfail);
}

/*
 *  link quality management
 */
static void
getlqm(PPP *ppp, Block *b)
{
	Qualpkt *p;

	p = (Qualpkt*)b->rptr;
	if(BLEN(b) == sizeof(Qualpkt)){
		ppp->in.reports++;
		ppp->pout.reports = nhgetl(p->peeroutreports);
		ppp->pout.packets = nhgetl(p->peeroutpackets);
		ppp->pout.uchars = nhgetl(p->peeroutuchars);
		ppp->pin.reports = nhgetl(p->peerinreports);
		ppp->pin.packets = nhgetl(p->peerinpackets);
		ppp->pin.discards = nhgetl(p->peerindiscards);
		ppp->pin.errors = nhgetl(p->peerinerrors);
		ppp->pin.uchars = nhgetl(p->peerinuchars);

		/* save our numbers at time of reception */
		memmove(&ppp->sin, &ppp->in, sizeof(Qualstats));

	}
	freeb(b);
	if(ppp->period == 0)
		putlqm(ppp);

}

static void
putlqm(PPP *ppp)
{
	Qualpkt *p;
	Block *b;

	b = allocb(sizeof(Qualpkt));
	b->wptr += sizeof(Qualpkt);
	p = (Qualpkt*)b->rptr;
	hnputl(p->magic, 0);

	/* heresay (what he last told us) */
	hnputl(p->lastoutreports, ppp->pout.reports);
	hnputl(p->lastoutpackets, ppp->pout.packets);
	hnputl(p->lastoutuchars, ppp->pout.uchars);

	/* our numbers at time of last reception */
	hnputl(p->peerinreports, ppp->sin.reports);
	hnputl(p->peerinpackets, ppp->sin.packets);
	hnputl(p->peerindiscards, ppp->sin.discards);
	hnputl(p->peerinerrors, ppp->sin.errors);
	hnputl(p->peerinuchars, ppp->sin.uchars);

	/* our numbers now */
	hnputl(p->peeroutreports, ppp->out.reports+1);
	hnputl(p->peeroutpackets, ppp->out.packets+1);
	hnputl(p->peeroutuchars, ppp->out.uchars+53/*hack*/);

	putframe(ppp, Plqm, b);
	freeb(b);
	ppp->out.reports++;
}

/*
 * init challenge response dialog
 */
static void
chapinit(PPP *ppp)
{
	Block *b;
	Lcpmsg *m;
	Chap *c;
	int len;
	char *aproto;

	getauth(ppp);

	c = ppp->chap;
	c->id++;

	switch(c->proto){
	default:
		abort();
	case APmd5:
		aproto = "chap";
		break;
	case APmschap:
		aproto = "mschap";
		break;
	}
	if((c->cs = auth_challenge("proto=%q role=server", aproto)) == nil)
		sysfatal("auth_challenge: %r");
	syslog(0, LOG, ": remote=%I: sending %d byte challenge", ppp->remote, c->cs->nchal);
	len = 4 + 1 + c->cs->nchal + strlen(ppp->chapname);
	b = alloclcp(Cchallenge, c->id, len, &m);

	*b->wptr++ = c->cs->nchal;
	memmove(b->wptr, c->cs->chal, c->cs->nchal);
	b->wptr += c->cs->nchal;
	memmove(b->wptr, ppp->chapname, strlen(ppp->chapname));
	b->wptr += strlen(ppp->chapname);
	hnputs(m->len, len);
	putframe(ppp, Pchap, b);
	freeb(b);

	c->state = Cchalsent;
}

/*
 * BUG factotum should do this
 */
enum {
	MShashlen = 16,
	MSresplen = 24,
	MSchallen = 8,
};

void
desencrypt(uchar data[8], uchar key[7])
{
	ulong ekey[32];

	key_setup(key, ekey);
	block_cipher(ekey, data, 0);
}

void
nthash(uchar hash[MShashlen], char *passwd)
{
	uchar buf[512];
	int i;
	
	for(i=0; *passwd && i<sizeof(buf); passwd++) {
		buf[i++] = *passwd;
		buf[i++] = 0;
	}
	memset(hash, 0, 16);
	md4(buf, i, hash, 0);
}

void
mschalresp(uchar resp[MSresplen], uchar hash[MShashlen], uchar chal[MSchallen])
{
	int i;
	uchar buf[21];
	
	memset(buf, 0, sizeof(buf));
	memcpy(buf, hash, MShashlen);

	for(i=0; i<3; i++) {
		memmove(resp+i*MSchallen, chal, MSchallen);
		desencrypt(resp+i*MSchallen, buf+i*7);
	}
}

/*
 *  challenge response dialog
 */
extern	int	_asrdresp(int, uchar*, int);

static void
getchap(PPP *ppp, Block *b)
{
	AuthInfo *ai;
	Lcpmsg *m;
	int len, vlen, i, id, n, nresp;
	char md5buf[512], code;
	Chap *c;
	Chapreply cr;
	MSchapreply mscr;
	char uid[PATH];
	uchar digest[16], *p, *resp, sdigest[SHA1dlen];
	uchar mshash[MShashlen], mshash2[MShashlen];
	DigestState *s;
	uchar msresp[2*MSresplen+1];

	m = (Lcpmsg*)b->rptr;
	len = nhgets(m->len);
	if(BLEN(b) < len){
		syslog(0, LOG, "short chap message");
		freeb(b);
		return;
	}

	qlock(ppp);

	switch(m->code){
	case Cchallenge:
		getauth(ppp);

		vlen = m->data[0];
		if(vlen > len - 5) {
			netlog("PPP: chap: bad challenge len\n");
			break;
		}

		id = m->id;
		switch(ppp->chap->proto){
		default:
			abort();
		case APmd5:
			md5buf[0] = m->id;
			strcpy(md5buf+1, ppp->secret);
			n = strlen(ppp->secret) + 1;
			memmove(md5buf+n, m->data+1, vlen);
			n += vlen;
			md5((uchar*)md5buf, n, digest, nil);
			resp = digest;
			nresp = 16;
			break;
		case APmschap:
			nthash(mshash, ppp->secret);
			memset(msresp, 0, sizeof msresp);
			mschalresp(msresp+MSresplen, mshash, m->data+1);
			resp = msresp;
			nresp = sizeof msresp;
			nthash(mshash, ppp->secret);
			md4(mshash, 16, mshash2, 0);
			s = sha1(mshash2, 16, 0, 0);
			sha1(mshash2, 16, 0, s);
			sha1(m->data+1, 8, sdigest, s);
			memmove(ppp->key, sdigest, 16);
			break;
		}
		len = 4 + 1 + nresp + strlen(ppp->chapname);
		freeb(b);
		b = alloclcp(Cresponse, id, len, &m);
		*b->wptr++ = nresp;
		memmove(b->wptr, resp, nresp);
		b->wptr += nresp;
		memmove(b->wptr, ppp->chapname, strlen(ppp->chapname));
		b->wptr += strlen(ppp->chapname);
		hnputs(m->len, len);
		netlog("PPP: sending response len %d\n", len);
		putframe(ppp, Pchap, b);
		break;
	case Cresponse:
		c = ppp->chap;
		vlen = m->data[0];
		if(m->id != c->id) {
			netlog("PPP: chap: bad response id\n");
			break;
		}
		switch(c->proto) {
		default:
			sysfatal("unknown chap protocol: %d", c->proto);
		case APmd5:
			if(vlen > len - 5 || vlen != 16) {
				netlog("PPP: chap: bad response len\n");
				break;
			}

			cr.id = m->id;
			memmove(cr.resp, m->data+1, 16);
			memset(uid, 0, sizeof(uid));
			n = len-5-vlen;
			if(n >= PATH)
				n = PATH-1;
			memmove(uid, m->data+1+vlen, n);
			c->cs->user = uid;
			c->cs->resp = &cr;
			c->cs->nresp = sizeof cr;
			break;
		case APmschap:
			if(vlen > len - 5 || vlen != 49) {
				netlog("PPP: chap: bad response len\n");
				break;
			}
			memset(&mscr, 0, sizeof(mscr));
			memmove(mscr.LMresp, m->data+1, 24);
			memmove(mscr.NTresp, m->data+24+1, 24);
			n = len-5-vlen;
			p = m->data+1+vlen;
			/* remove domain name */
			for(i=0; i<n; i++) {
				if(p[i] == '\\') {
					p += i+1;
					n -= i+1;
					break;
				}
			}
			if(n >= PATH)
				n = PATH-1;
			memset(uid, 0, sizeof(uid));
			memmove(uid, p, n);
			c->cs->user = uid;
			c->cs->resp = &mscr;
			c->cs->nresp = sizeof mscr;
			break;
		} 

		syslog(0, LOG, ": remote=%I vlen %d proto %d response user %s nresp %d", ppp->remote, vlen, c->proto, c->cs->user, c->cs->nresp);
		if((ai = auth_response(c->cs)) == nil || auth_chuid(ai, nil) < 0){
			c->state = Cunauth;
			code = Cfailure;
			syslog(0, LOG, ": remote=%I: auth failed: %r, uid=%s", ppp->remote, uid);
		}else{
			c->state = Cauthok;
			code = Csuccess;
			syslog(0, LOG, ": remote=%I: auth ok: uid=%s nsecret=%d", ppp->remote, uid, ai->nsecret);
			if(c->proto == APmschap){
				if(ai->nsecret != sizeof(ppp->key))
					sysfatal("could not get the encryption key");
				memmove(ppp->key, ai->secret, sizeof(ppp->key));
			}
		}
		auth_freeAI(ai);
		auth_freechal(c->cs);
		c->cs = nil;
		freeb(b);

		/* send reply */
		len = 4;
		b = alloclcp(code, c->id, len, &m);
		hnputs(m->len, len);
		putframe(ppp, Pchap, b);

		if(c->state == Cauthok) {
			setphase(ppp, Pnet);
		} else {
			/* restart chapp negotiation */
			chapinit(ppp);
		}
		
		break;
	case Csuccess:
		netlog("ppp: chap succeeded\n");
		break;
	case Cfailure:
		netlog("ppp: chap failed\n");
		break;
	default:
		syslog(0, LOG, "chap code %d?\n", m->code);
		break;
	}
	qunlock(ppp);
	freeb(b);
}

static void
putpaprequest(PPP *ppp)
{
	Block *b;
	Lcpmsg *m;
	Chap *c;
	int len, nlen, slen;

	getauth(ppp);

	c = ppp->chap;
	c->id++;
	netlog("PPP: pap: send authreq %d %s %s\n", c->id, ppp->chapname, "****");

	nlen = strlen(ppp->chapname);
	slen = strlen(ppp->secret);
	len = 4 + 1 + nlen + 1 + slen;
	b = alloclcp(Pauthreq, c->id, len, &m);

	*b->wptr++ = nlen;
	memmove(b->wptr, ppp->chapname, nlen);
	b->wptr += nlen;
	*b->wptr++ = slen;
	memmove(b->wptr, ppp->secret, slen);
	b->wptr += slen;
	hnputs(m->len, len);

	putframe(ppp, Ppasswd, b);
	freeb(b);
}

static void
papinit(PPP *ppp)
{
	ppp->chap->id = 0;
	putpaprequest(ppp);
}

static void
getpap(PPP *ppp, Block *b)
{
	Lcpmsg *m;
	int len;

	m = (Lcpmsg*)b->rptr;
	len = 4;
	if(BLEN(b) < 4 || BLEN(b) < (len = nhgets(m->len))){
		syslog(0, LOG, "short pap message (%ld < %d)", BLEN(b), len);
		freeb(b);
		return;
	}
	if(len < sizeof(Lcpmsg))
		m->data[0] = 0;

	qlock(ppp);
	switch(m->code){
	case Pauthreq:
		netlog("PPP: pap auth request, not supported\n");
		break;
	case Pauthack:
		if(ppp->phase == Pauth
		&& ppp->chap->proto == APpasswd
		&& m->id <= ppp-> chap->id){
			netlog("PPP: pap succeeded\n");
			setphase(ppp, Pnet);
		}
		break;
	case Pauthnak:
		if(ppp->phase == Pauth
		&& ppp->chap->proto == APpasswd
		&& m->id <= ppp-> chap->id){
			netlog("PPP: pap failed (%d:%.*s)\n",
				m->data[0], m->data[0], (char*)m->data+1);
			terminate(ppp, 0);
		}
		break;
	default:
		netlog("PPP: unknown pap messsage %d\n", m->code);
	}
	qunlock(ppp);
	freeb(b);
}

static void
printopts(Pstate *p, Block *b, int send)
{
	Lcpmsg *m;	
	Lcpopt *o;
	int proto, x, period;
	uchar *cp;
	char *code, *dir;

	m = (Lcpmsg*)b->rptr;
	switch(m->code) {
	default: code = "<unknown>"; break;
	case Lconfreq: code = "confrequest"; break;
	case Lconfack: code = "confack"; break;
	case Lconfnak: code = "confnak"; break;
	case Lconfrej: code = "confreject"; break;
	}

	if(send)
		dir = "send";
	else
		dir = "recv";

	netlog("ppp: %s %s: id=%d\n", dir, code, m->id);

	for(cp = m->data; cp < b->wptr; cp += o->len){
		o = (Lcpopt*)cp;
		if(cp + o->len > b->wptr){
			netlog("\tbad option length %ux\n", o->type);
			return;
		}

		switch(p->proto){
		case Plcp:
			switch(o->type){
			default:
				netlog("\tunknown %d len=%d\n", o->type, o->len);
				break;
			case Omtu:
				netlog("\tmtu = %d\n", nhgets(o->data));
				break;
			case Octlmap:
				netlog("\tctlmap = %ux\n", nhgetl(o->data));
				break;
			case Oauth:
				netlog("\tauth = %ux", nhgetl(o->data));
				proto = nhgets(o->data);
				switch(proto) {
				default:
					netlog("unknown auth proto %d\n", proto);
					break;
				case Ppasswd:
					netlog("password\n");
					break;
				case Pchap:
					netlog("chap %ux\n", o->data[2]);
					break;
				}
				break;
			case Oquality:
				proto = nhgets(o->data);
				switch(proto) {
				default:
					netlog("\tunknown quality proto %d\n", proto);
					break;
				case Plqm:
					x = nhgetl(o->data+2)*10;
					period = (x+Period-1)/Period;
					netlog("\tlqm period = %d\n", period);
					break;
				}
			case Omagic:
				netlog("\tmagic = %ux\n", nhgetl(o->data));
				break;
			case Opc:
				netlog("\tprotocol compress\n");
				break;
			case Oac:
				netlog("\taddr compress\n");
				break;
			}
			break;
		case Pccp:
			switch(o->type){
			default:
				netlog("\tunknown %d len=%d\n", o->type, o->len);
				break;
			case Ocoui:	
				netlog("\tOUI\n");
				break;
			case Ocstac:
				netlog("\tstac LZS\n");
				break;
			case Ocmppc:	
				netlog("\tMicrosoft PPC len=%d %ux\n", o->len, nhgetl(o->data));
				break;
			case Octhwack:	
				netlog("\tThwack\n");
				break;
			}
			break;
		case Pecp:
			switch(o->type){
			default:
				netlog("\tunknown %d len=%d\n", o->type, o->len);
				break;
			case Oeoui:	
				netlog("\tOUI\n");
				break;
			case Oedese:
				netlog("\tDES\n");
				break;
			}
			break;
		case Pipcp:
			switch(o->type){
			default:
				netlog("\tunknown %d len=%d\n", o->type, o->len);
				break;
			case Oipaddrs:	
				netlog("\tip addrs - deprecated\n");
				break;
			case Oipcompress:
				netlog("\tip compress\n");
				break;
			case Oipaddr:	
				netlog("\tip addr %V\n", o->data);
				break;
			case Oipdns:	
				netlog("\tdns addr %V\n", o->data);
				break;
			case Oipwins:	
				netlog("\twins addr %V\n", o->data);
				break;
			case Oipdns2:	
				netlog("\tdns2 addr %V\n", o->data);
				break;
			case Oipwins2:	
				netlog("\twins2 addr %V\n", o->data);
				break;
			}
			break;
		}
	}
}

static void
sendtermreq(PPP *ppp, Pstate *p)
{
	Block *b;
	Lcpmsg *m;

	p->termid = ++(p->id);
	b = alloclcp(Ltermreq, p->termid, 4, &m);
	hnputs(m->len, 4);
	putframe(ppp, p->proto, b);
	freeb(b);
	newstate(ppp, p, Sclosing);
}

static void
sendechoreq(PPP *ppp, Pstate *p)
{
	Block *b;
	Lcpmsg *m;

	p->termid = ++(p->id);
	b = alloclcp(Lechoreq, p->id, 4, &m);
	hnputs(m->len, 4);
	putframe(ppp, p->proto, b);
	freeb(b);
}

enum
{
	CtrlD	= 0x4,
	CtrlE	= 0x5,
	CtrlO	= 0xf,
	Cr	= 13,
	View	= 0x80,
};

int conndone;

static void
xfer(int fd)
{
	int i, n;
	uchar xbuf[128];

	for(;;) {
		n = read(fd, xbuf, sizeof(xbuf));
		if(n < 0)
			break;
		if(conndone)
			break;
		for(i = 0; i < n; i++)
			if(xbuf[i] == Cr)
				xbuf[i] = ' ';
		write(1, xbuf, n);
	}
	close(fd);
}

static int
readcr(int fd, char *buf, int nbuf)
{
	char c;
	int n, tot;

	tot = 0;
	while((n=read(fd, &c, 1)) == 1){
		if(c == '\n'){
			buf[tot] = 0;
			return tot;
		}
		buf[tot++] = c;
		if(tot == nbuf)
			sysfatal("line too long in readcr");
	}
	return n;
}

static void
connect(int fd, int cfd)
{
	int n, ctl;
	char xbuf[128];

	if (chatfile) {
		int chatfd, lineno, nb;
		char *buf, *p, *s, response[128];
		Dir *dir;

		if ((chatfd = open(chatfile, OREAD)) < 0)
			sysfatal("cannot open %s: %r", chatfile);

		if ((dir = dirfstat(chatfd)) == nil)
			sysfatal("cannot fstat %s: %r",chatfile);

		buf = (char *)malloc(dir->length + 1);
		assert(buf);

		if ((nb = read(chatfd, buf, dir->length)) < 0)
			sysfatal("cannot read chatfile %s: %r", chatfile);
		assert(nb == dir->length);
		buf[dir->length] = '\0';
		free(dir);
		close(chatfd);

		p = buf;
		lineno = 0;
		for(;;) {
			char *_args[3];

			if ((s = strchr(p, '\n')) == nil)
				break;
			*s++ = '\0';
		
			lineno++;

			if (*p == '#') {
				p = s; 
				continue;
			}

			if (tokenize(p, _args, 3) != 2)
				sysfatal("invalid line %d (line expected: 'send' 'expect')", 
						lineno);

			if (debug)
				print("sending %s, expecting %s\n", _args[0], _args[1]);

			if(strlen(_args[0])){
				nb = fprint(fd, "%s\r", _args[0]);
				assert(nb > 0);
			}

			if (strlen(_args[1]) > 0) {
				if ((nb = readcr(fd, response, sizeof response-1)) < 0)
					sysfatal("cannot read response from: %r");

				if (debug)
					print("response %s\n", response);

				if (nb == 0)
					sysfatal("eof on input?");

				if (cistrstr(response, _args[1]) == nil)
					sysfatal("expected %s, got %s", _args[1], response);
			}
			p = s;
		}
		free(buf);
		return;
	}

	print("Connect to file system now, type ctrl-d when done.\n");
	print("...(Use the view or down arrow key to send a break)\n");
	print("...(Use ctrl-e to set even parity or ctrl-o for odd)\n");

	ctl = open("/dev/consctl", OWRITE);
	if(ctl < 0)
		sysfatal("opening consctl");
	fprint(ctl, "rawon");

	fd = dup(fd, -1);
	conndone = 0;
	switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
	case -1:
		sysfatal("forking xfer");
	case 0:
		xfer(fd);
		_exits(nil);
	}

	for(;;){
		read(0, xbuf, 1);
		switch(xbuf[0]&0xff) {
		case CtrlD:	/* done */
			conndone = 1;
			close(ctl);
			print("\n");
			return;
		case CtrlE:	/* set even parity */
			fprint(cfd, "pe");
			break;
		case CtrlO:	/* set odd parity */
			fprint(cfd, "po");
			break;
		case View:	/* send a break */
			fprint(cfd, "k500");
			break;
		default:
			n = write(fd, xbuf, 1);
			if(n < 0) {
				errstr(xbuf, sizeof(xbuf));
				conndone = 1;
				close(ctl);
				print("[remote write error (%s)]\n", xbuf);
				return;
			}
		}
	}
}

int interactive;

void
usage(void)
{
	fprint(2, "usage: ppp [-CPSacdfu] [-b baud] [-k keyspec] [-m mtu] "
		"[-M chatfile] [-p dev] [-x netmntpt] [-t modemcmd] "
		"[local-addr [remote-addr]]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int mtu, baud, framing, user, mediain, mediaout, cfd;
	Ipaddr ipaddr, remip;
	char *dev, *modemcmd;
	char net[128];
	PPP *ppp;
	char buf[128];

	rfork(RFREND|RFNOTEG|RFNAMEG);

	fmtinstall('I', eipfmt);
	fmtinstall('V', eipfmt);
	fmtinstall('E', eipfmt);

	dev = nil;

	invalidate(ipaddr);
	invalidate(remip);

	mtu = Defmtu;
	baud = 0;
	framing = 0;
	setnetmtpt(net, sizeof(net), nil);
	user = 0;
	modemcmd = nil;

	ARGBEGIN{
	case 'a':
		noauth = 1;
		break;
	case 'b':
		baud = atoi(EARGF(usage()));
		if(baud < 0)
			baud = 0;
		break;
	case 'c':
		nocompress = 1;
		break;
	case 'C':
		noipcompress = 1;
		break;
	case 'd':
		debug++;
		break;
	case 'f':
		framing = 1;
		break;
	case 'F':
		pppframing = 0;
		break;
	case 'k':
		keyspec = EARGF(usage());
		break;
	case 'm':
		mtu = atoi(EARGF(usage()));
		if(mtu < Minmtu)
			mtu = Minmtu;
		if(mtu > Maxmtu)
			mtu = Maxmtu;
		break;
	case 'M':
		chatfile = EARGF(usage());
		break;
	case 'p':
		dev = EARGF(usage());
		break;
	case 'P':
		primary = 1;
		break;
	case 'S':
		server = 1;
		break;
	case 't':
		modemcmd = EARGF(usage());
		break;
	case 'u':
		user = 1;
		break;
	case 'x':
		setnetmtpt(net, sizeof net, EARGF(usage()));
		break;
	default:
		fprint(2, "unknown option %c\n", ARGC());
		usage();
	}ARGEND;

	switch(argc){
	case 2:
		if (parseip(remip, argv[1]) == -1)
			sysfatal("bad remote ip %s", argv[1]);
	case 1:
		if (parseip(ipaddr, argv[0]) == -1)
			sysfatal("bad ip %s", argv[0]);
	case 0:
		break;
	default:
		usage();
	}

	nip = nipifcs(net);
	if(nip == 0 && !server)
		primary = 1;

	if(dev != nil){
		mediain = open(dev, ORDWR);
		if(mediain < 0){
			if(strchr(dev, '!')){
				if((mediain = dial(dev, 0, 0, &cfd)) == -1){
					fprint(2, "ppp: couldn't dial %s: %r\n", dev);
					exits(dev);
				}
			} else {
				fprint(2, "ppp: couldn't open %s\n", dev);
				exits(dev);
			}
		} else {
			snprint(buf, sizeof buf, "%sctl", dev);
			cfd = open(buf, ORDWR);
		}
		if(cfd > 0){
			if(baud)
				fprint(cfd, "b%d", baud);
			fprint(cfd, "m1");	/* cts/rts flow control (and fifo's) on */
			fprint(cfd, "q64000");	/* increase q size to 64k */
			fprint(cfd, "n1");	/* nonblocking writes on */
			fprint(cfd, "r1");	/* rts on */
			fprint(cfd, "d1");	/* dtr on */
			fprint(cfd, "c1");	/* dcdhup on */
			if(user || chatfile)
				connect(mediain, cfd);
			close(cfd);
		} else {
			if(user || chatfile)
				connect(mediain, -1);
		}
		mediaout = mediain;
	} else {
		mediain = open("/fd/0", OREAD);
		if(mediain < 0){
			fprint(2, "ppp: couldn't open /fd/0\n");
			exits("/fd/0");
		}
		mediaout = open("/fd/1", OWRITE);
		if(mediaout < 0){
			fprint(2, "ppp: couldn't open /fd/0\n");
			exits("/fd/1");
		}
	}

	if(modemcmd != nil && mediaout >= 0)
		fprint(mediaout, "%s\r", modemcmd);

	ppp = mallocz(sizeof(*ppp), 1);
	pppopen(ppp, mediain, mediaout, net, ipaddr, remip, mtu, framing);

	/* wait until ip is configured */
	rendezvous((void*)Rmagic, 0);

	if(primary){
		/* create a /net/ndb entry */
		putndb(ppp, net);
	}

	exits(0);
}

void
netlog(char *fmt, ...)
{
	va_list arg;
	char *m;
	static long start;
	long now;

	if(debug == 0)
		return;

	now = time(0);
	if(start == 0)
		start = now;

	va_start(arg, fmt);
	m = vsmprint(fmt, arg);
	fprint(2, "%ld %s", now-start, m);
	free(m);
	va_end(arg);
}

/*
 *  return non-zero if this is a valid v4 address
 */
static int
validv4(Ipaddr addr)
{
	return memcmp(addr, v4prefix, IPv4off) == 0 && memcmp(addr, v4prefix, IPaddrlen) != 0;
}

static void
invalidate(Ipaddr addr)
{
	ipmove(addr, IPnoaddr);
}

/*
 *  return number of networks
 */
static int
nipifcs(char *net)
{
	static Ipifc *ifc;
	Ipifc *nifc;
	Iplifc *lifc;
	int n;

	n = 0;
	ifc = readipifc(net, ifc, -1);
	for(nifc = ifc; nifc != nil; nifc = nifc->next)
		for(lifc = ifc->lifc; lifc != nil; lifc = lifc->next)
			n++;
	return n;
}

/*
 *   make an ndb entry and put it into /net/ndb for the servers to see
 */
static void
putndb(PPP *ppp, char *net)
{
	char buf[1024];
	char file[64];
	char *p, *e;
	int fd;

	e = buf + sizeof(buf);
	p = buf;
	p = seprint(p, e, "ip=%I ipmask=255.255.255.255 ipgw=%I\n", ppp->local,
			ppp->remote);
	if(validv4(ppp->dns[0]))
		p = seprint(p, e, "\tdns=%I\n", ppp->dns[0]);
	if(validv4(ppp->dns[1]))
		p = seprint(p, e, "\tdns=%I\n", ppp->dns[1]);
	if(validv4(ppp->wins[0]))
		p = seprint(p, e, "\twins=%I\n", ppp->wins[0]);
	if(validv4(ppp->wins[1]))
		p = seprint(p, e, "\twins=%I\n", ppp->wins[1]);
	seprint(file, file+sizeof file, "%s/ndb", net);
	fd = open(file, OWRITE);
	if(fd < 0)
		return;
	write(fd, buf, p-buf);
	close(fd);
	seprint(file, file+sizeof file, "%s/cs", net);
	fd = open(file, OWRITE);
	write(fd, "refresh", 7);
	close(fd);
	seprint(file, file+sizeof file, "%s/dns", net);
	fd = open(file, OWRITE);
	write(fd, "refresh", 7);
	close(fd);
}

static void
getauth(PPP *ppp)
{
	UserPasswd *up;

	if(*ppp->chapname)
		return;

	up = auth_getuserpasswd(auth_getkey,"proto=pass service=ppp %s", keyspec);
	if(up != nil){
		strcpy(ppp->chapname, up->user);
		strcpy(ppp->secret, up->passwd);
	}		
}
