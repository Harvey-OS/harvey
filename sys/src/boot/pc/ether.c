#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "ether.h"

static Ctlr ether[MaxEther];

static struct {
	char	*type;
	int	(*reset)(Ctlr*);
} cards[] = {
	{ "NE2000", ne2000reset, },
	{ "NE3210", ne3210reset, },
	{ "3C509", tcm509reset, },
	{ "WD8003", wd8003reset, },
	{ 0, }
};

int
etherinit(void)
{
	Ctlr *ctlr;
	int ctlrno, i, mask, n;

	mask = 0;
	for(ctlrno = 0; ctlrno < MaxEther; ctlrno++){
		ctlr = &ether[ctlrno];
		memset(ctlr, 0, sizeof(Ctlr));
		if(isaconfig("ether", ctlrno, &ctlr->card) == 0)
			continue;
		for(n = 0; cards[n].type; n++){
			if(strcmp(cards[n].type, ctlr->card.type))
				continue;
			ctlr->ctlrno = ctlrno;
			if((*cards[n].reset)(ctlr))
				break;

			ctlr->present = 1;
			mask |= 1<<ctlrno;
			if(ctlr->card.irq == 2)
				ctlr->card.irq = 9;
			setvec(Int0vec + ctlr->card.irq, ctlr->card.intr, ctlr);

			print("ether%d: %s: port %lux irq %d addr %lux size %d width %d:",
				ctlr->ctlrno, ctlr->card.type, ctlr->card.port, ctlr->card.irq,
				ctlr->card.mem, ctlr->card.size, ctlr->card.bit16 ? 16: 8);
			for(i = 0; i < sizeof(ctlr->card.ea); i++)
				print(" %2.2ux", ctlr->card.ea[i]);
			print("\n");
		
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
		
			break;
		}
	}

	return mask;
}

static Ctlr*
attach(int ctlrno)
{
	Ctlr *ctlr;

	if(ctlrno >= MaxEther || ether[ctlrno].present == 0)
		return 0;

	ctlr = &ether[ctlrno];
	if(ctlr->present == 1){
		ctlr->present = 2;
		(*ctlr->card.attach)(ctlr);
	}

	return ctlr;
}

uchar*
etheraddr(int ctlrno)
{
	Ctlr *ctlr;

	if((ctlr = attach(ctlrno)) == 0)
		return 0;

	return ctlr->card.ea;
}

static int
wait(RingBuf *ring, uchar owner, int timo)
{
	ulong start;

	start = m->ticks;
	while(TK2MS(m->ticks - start) < timo){
		if(ring->owner != owner)
			return 1;
	}

	return 0;
}

int
etherrxpkt(int ctlrno, Etherpkt *pkt, int timo)
{
	int n;
	Ctlr *ctlr;
	RingBuf *ring;

	if((ctlr = attach(ctlrno)) == 0)
		return 0;

	ring = &ctlr->rb[ctlr->rh];
	if(wait(ring, Interface, timo) == 0){
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
ethertxpkt(int ctlrno, Etherpkt *pkt, int len, int timo)
{
	Ctlr *ctlr;
	RingBuf *ring;

	if((ctlr = attach(ctlrno)) == 0)
		return 0;

	ring = &ctlr->tb[ctlr->th];
	memmove(ring->pkt, pkt, len);
	if(len < ETHERMINTU){
		memset(ring->pkt+len, 0, ETHERMINTU-len);
		len = ETHERMINTU;
	}
	ring->len = len;
	ring->owner = Interface;
	ctlr->th = NEXT(ctlr->th, ctlr->ntb);
	(*ctlr->card.transmit)(ctlr);

	if(wait(ring, Interface, timo) == 0){
		print("ether%d: rx timeout\n", ctlrno);
		return 0;
	}

	return 1;
}
