#include "all.h"
#include "io.h"
#include "mem.h"

#include "ether.h"

Ctlr ether[MaxEther];

extern int ne2000reset(Ctlr*);
extern int tcm509reset(Ctlr*);
extern int wd8003reset(Ctlr*);

static struct {
	char	*type;
	int	(*reset)(Ctlr*);
} cards[] = {
	{ "NE2000", ne2000reset, },
	{ "3C509", tcm509reset, },
	{ "WD8003", wd8003reset, },
	{ 0, }
};

void
etherinit(void)
{
	Ctlr *ctlr;
	int i, n, ctlrno;

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
			if(ctlr->card.irq == 2)
				ctlr->card.irq = 9;
			setvec(Int0vec + ctlr->card.irq, ctlr->card.intr, ctlr);
			memmove(ctlr->ifc.ea, ctlr->card.ea, 6);

			print("ether%d: %s: port %lux irq %d addr %lux size %d width %d:",
				ctlr->ctlrno, ctlr->card.type, ctlr->card.port, ctlr->card.irq,
				ctlr->card.mem, ctlr->card.size, ctlr->card.bit16 ? 16: 8);
			for(i = 0; i < sizeof(ctlr->ifc.ea); i++)
				print(" %2.2ux", ctlr->ifc.ea[i]);
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
}

static void
stats(Ctlr *ctlr)
{
	Ifc *ifc;

	print("ether stats %d\n", ctlr-ether);
	ifc = &ctlr->ifc;
	print("	work = %F pkts\n", (Filta){&ifc->work, 1});
	print("	rate = %F tBps\n", (Filta){&ifc->rate, 1000});
	print("	err  =    %3ld rc %3ld sum\n", ifc->rcverr, ifc->sumerr);
}

static void
cmd_state(int argc, char *argv[])
{
	Ctlr *ctlr;

	USED(argc, argv);

	for(ctlr = &ether[0]; ctlr < &ether[MaxEther]; ctlr++){
		if(ctlr->present)
			stats(ctlr);
	}
}

static int
isinput(void *arg)
{
	Ctlr *ctlr = arg;

	return ctlr->rb[ctlr->rh].owner == Host;
}

static void
etherinput(void)
{
	Ctlr *ctlr;
	Ifc *ifc;
	RingBuf *ring;
	Enpkt *p;

	ctlr = getarg();
	ifc = &ctlr->ifc;
	print("ether%di: %E %I\n", ctlr->ctlrno, ctlr->ifc.ea, ctlr->ifc.ipa);	
	(*ctlr->card.attach)(ctlr);

loop:
	while(isinput(ctlr) == 0)
		sleep(&ctlr->rr, isinput, ctlr);

	while(ctlr->rb[ctlr->rh].owner == Host){
		ring = &ctlr->rb[ctlr->rh];
		p = (Enpkt*)ring->pkt;

		switch(nhgets(p->type)){
		case Arptype:
			arpreceive(p, ring->len, ifc);
			break;
		case Iptype:
			ipreceive(p, ring->len, ifc);
			ifc->rxpkt++;
			ifc->work.count++;
			ifc->rate.count += ring->len;
			break;
		}

		ring->owner = Interface;
		ctlr->rh = NEXT(ctlr->rh, ctlr->nrb);
	}

	goto loop;
}

static int
isoutput(void *arg)
{
	Ctlr *ctlr = arg;

	return ctlr->tb[ctlr->th].owner == Host;
}

static void
etheroutput(void)
{
	Ctlr *ctlr;
	Ifc *ifc;
	RingBuf *ring;
	Msgbuf *mb;
	int len, s;

	ctlr = getarg();
	ifc = &ctlr->ifc;
	print("ether%do: %E %I\n", ctlr->ctlrno, ifc->ea, ifc->ipa);	

loop:
	while((mb = recv(ifc->reply, 0)) == 0)
		;

	memmove(((Enpkt*)(mb->data))->s, ifc->ea, Easize);
	if((len = mb->count) > ETHERMAXTU){
		print("ether%do: pkt too big - %d\n", ctlr->ctlrno, len);
		mbfree(mb);
		goto loop;
	}

	while(isoutput(ctlr) == 0)
		sleep(&ctlr->tr, isoutput, ctlr);

	ring = &ctlr->tb[ctlr->th];
	memmove(ring->pkt, mb->data, len);
	if(len < ETHERMINTU){
		memset(ring->pkt+len, 0, ETHERMINTU-len);
		len = ETHERMINTU;
	}

	s = splhi();
	ring->len = len;
	ring->owner = Interface;
	ctlr->th = NEXT(ctlr->th, ctlr->ntb);
	(*ctlr->card.transmit)(ctlr);
	splx(s);

	ifc->work.count++;
	ifc->rate.count += len;
	ifc->txpkt++;
	mbfree(mb);
	goto loop;
}

void
etherstart(void)
{
	Ctlr *ctlr;
	Ifc *ifc;

	for(ctlr = &ether[0]; ctlr < &ether[MaxEther]; ctlr++){
		if(ctlr->present){
			ifc = &ctlr->ifc;
			lock(ifc);
			if(ifc->reply == 0){
				dofilter(&ifc->work);
				dofilter(&ifc->rate);
				ifc->reply = newqueue(Nqueue);
			}
			getipa(ifc);
			unlock(ifc);
			sprint(ctlr->oname, "ether%do", ctlr->ctlrno);
			userinit(etheroutput, ctlr, ctlr->oname);
			sprint(ctlr->iname, "ether%di", ctlr->ctlrno);
			userinit(etherinput, ctlr, ctlr->iname);
			cmd_install("state", "-- ether stats", cmd_state);

			ifc->next = enets;
			enets = ifc;
		}
	}
	arpstart();
}
