/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"

#include "etherif.h"

static Ether *etherxx[MaxEther];

extern Ether *archetherprobe(int ctlrno, char *type, int (*reset)(Ether *));
extern void archethershutdown(Ether *ether);

Chan*
etherattach(char* spec)
{
	Proc *up = externup();
	int ctlrno;
	char *p;
	Chan *chan;

	ctlrno = 0;
	if(spec && *spec){
		ctlrno = strtoul(spec, &p, 0);
		if((ctlrno == 0 && p == spec) || *p || (ctlrno >= MaxEther))
			error(Ebadarg);
	}
	if(etherxx[ctlrno] == 0)
		error(Enodev);

	chan = devattach('l', spec);
	if(waserror()){
		chanfree(chan);
		nexterror();
	}
	chan->devno = ctlrno;
	if(etherxx[ctlrno]->attach)
		etherxx[ctlrno]->attach(etherxx[ctlrno]);
	poperror();
	return chan;
}

static Walkqid*
etherwalk(Chan* chan, Chan* nchan, char** name, int nname)
{
	return netifwalk(&etherxx[chan->devno]->Netif, chan, nchan, name, nname);
}

static int32_t
etherstat(Chan* chan, uint8_t* dp, int32_t n)
{
	return netifstat(&etherxx[chan->devno]->Netif, chan, dp, n);
}

static Chan*
etheropen(Chan* chan, int omode)
{
	return netifopen(&etherxx[chan->devno]->Netif, chan, omode);
}

static void
ethercreate(Chan* c, char* d, int i, int n)
{
}

static void
etherclose(Chan* chan)
{
	netifclose(&etherxx[chan->devno]->Netif, chan);
}

static int32_t
etherread(Chan* chan, void* buf, int32_t n, int64_t off)
{
	Ether *ether;
	uint32_t offset = off;

	ether = etherxx[chan->devno];
	if((chan->qid.type & QTDIR) == 0 && ether->ifstat){
		/*
		 * With some controllers it is necessary to reach
		 * into the chip to extract statistics.
		 */
		if(NETTYPE(chan->qid.path) == Nifstatqid)
			return ether->ifstat(ether, buf, n, offset);
		else if(NETTYPE(chan->qid.path) == Nstatqid)
			ether->ifstat(ether, buf, 0, offset);
	}

	return netifread(&ether->Netif, chan, buf, n, offset);
}

static Block*
etherbread(Chan* chan, int32_t n, int64_t offset)
{
	return netifbread(&etherxx[chan->devno]->Netif, chan, n, offset);
}

static int32_t
etherwstat(Chan* chan, uint8_t* dp, int32_t n)
{
	return netifwstat(&etherxx[chan->devno]->Netif, chan, dp, n);
}

static void
etherrtrace(Netfile* f, Etherpkt* pkt, int len)
{
	int i, n;
	Block *bp;

	if(qwindow(f->iq) <= 0)
		return;
	if(len > 58)
		n = 58;
	else
		n = len;
	bp = iallocb(64);
	if(bp == nil)
		return;
	memmove(bp->wp, pkt->d, n);
	i = TK2MS(sys->ticks);
	bp->wp[58] = len>>8;
	bp->wp[59] = len;
	bp->wp[60] = i>>24;
	bp->wp[61] = i>>16;
	bp->wp[62] = i>>8;
	bp->wp[63] = i;
	bp->wp += 64;
	qpass(f->iq, bp);
}

Block*
etheriq(Ether* ether, Block* bp, int fromwire)
{
	Etherpkt *pkt;
	uint16_t type;
	int multi, tome, fromme;
	Netfile **ep, *f, **fp, *fx;
	Block *xbp;
	size_t len;

	ether->Netif.inpackets++;

	pkt = (Etherpkt*)bp->rp;
	len = BLEN(bp);
	type = (pkt->type[0]<<8)|pkt->type[1];
	fx = 0;
	ep = &ether->Netif.f[Ntypes];

	multi = pkt->d[0] & 1;
	/* check for valid multicast addresses */
	if(multi && memcmp(pkt->d, ether->Netif.bcast, sizeof(pkt->d)) != 0 && ether->Netif.prom == 0){
		if(!activemulti(&ether->Netif, pkt->d, sizeof(pkt->d))){
			if(fromwire){
				freeb(bp);
				bp = 0;
			}
			return bp;
		}
	}

	/* is it for me? */
	tome = memcmp(pkt->d, ether->ea, sizeof(pkt->d)) == 0;
	fromme = memcmp(pkt->s, ether->ea, sizeof(pkt->s)) == 0;

	/*
	 * Multiplex the packet to all the connections which want it.
	 * If the packet is not to be used subsequently (fromwire != 0),
	 * attempt to simply pass it into one of the connections, thereby
	 * saving a copy of the data (usual case hopefully).
	 */
	for(fp = ether->Netif.f; fp < ep; fp++){
		if(f = *fp)
		if(f->type == type || f->type < 0)
		if(tome || multi || f->prom || f->bridge & 2){
			/* Don't want to hear bridged packets */
			if(f->bridge && !fromwire && !fromme)
				continue;
			if(!f->headersonly){
				if(fromwire && fx == 0)
					fx = f;
				else if(xbp = iallocb(len)){
					memmove(xbp->wp, pkt, len);
					xbp->wp += len;
					if(qpass(f->iq, xbp) < 0)
						ether->Netif.soverflows++;
				}
				else
					ether->Netif.soverflows++;
			}
			else
				etherrtrace(f, pkt, len);
		}
	}

	if(fx){
		if(qpass(fx->iq, bp) < 0)
			ether->Netif.soverflows++;
		return 0;
	}
	if(fromwire){
		freeb(bp);
		return 0;
	}

	return bp;
}

static int
etheroq(Ether* ether, Block* bp)
{
	int loopback, s;
	Etherpkt *pkt;
	size_t len;

	ether->Netif.outpackets++;

	/*
	 * Check if the packet has to be placed back onto the input queue,
	 * i.e. if it's a loopback or broadcast packet or the interface is
	 * in promiscuous mode.
	 * If it's a loopback packet indicate to etheriq that the data isn't
	 * needed and return, etheriq will pass-on or free the block.
	 * To enable bridging to work, only packets that were originated
	 * by this interface are fed back.
	 */
	pkt = (Etherpkt*)bp->rp;
	len = BLEN(bp);
	loopback = memcmp(pkt->d, ether->ea, sizeof(pkt->d)) == 0;
	if(loopback || memcmp(pkt->d, ether->Netif.bcast, sizeof(pkt->d)) == 0 || ether->Netif.prom){
		s = splhi();
		etheriq(ether, bp, 0);
		splx(s);
	}

	if(!loopback){
		qbwrite(ether->oq, bp);
		if(ether->transmit != nil)
			ether->transmit(ether);
	} else
		freeb(bp);

	return len;
}

static int32_t
etherwrite(Chan* chan, void* buf, int32_t n, int64_t mm)
{
	Proc *up = externup();
	Ether *ether;
	Block *bp;
	int nn, onoff;
	Cmdbuf *cb;

	ether = etherxx[chan->devno];
	if(NETTYPE(chan->qid.path) != Ndataqid) {
		nn = netifwrite(&ether->Netif, chan, buf, n);
		if(nn >= 0)
			return nn;
		cb = parsecmd(buf, n);
		if(cb->f[0] && strcmp(cb->f[0], "nonblocking") == 0){
			if(cb->nf <= 1)
				onoff = 1;
			else
				onoff = atoi(cb->f[1]);
			qnoblock(ether->oq, onoff);
			free(cb);
			return n;
		}
		free(cb);
		if(ether->ctl != nil)
			return ether->ctl(ether, buf, n);

		error(Ebadctl);
	}

	if(n > ether->Netif.mtu)
		error(Etoobig);
	if(n < ether->Netif.minmtu)
		error(Etoosmall);

	bp = allocb(n);
	if(waserror()){
		freeb(bp);
		nexterror();
	}
	memmove(bp->rp, buf, n);
	if((ether->Netif.f[NETID(chan->qid.path)]->bridge & 2) == 0)
		memmove(bp->rp+Eaddrlen, ether->ea, Eaddrlen);
	poperror();
	bp->wp += n;

	return etheroq(ether, bp);
}

static int32_t
etherbwrite(Chan* chan, Block* bp, int64_t mm)
{
	Proc *up = externup();
	Ether *ether;
	size_t n;

	n = BLEN(bp);
	if(NETTYPE(chan->qid.path) != Ndataqid){
		if(waserror()) {
			freeb(bp);
			nexterror();
		}
		n = etherwrite(chan, bp->rp, n, 0);
		poperror();
		freeb(bp);
		return n;
	}
	ether = etherxx[chan->devno];

	if(n > ether->Netif.mtu){
		freeb(bp);
		error(Etoobig);
	}
	if(n < ether->Netif.minmtu){
		freeb(bp);
		error(Etoosmall);
	}

	return etheroq(ether, bp);
}

static struct {
	char*	type;
	int	(*reset)(Ether*);
} cards[MaxEther+1];

void
addethercard(char* t, int (*r)(Ether*))
{
	static int ncard;

	if(ncard == MaxEther)
		panic("too many ether cards");
	cards[ncard].type = t;
	cards[ncard].reset = r;
	ncard++;
}

int
parseether(uint8_t *to, char *from)
{
	char nip[4];
	char *p;
	int i;

	p = from;
	for(i = 0; i < Eaddrlen; i++){
		if(*p == 0)
			return -1;
		nip[0] = *p++;
		if(*p == 0)
			return -1;
		nip[1] = *p++;
		nip[2] = 0;
		to[i] = strtoul(nip, 0, 16);
		if(*p == ':')
			p++;
	}
	return 0;
}

static Ether*
etherprobe(int cardno, int ctlrno)
{

	if(cardno >= MaxEther)
		return nil;
	if(cards[cardno].type == nil || cards[cardno].reset == nil)
		return nil;
	return archetherprobe(ctlrno, cards[cardno].type, cards[cardno].reset);
}

static void
etherreset(void)
{
	Ether *ether;
	int cardno, ctlrno;

	cardno = ctlrno = 0;
	while(cards[cardno].type != nil && ctlrno < MaxEther){
		if(etherxx[ctlrno] != nil){
			ctlrno++;
			continue;
		}
		if((ether = etherprobe(cardno, ctlrno)) == nil){
			cardno++;
			continue;
		}
		etherxx[ctlrno] = ether;
		ctlrno++;
	}
}

static void
ethershutdown(void)
{
	char name[32];
	int i;
	Ether *ether;

	for(i = 0; i < MaxEther; i++){
		ether = etherxx[i];
		if(ether == nil)
			continue;
		if(ether->shutdown == nil) {
			print("#l%d: no shutdown function\n", i);
			continue;
		}
		snprint(name, sizeof(name), "ether%d", i);
		archethershutdown(ether);
		(*ether->shutdown)(ether);
	}
}


#define POLY 0xedb88320

/* really slow 32 bit crc for ethers */
uint32_t
ethercrc(uint8_t *p, int len)
{
	int i, j;
	uint32_t crc, b;

	crc = 0xffffffff;
	for(i = 0; i < len; i++){
		b = *p++;
		for(j = 0; j < 8; j++){
			crc = (crc>>1) ^ (((crc^b) & 1) ? POLY : 0);
			b >>= 1;
		}
	}
	return crc;
}

Dev etherdevtab = {
	.dc = 'l',
	.name = "ether",

	.reset = etherreset,
	.init = devinit,
	.shutdown = ethershutdown,
	.attach = etherattach,
	.walk = etherwalk,
	.stat = etherstat,
	.open = etheropen,
	.create = ethercreate,
	.close = etherclose,
	.read = etherread,
	.bread = etherbread,
	.write = etherwrite,
	.bwrite = etherbwrite,
	.remove = devremove,
	.wstat = etherwstat,
};
