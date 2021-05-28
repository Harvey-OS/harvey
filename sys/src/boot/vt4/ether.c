#include "include.h"
#include "ip.h"

static Ether *ether;

int debug;

int	temacreset(Ether*);
int	plbtemacreset(Ether*);
int	isaconfig(char*, int, ISAConf*);

typedef struct Ethercard Ethercard;
struct Ethercard {
	char	*type;
	int	(*reset)(Ether*);
	int	noprobe;
} ethercards[] = {
	{ "temac", temacreset, 0, },
	{ 0, }
};

static void xetherdetach(void);

int
etherinit(void)		/* called from probe() */
{
	Ether *ctlr;
	int ctlrno, i, mask, n, x;

	fmtinstall('E', eipfmt);
	fmtinstall('V', eipfmt);

	etherdetach = xetherdetach;
	mask = 0;
	ether = malloc(MaxEther * sizeof *ether);
	for(ctlrno = 0; ctlrno < MaxEther; ctlrno++){
		ctlr = &ether[ctlrno];
		memset(ctlr, 0, sizeof(Ether));
//		if(isaconfig("ether", ctlrno, ctlr) == 0)
//			continue;

		for(n = 0; ethercards[n].type; n++){
			Ethercard *ecp;

			ecp = &ethercards[n];
			if (1) {
				if(ecp->noprobe)
					continue;
				memset(ctlr, 0, sizeof(Ether));
//				strecpy(ctlr->type, &ctlr->type[NAMELEN],
//					ecp->type);
			}
//			else if(cistrcmp(ecp->type, ctlr->type) != 0)
//				continue;
			ctlr->ctlrno = ctlrno;

			x = splhi();
			if((*ecp->reset)(ctlr)){
				splx(x);
				continue;
			}

			ctlr->state = 1;		/* card found */
			mask |= 1<<ctlrno;
//			setvec(VectorPIC + ctlr->irq, ctlr->interrupt, ctlr);
			intrenable(Inttemac, ctlr->interrupt);

			print("ether#%d: port 0x%lux", ctlr->ctlrno, ctlr->port);
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
	seprint(s, s + NAMELEN, "ether%d", i);
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
	if(ctlr->state == 1){		/* card found? */
		ctlr->state = 2;	/* attaching */
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
		if(ctlr->detach && ctlr->state != 0)	/* found | attaching? */
			ctlr->detach(ctlr);
	}
	splx(x);
}

uchar*
etheraddr(int ctlrno)
{
	Ether *ctlr;

	if((ctlr = attach(ctlrno)) == 0)
		return 0;

	return ctlr->ea;
}

/* wait for owner of RingBuf to change from `owner' */
static int
wait(RingBuf* ring, uchar owner, int timo)
{
	ulong start;

	start = m->ticks;
	while(TK2MS(m->ticks - start) < timo)
		if(ring->owner != owner)
			return 1;
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
	coherence();
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
	coherence();
	s = splhi();
	(*ctlr->transmit)(ctlr);
	splx(s);

	return 1;
}
