/*
 * USB host driver for BCM2835
 *	Synopsis DesignWare Core USB 2.0 OTG controller
 *
 * Copyright © 2012 Richard Miller <r.miller@acm.org>
 *
 * This is work in progress:
 * - no isochronous pipes
 * - no bandwidth budgeting
 * - frame scheduling is crude
 * - error handling is overly optimistic
 * It should be just about adequate for a Plan 9 terminal with
 * keyboard, mouse, ethernet adapter, and an external flash drive.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include	"../port/usb.h"

#include "dwcotg.h"

enum
{
	USBREGS		= VIRTIO + 0x980000,
	Enabledelay	= 50,
	Resetdelay	= 10,
	ResetdelayHS	= 50,

	Read		= 0,
	Write		= 1,
};

typedef struct Ctlr Ctlr;
typedef struct Epio Epio;

struct Ctlr {
	Dwcregs	*regs;		/* controller registers */
	int	nchan;		/* number of host channels */
	ulong	chanbusy;	/* bitmap of in-use channels */
	QLock	chanlock;	/* serialise access to chanbusy */
	QLock	split;		/* serialise split transactions */
	int	splitretry;	/* count retries of Nyet */
	int	sofchan;	/* bitmap of channels waiting for sof */
	int	wakechan;	/* bitmap of channels to wakeup after fiq */
	int	debugchan;	/* bitmap of channels for interrupt debug */
	Rendez	*chanintr;	/* sleep till interrupt on channel N */
};

struct Epio {
	QLock;
	Block	*cb;
	ulong	lastpoll;
};

static Ctlr dwc;
static int debug;

static char Ebadlen[] = "bad usb request length";

static void clog(Ep *ep, Hostchan *hc);
static void logdump(Ep *ep);

static Hostchan*
chanalloc(Ep *ep)
{
	Ctlr *ctlr;
	int bitmap, i;

	ctlr = ep->hp->aux;
	qlock(&ctlr->chanlock);
	bitmap = ctlr->chanbusy;
	for(i = 0; i < ctlr->nchan; i++)
		if((bitmap & (1<<i)) == 0){
			ctlr->chanbusy = bitmap | 1<<i;
			qunlock(&ctlr->chanlock);
			return &ctlr->regs->hchan[i];
		}
	qunlock(&ctlr->chanlock);
	panic("miller is a lazy git");
	return nil;
}

static void
chanrelease(Ep *ep, Hostchan *chan)
{
	Ctlr *ctlr;
	int i;

	ctlr = ep->hp->aux;
	i = chan - ctlr->regs->hchan;
	qlock(&ctlr->chanlock);
	ctlr->chanbusy &= ~(1<<i);
	qunlock(&ctlr->chanlock);
}

static void
chansetup(Hostchan *hc, Ep *ep)
{
	int hcc;
	Ctlr *ctlr = ep->hp->aux;

	if(ep->debug)
		ctlr->debugchan |= 1 << (hc - ctlr->regs->hchan);
	else
		ctlr->debugchan &= ~(1 << (hc - ctlr->regs->hchan));
	switch(ep->dev->state){
	case Dconfig:
	case Dreset:
		hcc = 0;
		break;
	default:
		hcc = ep->dev->nb<<ODevaddr;
		break;
	}
	hcc |= ep->maxpkt | 1<<OMulticnt | ep->nb<<OEpnum;
	switch(ep->ttype){
	case Tctl:
		hcc |= Epctl;
		break;
	case Tiso:
		hcc |= Episo;
		break;
	case Tbulk:
		hcc |= Epbulk;
		break;
	case Tintr:
		hcc |= Epintr;
		break;
	}
	switch(ep->dev->speed){
	case Lowspeed:
		hcc |= Lspddev;
		/* fall through */
	case Fullspeed:
		if(ep->dev->hub > 1){
			hc->hcsplt = Spltena | POS_ALL | ep->dev->hub<<OHubaddr |
				ep->dev->port;
			break;
		}
		/* fall through */
	default:
		hc->hcsplt = 0;
		break;
	}
	hc->hcchar = hcc;
	hc->hcint = ~0;
}

static int
sofdone(void *a)
{
	Dwcregs *r;

	r = a;
	return r->gintsts & Sofintr;
}

static void
sofwait(Ctlr *ctlr, int n)
{
	Dwcregs *r;
	int x;

	r = ctlr->regs;
	do{
		r->gintsts = Sofintr;
		x = splfhi();
		ctlr->sofchan |= 1<<n;
		r->gintmsk |= Sofintr;
		sleep(&ctlr->chanintr[n], sofdone, r);
		splx(x);
	}while((r->hfnum & 7) == 6);
}

static int
chandone(void *a)
{
	Hostchan *hc;

	hc = a;
	if(hc->hcint == (Chhltd|Ack))
		return 0;
	return (hc->hcint & hc->hcintmsk) != 0;
}

static int
chanwait(Ep *ep, Ctlr *ctlr, Hostchan *hc, int mask)
{
	int intr, n, x, ointr;
	ulong start, now;
	Dwcregs *r;

	r = ctlr->regs;
	n = hc - r->hchan;
	for(;;){
restart:
		x = splfhi();
		r->haintmsk |= 1<<n;
		hc->hcintmsk = mask;
		sleep(&ctlr->chanintr[n], chandone, hc);
		hc->hcintmsk = 0;
		splx(x);
		intr = hc->hcint;
		if(intr & Chhltd)
			return intr;
		start = fastticks(0);
		ointr = intr;
		now = start;
		do{
			intr = hc->hcint;
			if(intr & Chhltd){
				if((ointr != Ack && ointr != (Ack|Xfercomp)) ||
				   intr != (Ack|Chhltd|Xfercomp) ||
				   (now - start) > 60)
					dprint("await %x after %ld %x -> %x\n",
						mask, now - start, ointr, intr);
				return intr;
			}
			if((intr & mask) == 0){
				dprint("ep%d.%d await %x intr %x -> %x\n",
					ep->dev->nb, ep->nb, mask, ointr, intr);
				goto restart;
			}
			now = fastticks(0);
		}while(now - start < 100);
		dprint("ep%d.%d halting channel %8.8ux hcchar %8.8ux "
			"grxstsr %8.8ux gnptxsts %8.8ux hptxsts %8.8ux\n",
			ep->dev->nb, ep->nb, intr, hc->hcchar, r->grxstsr,
			r->gnptxsts, r->hptxsts);
		mask = Chhltd;
		hc->hcchar |= Chdis;
		start = m->ticks;
		while(hc->hcchar & Chen){
			if(m->ticks - start >= 100){
				print("ep%d.%d channel won't halt hcchar %8.8ux\n",
					ep->dev->nb, ep->nb, hc->hcchar);
				break;
			}
		}
		logdump(ep);
	}
}

static int
chanintr(Ctlr *ctlr, int n)
{
	Hostchan *hc;
	int i;

	hc = &ctlr->regs->hchan[n];
	if(ctlr->debugchan & (1<<n))
		clog(nil, hc);
	if((hc->hcsplt & Spltena) == 0)
		return 0;
	i = hc->hcint;
	if(i == (Chhltd|Ack)){
		hc->hcsplt |= Compsplt;
		ctlr->splitretry = 0;
	}else if(i == (Chhltd|Nyet)){
		if(++ctlr->splitretry >= 3)
			return 0;
	}else
		return 0;
	if(hc->hcchar & Chen){
		iprint("hcchar %8.8ux hcint %8.8ux", hc->hcchar, hc->hcint);
		hc->hcchar |= Chen | Chdis;
		while(hc->hcchar&Chen)
			;
		iprint(" %8.8ux\n", hc->hcint);
	}
	hc->hcint = i;
	if(ctlr->regs->hfnum & 1)
		hc->hcchar &= ~Oddfrm;
	else
		hc->hcchar |= Oddfrm;
	hc->hcchar = (hc->hcchar &~ Chdis) | Chen;
	return 1;
}

static Reg chanlog[32][5];
static int nchanlog;

static void
logstart(Ep *ep)
{
	if(ep->debug)
		nchanlog = 0;
}

static void
clog(Ep *ep, Hostchan *hc)
{
	Reg *p;

	if(ep != nil && !ep->debug)
		return;
	if(nchanlog == 32)
		nchanlog--;
	p = chanlog[nchanlog];
	p[0] = dwc.regs->hfnum;
	p[1] = hc->hcchar;
	p[2] = hc->hcint;
	p[3] = hc->hctsiz;
	p[4] = hc->hcdma;
	nchanlog++;
}

static void
logdump(Ep *ep)
{
	Reg *p;
	int i;

	if(!ep->debug)
		return;
	p = chanlog[0];
	for(i = 0; i < nchanlog; i++){
		print("%5.5d.%5.5d %8.8ux %8.8ux %8.8ux %8.8ux\n",
			p[0]&0xFFFF, p[0]>>16, p[1], p[2], p[3], p[4]);
		p += 5;
	}
	nchanlog = 0;
}

static int
chanio(Ep *ep, Hostchan *hc, int dir, int pid, void *a, int len)
{
	Ctlr *ctlr;
	int nleft, n, nt, i, maxpkt, npkt;
	uint hcdma, hctsiz;

	ctlr = ep->hp->aux;
	maxpkt = ep->maxpkt;
	npkt = HOWMANY(len, ep->maxpkt);
	if(npkt == 0)
		npkt = 1;

	hc->hcchar = (hc->hcchar & ~Epdir) | dir;
	if(dir == Epin)
		n = ROUND(len, ep->maxpkt);
	else
		n = len;
	hc->hctsiz = n | npkt<<OPktcnt | pid;
	hc->hcdma  = PADDR(a);

	nleft = len;
	logstart(ep);
	for(;;){
		hcdma = hc->hcdma;
		hctsiz = hc->hctsiz;
		hc->hctsiz = hctsiz & ~Dopng;
		if(hc->hcchar&Chen){
			dprint("ep%d.%d before chanio hcchar=%8.8ux\n",
				ep->dev->nb, ep->nb, hc->hcchar);
			hc->hcchar |= Chen | Chdis;
			while(hc->hcchar&Chen)
				;
			hc->hcint = Chhltd;
		}
		if((i = hc->hcint) != 0){
			dprint("ep%d.%d before chanio hcint=%8.8ux\n",
				ep->dev->nb, ep->nb, i);
			hc->hcint = i;
		}
		if(hc->hcsplt & Spltena){
			qlock(&ctlr->split);
			sofwait(ctlr, hc - ctlr->regs->hchan);
			if((dwc.regs->hfnum & 1) == 0)
				hc->hcchar &= ~Oddfrm;
			else
				hc->hcchar |= Oddfrm;
		}
		hc->hcchar = (hc->hcchar &~ Chdis) | Chen;
		clog(ep, hc);
		if(ep->ttype == Tbulk && dir == Epin)
			i = chanwait(ep, ctlr, hc, /* Ack| */ Chhltd);
		else if(ep->ttype == Tintr && (hc->hcsplt & Spltena))
			i = chanwait(ep, ctlr, hc, Chhltd);
		else
			i = chanwait(ep, ctlr, hc, Chhltd|Nak);
		clog(ep, hc);
		hc->hcint = i;

		if(hc->hcsplt & Spltena){
			hc->hcsplt &= ~Compsplt;
			qunlock(&ctlr->split);
		}

		if((i & Xfercomp) == 0 && i != (Chhltd|Ack) && i != Chhltd){
			if(i & Stall)
				error(Estalled);
			if(i & (Nyet|Frmovrun))
				continue;
			if(i & Nak){
				if(ep->ttype == Tintr)
					tsleep(&up->sleep, return0, 0, ep->pollival);
				else
					tsleep(&up->sleep, return0, 0, 1);
				continue;
			}
			logdump(ep);
			print("usbotg: ep%d.%d error intr %8.8ux\n",
				ep->dev->nb, ep->nb, i);
			if(i & ~(Chhltd|Ack))
				error(Eio);
			if(hc->hcdma != hcdma)
				print("usbotg: weird hcdma %x->%x intr %x->%x\n",
					hcdma, hc->hcdma, i, hc->hcint);
		}
		n = hc->hcdma - hcdma;
		if(n == 0){
			if((hc->hctsiz & Pktcnt) != (hctsiz & Pktcnt))
				break;
			else
				continue;
		}
		if(dir == Epin && ep->ttype == Tbulk && n == nleft){
			nt = (hctsiz & Xfersize) - (hc->hctsiz & Xfersize);
			if(nt != n){
				if(n == ROUND(nt, 4))
					n = nt;
				else
					print("usbotg: intr %8.8ux "
						"dma %8.8ux-%8.8ux "
						"hctsiz %8.8ux-%8.ux\n",
						i, hcdma, hc->hcdma, hctsiz,
						hc->hctsiz);
			}
		}
		if(n > nleft){
			if(n != ROUND(nleft, 4))
				dprint("too much: wanted %d got %d\n",
					len, len - nleft + n);
			n = nleft;
		}
		nleft -= n;
		if(nleft == 0 || (n % maxpkt) != 0)
			break;
		if((i & Xfercomp) && ep->ttype != Tctl)
			break;
		if(dir == Epout)
			dprint("too little: nleft %d hcdma %x->%x hctsiz %x->%x intr %x\n",
				nleft, hcdma, hc->hcdma, hctsiz, hc->hctsiz, i);
	}
	logdump(ep);
	return len - nleft;
}

static long
multitrans(Ep *ep, Hostchan *hc, int rw, void *a, long n)
{
	long sofar, m;

	sofar = 0;
	do{
		m = n - sofar;
		if(m > ep->maxpkt)
			m = ep->maxpkt;
		m = chanio(ep, hc, rw == Read? Epin : Epout, ep->toggle[rw],
			(char*)a + sofar, m);
		ep->toggle[rw] = hc->hctsiz & Pid;
		sofar += m;
	}while(sofar < n && m == ep->maxpkt);
	return sofar;
}

static long
eptrans(Ep *ep, int rw, void *a, long n)
{
	Hostchan *hc;

	if(ep->clrhalt){
		ep->clrhalt = 0;
		if(ep->mode != OREAD)
			ep->toggle[Write] = DATA0;
		if(ep->mode != OWRITE)
			ep->toggle[Read] = DATA0;
	}
	hc = chanalloc(ep);
	if(waserror()){
		ep->toggle[rw] = hc->hctsiz & Pid;
		chanrelease(ep, hc);
		if(strcmp(up->errstr, Estalled) == 0)
			return 0;
		nexterror();
	}
	chansetup(hc, ep);
	if(rw == Read && ep->ttype == Tbulk)
		n = multitrans(ep, hc, rw, a, n);
	else{
		n = chanio(ep, hc, rw == Read? Epin : Epout, ep->toggle[rw],
			a, n);
		ep->toggle[rw] = hc->hctsiz & Pid;
	}
	chanrelease(ep, hc);
	poperror();
	return n;
}

static long
ctltrans(Ep *ep, uchar *req, long n)
{
	Hostchan *hc;
	Epio *epio;
	Block *b;
	uchar *data;
	int datalen;

	epio = ep->aux;
	if(epio->cb != nil){
		freeb(epio->cb);
		epio->cb = nil;
	}
	if(n < Rsetuplen)
		error(Ebadlen);
	if(req[Rtype] & Rd2h){
		datalen = GET2(req+Rcount);
		if(datalen <= 0 || datalen > Maxctllen)
			error(Ebadlen);
		/* XXX cache madness */
		epio->cb = b = allocb(ROUND(datalen, ep->maxpkt) + CACHELINESZ);
		b->wp = (uchar*)ROUND((uintptr)b->wp, CACHELINESZ);
		memset(b->wp, 0x55, b->lim - b->wp);
		cachedwbinvse(b->wp, b->lim - b->wp);
		data = b->wp;
	}else{
		b = nil;
		datalen = n - Rsetuplen;
		data = req + Rsetuplen;
	}
	hc = chanalloc(ep);
	if(waserror()){
		chanrelease(ep, hc);
		if(strcmp(up->errstr, Estalled) == 0)
			return 0;
		nexterror();
	}
	chansetup(hc, ep);
	chanio(ep, hc, Epout, SETUP, req, Rsetuplen);
	if(req[Rtype] & Rd2h){
		if(ep->dev->hub <= 1){
			ep->toggle[Read] = DATA1;
			b->wp += multitrans(ep, hc, Read, data, datalen);
		}else
			b->wp += chanio(ep, hc, Epin, DATA1, data, datalen);
		chanio(ep, hc, Epout, DATA1, nil, 0);
		n = Rsetuplen;
	}else{
		if(datalen > 0)
			chanio(ep, hc, Epout, DATA1, data, datalen);
		chanio(ep, hc, Epin, DATA1, nil, 0);
		n = Rsetuplen + datalen;
	}
	chanrelease(ep, hc);
	poperror();
	return n;
}

static long
ctldata(Ep *ep, void *a, long n)
{
	Epio *epio;
	Block *b;

	epio = ep->aux;
	b = epio->cb;
	if(b == nil)
		return 0;
	if(n > BLEN(b))
		n = BLEN(b);
	memmove(a, b->rp, n);
	b->rp += n;
	if(BLEN(b) == 0){
		freeb(b);
		epio->cb = nil;
	}
	return n;
}

static void
greset(Dwcregs *r, int bits)
{
	r->grstctl |= bits;
	while(r->grstctl & bits)
		;
	microdelay(10);
}

static void
init(Hci *hp)
{
	Ctlr *ctlr;
	Dwcregs *r;
	uint n, rx, tx, ptx;

	ctlr = hp->aux;
	r = ctlr->regs;

	ctlr->nchan = 1 + ((r->ghwcfg2 & Num_host_chan) >> ONum_host_chan);
	ctlr->chanintr = malloc(ctlr->nchan * sizeof(Rendez));

	r->gahbcfg = 0;
	setpower(PowerUsb, 1);

	while((r->grstctl&Ahbidle) == 0)
		;
	greset(r, Csftrst);

	r->gusbcfg |= Force_host_mode;
	tsleep(&up->sleep, return0, 0, 25);
	r->gahbcfg |= Dmaenable;

	n = (r->ghwcfg3 & Dfifo_depth) >> ODfifo_depth;
	rx = 0x306;
	tx = 0x100;
	ptx = 0x200;
	r->grxfsiz = rx;
	r->gnptxfsiz = rx | tx<<ODepth;
	tsleep(&up->sleep, return0, 0, 1);
	r->hptxfsiz = (rx + tx) | ptx << ODepth;
	greset(r, Rxfflsh);
	r->grstctl = TXF_ALL;
	greset(r, Txfflsh);
	dprint("usbotg: FIFO depth %d sizes rx/nptx/ptx %8.8ux %8.8ux %8.8ux\n",
		n, r->grxfsiz, r->gnptxfsiz, r->hptxfsiz);

	r->hport0 = Prtpwr|Prtconndet|Prtenchng|Prtovrcurrchng;
	r->gintsts = ~0;
	r->gintmsk = Hcintr;
	r->gahbcfg |= Glblintrmsk;
}

static void
dump(Hci*)
{
}

static void
fiqintr(Ureg*, void *a)
{
	Hci *hp;
	Ctlr *ctlr;
	Dwcregs *r;
	uint intr, haint, wakechan;
	int i;

	hp = a;
	ctlr = hp->aux;
	r = ctlr->regs;
	wakechan = 0;
	intr = r->gintsts;
	if(intr & Hcintr){
		haint = r->haint & r->haintmsk;
		for(i = 0; haint; i++){
			if(haint & 1){
				if(chanintr(ctlr, i) == 0){
					r->haintmsk &= ~(1<<i);
					wakechan |= 1<<i;
				}
			}
			haint >>= 1;
		}
	}
	if(intr & Sofintr){
		r->gintsts = Sofintr;
		if((r->hfnum&7) != 6){
			r->gintmsk &= ~Sofintr;
			wakechan |= ctlr->sofchan;
			ctlr->sofchan = 0;
		}
	}
	if(wakechan){
		ctlr->wakechan |= wakechan;
		armtimerset(1);
	}
}

static void
irqintr(Ureg*, void *a)
{
	Ctlr *ctlr;
	uint wakechan;
	int i, x;

	ctlr = a;
	x = splfhi();
	armtimerset(0);
	wakechan = ctlr->wakechan;
	ctlr->wakechan = 0;
	splx(x);
	for(i = 0; wakechan; i++){
		if(wakechan & 1)
			wakeup(&ctlr->chanintr[i]);
		wakechan >>= 1;
	}
}

static void
epopen(Ep *ep)
{
	ddprint("usbotg: epopen ep%d.%d ttype %d\n",
		ep->dev->nb, ep->nb, ep->ttype);
	switch(ep->ttype){
	case Tnone:
		error(Enotconf);
	case Tintr:
		assert(ep->pollival > 0);
		/* fall through */
	case Tbulk:
		if(ep->toggle[Read] == 0)
			ep->toggle[Read] = DATA0;
		if(ep->toggle[Write] == 0)
			ep->toggle[Write] = DATA0;
		break;
	}
	ep->aux = malloc(sizeof(Epio));
	if(ep->aux == nil)
		error(Enomem);
}

static void
epclose(Ep *ep)
{
	ddprint("usbotg: epclose ep%d.%d ttype %d\n",
		ep->dev->nb, ep->nb, ep->ttype);
	switch(ep->ttype){
	case Tctl:
		freeb(((Epio*)ep->aux)->cb);
		/* fall through */
	default:
		free(ep->aux);
		break;
	}
}

static long
epread(Ep *ep, void *a, long n)
{
	Epio *epio;
	Block *b;
	uchar *p;
	ulong elapsed;
	long nr;

	ddprint("epread ep%d.%d %ld\n", ep->dev->nb, ep->nb, n);
	epio = ep->aux;
	b = nil;
	qlock(epio);
	if(waserror()){
		qunlock(epio);
		if(b)
			freeb(b);
		nexterror();
	}
	switch(ep->ttype){
	default:
		error(Egreg);
	case Tctl:
		nr = ctldata(ep, a, n);
		qunlock(epio);
		poperror();
		return nr;
	case Tintr:
		elapsed = TK2MS(m->ticks) - epio->lastpoll;
		if(elapsed < ep->pollival)
			tsleep(&up->sleep, return0, 0, ep->pollival - elapsed);
		/* fall through */
	case Tbulk:
		/* XXX cache madness */
		b = allocb(ROUND(n, ep->maxpkt) + CACHELINESZ);
		p = (uchar*)ROUND((uintptr)b->base, CACHELINESZ);
		cachedwbinvse(p, n);
		nr = eptrans(ep, Read, p, n);
		epio->lastpoll = TK2MS(m->ticks);
		memmove(a, p, nr);
		qunlock(epio);
		freeb(b);
		poperror();
		return nr;
	}
}

static long
epwrite(Ep *ep, void *a, long n)
{
	Epio *epio;
	Block *b;
	uchar *p;
	ulong elapsed;

	ddprint("epwrite ep%d.%d %ld\n", ep->dev->nb, ep->nb, n);
	epio = ep->aux;
	b = nil;
	qlock(epio);
	if(waserror()){
		qunlock(epio);
		if(b)
			freeb(b);
		nexterror();
	}
	switch(ep->ttype){
	default:
		error(Egreg);
	case Tintr:
		elapsed = TK2MS(m->ticks) - epio->lastpoll;
		if(elapsed < ep->pollival)
			tsleep(&up->sleep, return0, 0, ep->pollival - elapsed);
		/* fall through */
	case Tctl:
	case Tbulk:
		/* XXX cache madness */
		b = allocb(n + CACHELINESZ);
		p = (uchar*)ROUND((uintptr)b->base, CACHELINESZ);
		memmove(p, a, n);
		cachedwbse(p, n);
		if(ep->ttype == Tctl)
			n = ctltrans(ep, p, n);
		else{
			n = eptrans(ep, Write, p, n);
			epio->lastpoll = TK2MS(m->ticks);
		}
		qunlock(epio);
		freeb(b);
		poperror();
		return n;
	}
}

static char*
seprintep(char *s, char*, Ep*)
{
	return s;
}
	
static int
portenable(Hci *hp, int port, int on)
{
	Ctlr *ctlr;
	Dwcregs *r;

	assert(port == 1);
	ctlr = hp->aux;
	r = ctlr->regs;
	dprint("usbotg enable=%d; sts %#x\n", on, r->hport0);
	if(!on)
		r->hport0 = Prtpwr | Prtena;
	tsleep(&up->sleep, return0, 0, Enabledelay);
	dprint("usbotg enable=%d; sts %#x\n", on, r->hport0);
	return 0;
}

static int
portreset(Hci *hp, int port, int on)
{
	Ctlr *ctlr;
	Dwcregs *r;
	int b, s;

	assert(port == 1);
	ctlr = hp->aux;
	r = ctlr->regs;
	dprint("usbotg reset=%d; sts %#x\n", on, r->hport0);
	if(!on)
		return 0;
	r->hport0 = Prtpwr | Prtrst;
	tsleep(&up->sleep, return0, 0, ResetdelayHS);
	r->hport0 = Prtpwr;
	tsleep(&up->sleep, return0, 0, Enabledelay);
	s = r->hport0;
	b = s & (Prtconndet|Prtenchng|Prtovrcurrchng);
	if(b != 0)
		r->hport0 = Prtpwr | b;
	dprint("usbotg reset=%d; sts %#x\n", on, s);
	if((s & Prtena) == 0)
		print("usbotg: host port not enabled after reset");
	return 0;
}

static int
portstatus(Hci *hp, int port)
{
	Ctlr *ctlr;
	Dwcregs *r;
	int b, s;

	assert(port == 1);
	ctlr = hp->aux;
	r = ctlr->regs;
	s = r->hport0;
	b = s & (Prtconndet|Prtenchng|Prtovrcurrchng);
	if(b != 0)
		r->hport0 = Prtpwr | b;
	b = 0;
	if(s & Prtconnsts)
		b |= HPpresent;
	if(s & Prtconndet)
		b |= HPstatuschg;
	if(s & Prtena)
		b |= HPenable;
	if(s & Prtenchng)
		b |= HPchange;
	if(s & Prtovrcurract)
		 b |= HPovercurrent;
	if(s & Prtsusp)
		b |= HPsuspend;
	if(s & Prtrst)
		b |= HPreset;
	if(s & Prtpwr)
		b |= HPpower;
	switch(s & Prtspd){
	case HIGHSPEED:
		b |= HPhigh;
		break;
	case LOWSPEED:
		b |= HPslow;
		break;
	}
	return b;
}

static void
shutdown(Hci*)
{
}

static void
setdebug(Hci*, int d)
{
	debug = d;
}

static int
reset(Hci *hp)
{
	Ctlr *ctlr;
	uint id;

	ctlr = &dwc;
	if(ctlr->regs != nil)
		return -1;
	ctlr->regs = (Dwcregs*)USBREGS;
	id = ctlr->regs->gsnpsid;
	if((id>>16) != ('O'<<8 | 'T'))
		return -1;
	dprint("usbotg: rev %d.%3.3x\n", (id>>12)&0xF, id&0xFFF);

	intrenable(IRQtimerArm, irqintr, ctlr, 0, "dwc");

	hp->aux = ctlr;
	hp->port = 0;
	hp->irq = IRQusb;
	hp->tbdf = 0;
	hp->nports = 1;
	hp->highspeed = 1;

	hp->init = init;
	hp->dump = dump;
	hp->interrupt = fiqintr;
	hp->epopen = epopen;
	hp->epclose = epclose;
	hp->epread = epread;
	hp->epwrite = epwrite;
	hp->seprintep = seprintep;
	hp->portenable = portenable;
	hp->portreset = portreset;
	hp->portstatus = portstatus;
	hp->shutdown = shutdown;
	hp->debug = setdebug;
	hp->type = "dwcotg";
	return 0;
}

void
usbdwclink(void)
{
	addhcitype("dwcotg", reset);
}
