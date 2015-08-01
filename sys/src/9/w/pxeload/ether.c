/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ip.h"

#include "etherif.h"

static Ether ether[MaxEther];

extern int amd79c970reset(Ether*);
extern int ether2114xreset(Ether*);
extern int etherbcm4401pnp(Ether*);
extern int i82557reset(Ether*);
extern int i82563pnp(Ether*);
extern int igbepnp(Ether *);
extern int rtl8139pnp(Ether*);
extern int rtl8169pnp(Ether*);
extern int vgbepnp(Ether*);

struct {
	char	*type;
	int	(*reset)(Ether*);
	int	noprobe;
} ethercards[] = {
	{ "21140", ether2114xreset, 0, },
	{ "2114x", ether2114xreset, 0, },
	{ "bcm4401", etherbcm4401pnp, 0, },
	{ "i82557", i82557reset, 0, },
	{ "i82563", i82563pnp, 0, },
	{ "igbe",  igbepnp, 0, },
	{ "RTL8139", rtl8139pnp, 0, },
	{ "RTL8169", rtl8169pnp, 0, },
	{ "AMD79C970", amd79c970reset, 0, },
	{ "vgbe", vgbepnp, 0, },

	{ 0, }
};

static void xetherdetach(void);

int
etherinit(void)
{
	Ether *ctlr;
	int ctlrno, i, mask, n, x;

	fmtinstall('E', eipfmt);

	etherdetach = xetherdetach;
	mask = 0;
	for(ctlrno = 0; ctlrno < MaxEther; ctlrno++){
		ctlr = &ether[ctlrno];
		memset(ctlr, 0, sizeof(Ether));
		if(iniread && isaconfig("ether", ctlrno, ctlr) == 0)
			continue;

		for(n = 0; ethercards[n].type; n++){
			if(!iniread){
				if(ethercards[n].noprobe)
					continue;
				memset(ctlr, 0, sizeof(Ether));
				ctlr->type = ethercards[n].type;
			}
			else if(cistrcmp(ethercards[n].type, ctlr->type))
				continue;
			ctlr->ctlrno = ctlrno;

			x = splhi();
			if((*ethercards[n].reset)(ctlr)){
				splx(x);
				if(iniread)
					break;
				else
					continue;
			}

			ctlr->state = 1;
			mask |= 1<<ctlrno;
			if(ctlr->irq == 2)
				ctlr->irq = 9;
			setvec(VectorPIC + ctlr->irq, ctlr->interrupt, ctlr);

			print("ether#%d: %s: port 0x%luX irq %lud",
				ctlr->ctlrno, ctlr->type, ctlr->port, ctlr->irq);
			if(ctlr->mem)
				print(" addr 0x%luX", ctlr->mem & ~KZERO);
			if(ctlr->size)
				print(" size 0x%luX", ctlr->size);
			print(": %E\n", ctlr->ea);

			if(ctlr->nrb == 0)
				ctlr->nrb = Nrb;
			ctlr->rb = ialloc(sizeof(RingBuf)*ctlr->nrb, 0);
			if(ctlr->ntb == 0)
				ctlr->ntb = Ntb;
			ctlr->tb = ialloc(sizeof(RingBuf)*ctlr->ntb, 0);

			ctlr->rh = 0;
			ctlr->ri = 0;
			for(i = 0; i < ctlr->nrb; i++)
				ctlr->rb[i].owner = Interface;

			ctlr->th = 0;
			ctlr->ti = 0;
			for(i = 0; i < ctlr->ntb; i++)
				ctlr->tb[i].owner = Host;

			splx(x);
			break;
		}
	}

	return mask;
}

void
etherinitdev(int i, char *s)
{
	sprint(s, "ether%d", i);
}

void
etherprintdevs(int i)
{
	print(" ether%d", i);
}

static Ether*
attach(int ctlrno)
{
	Ether *ctlr;

	if(ctlrno >= MaxEther || ether[ctlrno].state == 0)
		return 0;

	ctlr = &ether[ctlrno];
	if(ctlr->state == 1){
		ctlr->state = 2;
		(*ctlr->attach)(ctlr);
	}

	return ctlr;
}

static void
xetherdetach(void)
{
	Ether *ctlr;
	int ctlrno, x;

	x = splhi();
	for(ctlrno = 0; ctlrno < MaxEther; ctlrno++){
		ctlr = &ether[ctlrno];
		if(ctlr->detach && ctlr->state != 0)
			ctlr->detach(ctlr);
	}
	splx(x);
}

uint8_t*
etheraddr(int ctlrno)
{
	Ether *ctlr;

	if((ctlr = attach(ctlrno)) == 0)
		return 0;

	return ctlr->ea;
}

static int
wait(RingBuf* ring, uint8_t owner, int timo)
{
	uint32_t start;
	extern void hlt(void);

	start = machp()->ticks;
	while(TK2MS(machp()->ticks - start) < timo){
		if(ring->owner != owner)
			return 1;
		hlt();
	}

	return 0;
}

int
etherrxpkt(int ctlrno, Etherpkt* pkt, int timo)
{
	int n;
	Ether *ctlr;
	RingBuf *ring;

	if((ctlr = attach(ctlrno)) == 0)
		return 0;

	ring = &ctlr->rb[ctlr->rh];
	if(wait(ring, Interface, timo) == 0){
		if(debug)
			print("ether%d: rx timeout\n", ctlrno);
		return 0;
	}

	n = ring->len;
	memmove(pkt, ring->pkt, n);
	ring->owner = Interface;
	ctlr->rh = NEXT(ctlr->rh, ctlr->nrb);

	return n;
}

int
etherrxflush(int ctlrno)
{
	int n;
	Ether *ctlr;
	RingBuf *ring;

	if((ctlr = attach(ctlrno)) == 0)
		return 0;

	n = 0;
	for(;;){
		ring = &ctlr->rb[ctlr->rh];
		if(wait(ring, Interface, 100) == 0)
			break;

		ring->owner = Interface;
		ctlr->rh = NEXT(ctlr->rh, ctlr->nrb);
		n++;
	}

	return n;
}

int
ethertxpkt(int ctlrno, Etherpkt* pkt, int len, int)
{
	Ether *ctlr;
	RingBuf *ring;
	int s;

	if((ctlr = attach(ctlrno)) == 0)
		return 0;

	ring = &ctlr->tb[ctlr->th];
	if(wait(ring, Interface, 1000) == 0){
		print("ether%d: tx buffer timeout\n", ctlrno);
		return 0;
	}

	memmove(pkt->s, ctlr->ea, Eaddrlen);
	if(debug)
		print("%E to %E...\n", pkt->s, pkt->d);
	memmove(ring->pkt, pkt, len);
	if(len < ETHERMINTU){
		memset(ring->pkt+len, 0, ETHERMINTU-len);
		len = ETHERMINTU;
	}
	ring->len = len;
	ring->owner = Interface;
	ctlr->th = NEXT(ctlr->th, ctlr->ntb);
	s = splhi();
	(*ctlr->transmit)(ctlr);
	splx(s);

	return 1;
}
