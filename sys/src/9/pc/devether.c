#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"
#include "devtab.h"

#include "ether.h"

/*
 * Half-arsed attempt at a general top-level
 * ethernet driver. Needs work:
 *	handle multiple controllers
 *	much tidying
 *	set ethernet address
 */
extern Board ether8003;
extern Board ether503;
extern Board ether2000;
extern Board ether509;

/*
 * The ordering here is important for those boards
 * using the DP8390 (WD8003, 3COM503 and NE2000) as
 * attempting to determine if a board is a NE2000
 * cannot be done passively, so it must be last to
 * prevent scrogging one of the others.
 */
static Board *boards[] = {
	&ether8003,
	&ether503,
	&ether2000,

	&ether509,
	0
};

enum {
	NCtlr		= 1,
};

static struct Ctlr *softctlr[NCtlr];
static int nctlr;

Chan*
etherclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
etherwalk(Chan *c, char *name)
{
	return netwalk(c, name, &softctlr[0]->net);
}

void
etherstat(Chan *c, char *dp)
{
	netstat(c, dp, &softctlr[0]->net);
}

Chan*
etheropen(Chan *c, int omode)
{
	return netopen(c, omode, &softctlr[0]->net);
}

void
ethercreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
etherclose(Chan *c)
{
	if(c->stream)
		streamclose(c);
}

long
etherread(Chan *c, void *a, long n, ulong offset)
{
	return netread(c, a, n, offset, &softctlr[0]->net);
}

long
etherwrite(Chan *c, char *a, long n, ulong offset)
{
	USED(offset);
	return streamwrite(c, a, n, 0);
}

void
etherremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
etherwstat(Chan *c, char *dp)
{
	netwstat(c, dp, &softctlr[0]->net);
}

static int
isobuf(void *arg)
{
	Ctlr *ctlr = arg;

	return ctlr->tb[ctlr->th].owner == Host;
}

static void
etheroput(Queue *q, Block *bp)
{
	Ctlr *ctlr;
	Type *type;
	Etherpkt *pkt;
	RingBuf *ring;
	int len, n;
	Block *nbp;

	type = q->ptr;
	ctlr = type->ctlr;
	if(bp->type == M_CTL){
		qlock(ctlr);
		if(streamparse("connect", bp)){
			if(type->type == -1)
				ctlr->all--;
			type->type = strtol((char*)bp->rptr, 0, 0);
			if(type->type == -1)
				ctlr->all++;
		}
		else if(streamparse("promiscuous", bp)) {
			type->prom = 1;
			ctlr->prom++;
			if(ctlr->prom == 1)
				(*ctlr->board->mode)(ctlr, 1);
		}
		qunlock(ctlr);
		freeb(bp);
		return;
	}

	/*
	 * Give packet a local address, return upstream if destined for
	 * this machine.
	 */
	if(BLEN(bp) < ETHERHDRSIZE && (bp = pullup(bp, ETHERHDRSIZE)) == 0)
		return;
	pkt = (Etherpkt*)bp->rptr;
	memmove(pkt->s, ctlr->ea, sizeof(ctlr->ea));
	if(memcmp(ctlr->ea, pkt->d, sizeof(ctlr->ea)) == 0){
		len = blen(bp);
		if(bp = expandb(bp, len >= ETHERMINTU ? len: ETHERMINTU)){
			putq(&ctlr->lbq, bp);
			wakeup(&ctlr->rr);
		}
		return;
	}
	if(memcmp(ctlr->ba, pkt->d, sizeof(ctlr->ba)) == 0 || ctlr->prom || ctlr->all){
		len = blen(bp);
		nbp = copyb(bp, len);
		if(nbp = expandb(nbp, len >= ETHERMINTU ? len: ETHERMINTU)){
			nbp->wptr = nbp->rptr+len;
			putq(&ctlr->lbq, nbp);
			wakeup(&ctlr->rr);
		}
	}

	/*
	 * Only one transmitter at a time.
	 */
	qlock(&ctlr->tlock);
	if(waserror()){
		freeb(bp);
		qunlock(&ctlr->tlock);
		nexterror();
	}

	/*
	 * Wait till we get an output buffer.
	 * should try to restart.
	 */
	sleep(&ctlr->tr, isobuf, ctlr);

	ring = &ctlr->tb[ctlr->th];

	/*
	 * Copy message into buffer.
	 */
	len = 0;
	for(nbp = bp; nbp; nbp = nbp->next){
		if(sizeof(Etherpkt) - len >= (n = BLEN(nbp))){
			memmove(ring->pkt+len, nbp->rptr, n);
			len += n;
		}
		if(bp->flags & S_DELIM)
			break;
	}

	/*
	 * Pad the packet (zero the pad).
	 */
	if(len < ETHERMINTU){
		memset(ring->pkt+len, 0, ETHERMINTU-len);
		len = ETHERMINTU;
	}

	/*
	 * Set up the transmit buffer and 
	 * start the transmission.
	 */
	ctlr->outpackets++;
	ring->len = len;
	ring->owner = Interface;
	ctlr->th = NEXT(ctlr->th, ctlr->ntb);
	(*ctlr->board->transmit)(ctlr);

	freeb(bp);
	qunlock(&ctlr->tlock);
	poperror();
}

/*
 * Open an ether line discipline.
 */
static void
etherstopen(Queue *q, Stream *s)
{
	Ctlr *ctlr = softctlr[0];
	Type *type;

	type = &ctlr->type[s->id];
	RD(q)->ptr = WR(q)->ptr = type;
	type->type = 0;
	type->q = RD(q);
	type->inuse = 1;
	type->ctlr = ctlr;
}

/*
 * Close ether line discipline.
 *
 * The locking is to synchronize changing the ethertype with
 * sending packets up the stream on interrupts.
 */
static int
isclosed(void *arg)
{
	return ((Type*)arg)->q == 0;
}

static void
etherstclose(Queue *q)
{
	Type *type = (Type*)(q->ptr);
	Ctlr *ctlr = type->ctlr;

	if(type->prom){
		qlock(ctlr);
		ctlr->prom--;
		if(ctlr->prom == 0)
			(*ctlr->board->mode)(ctlr, 0);
		qunlock(ctlr);
	}
	if(type->type == -1){
		qlock(ctlr);
		ctlr->all--;
		qunlock(ctlr);
	}

	/*
	 * Mark as closing and wait for kproc
	 * to close us.
	 */
	lock(&ctlr->clock);
	type->clist = ctlr->clist;
	ctlr->clist = type;
	unlock(&ctlr->clock);
	wakeup(&ctlr->rr);
	sleep(&type->cr, isclosed, type);

	type->type = 0;
	type->prom = 0;
	type->inuse = 0;
	netdisown(type);
	type->ctlr = 0;
}

static Qinfo info = {
	nullput,
	etheroput,
	etherstopen,
	etherstclose,
	"ether"
};

static int
clonecon(Chan *c)
{
	Ctlr *ctlr = softctlr[0];
	Type *type;

	USED(c);
	for(type = ctlr->type; type < &ctlr->type[NType]; type++){
		qlock(type);
		if(type->inuse || type->q){
			qunlock(type);
			continue;
		}
		type->inuse = 1;
		netown(type, u->p->user, 0);
		qunlock(type);
		return type - ctlr->type;
	}
	exhausted("ether channels");
	return 0;
}

static void
statsfill(Chan *c, char *p, int n)
{
	Ctlr *ctlr = softctlr[0];
	char buf[256];

	USED(c);
	sprint(buf, "in: %d\nout: %d\ncrc errs %d\noverflows: %d\nframe errs %d\nbuff errs: %d\noerrs %d\naddr: %.02x:%.02x:%.02x:%.02x:%.02x:%.02x\n",
		ctlr->inpackets, ctlr->outpackets, ctlr->crcs,
		ctlr->overflows, ctlr->frames, ctlr->buffs, ctlr->oerrs,
		ctlr->ea[0], ctlr->ea[1], ctlr->ea[2],
		ctlr->ea[3], ctlr->ea[4], ctlr->ea[5]);
	strncpy(p, buf, n);
}

static void
typefill(Chan *c, char *p, int n)
{
	char buf[16];
	Type *type;

	type = &softctlr[0]->type[STREAMID(c->qid.path)];
	sprint(buf, "%d", type->type);
	strncpy(p, buf, n);
}

static void
etherup(Ctlr *ctlr, Etherpkt *pkt, int len)
{
	int t;
	Type *type;
	Block *bp;

	t = (pkt->type[0]<<8)|pkt->type[1];
	for(type = &ctlr->type[0]; type < &ctlr->type[NType]; type++){

		/*
		 * Check for open, the right type, and flow control.
		 */
		if(type->q == 0)
			continue;
		if(t != type->type && type->type != -1)
			continue;
		if(type->q->next->len > Streamhi)
			continue;

		/*
		 * Only a trace channel gets packets destined for other machines.
		 */
		if(type->type != -1 && pkt->d[0] != 0xFF
		  && (*pkt->d != *ctlr->ea || memcmp(pkt->d, ctlr->ea, sizeof(pkt->d))))
			continue;

		if(waserror() == 0){
			bp = allocb(len);
			memmove(bp->rptr, pkt, len);
			bp->wptr += len;
			bp->flags |= S_DELIM;
			PUTNEXT(type->q, bp);
			poperror();
		}
	}
}

static int
isinput(void *arg)
{
	Ctlr *ctlr = arg;

	return ctlr->lbq.first || ctlr->rb[ctlr->rh].owner == Host || ctlr->clist;
}

static void
etherkproc(void *arg)
{
	Ctlr *ctlr = arg;
	RingBuf *ring;
	Block *bp;
	Type *type;

	if(waserror()){
		print("%s noted\n", ctlr->name);
		/* fix
		if(ctlr->board->reset)
			(*ctlr->board->reset)(ctlr);
		 */
		ctlr->kproc = 0;
		nexterror();
	}

	for(;;){
		tsleep(&ctlr->rr, isinput, ctlr, 500);
		if(ctlr->board->watch)
			(*ctlr->board->watch)(ctlr);

		/*
		 * Process any internal loopback packets.
		 */
		while(bp = getq(&ctlr->lbq)){
			ctlr->inpackets++;
			etherup(ctlr, (Etherpkt*)bp->rptr, BLEN(bp));
			freeb(bp);
		}

		/*
		 * Process any received packets.
		 */
		while(ctlr->rb[ctlr->rh].owner == Host){
			ctlr->inpackets++;
			ring = &ctlr->rb[ctlr->rh];
			etherup(ctlr, (Etherpkt*)ring->pkt, ring->len);
			ring->owner = Interface;
			ctlr->rh = NEXT(ctlr->rh, ctlr->nrb);
		}

		/*
		 * Close Types requesting it.
		 */
		if(ctlr->clist){
			lock(&ctlr->clock);
			for(type = ctlr->clist; type; type = type->clist){
				type->q = 0;
				wakeup(&type->cr);
			}
			ctlr->clist = 0;
			unlock(&ctlr->clock);
		}
	}
}

static void
etherintr(Ureg *ur)
{
	Ctlr *ctlr = softctlr[0];

	USED(ur);
	(*ctlr->board->intr)(ctlr);
}

void
etherreset(void)
{
	Ctlr *ctlr;
	Board **board;
	int i;

	if(softctlr[nctlr] == 0)
		softctlr[nctlr] = xalloc(sizeof(Ctlr));
	ctlr = softctlr[nctlr];
	for(board = boards; *board; board++){
		ctlr->board = *board;
		if((*ctlr->board->reset)(ctlr) == 0){
			ctlr->present = 1;

			/*
			 * IRQ2 doesn't really exist, it's used to gang the interrupt
			 * controllers together. A device set to IRQ2 will appear on
			 * the second interrupt controller as IRQ9.
			 */
			if(ctlr->board->irq == 2)
				ctlr->board->irq = 9;
			setvec(Int0vec + ctlr->board->irq, etherintr);
			break;
		}
	}
	if(ctlr->present == 0)
		return;
	nctlr++;

	if(ctlr->nrb == 0)
		ctlr->nrb = Nrb;
	ctlr->rb = xalloc(sizeof(RingBuf)*ctlr->nrb);
	if(ctlr->ntb == 0)
		ctlr->ntb = Ntb;
	ctlr->tb = xalloc(sizeof(RingBuf)*ctlr->ntb);

	memset(ctlr->ba, 0xFF, sizeof(ctlr->ba));

	ctlr->net.name = "ether";
	ctlr->net.nconv = NType;
	ctlr->net.devp = &info;
	ctlr->net.protop = 0;
	ctlr->net.listen = 0;
	ctlr->net.clone = clonecon;
	ctlr->net.ninfo = 2;
	ctlr->net.info[0].name = "stats";
	ctlr->net.info[0].fill = statsfill;
	ctlr->net.info[1].name = "type";
	ctlr->net.info[1].fill = typefill;
	for(i = 0; i < NType; i++)
		netadd(&ctlr->net, &ctlr->type[i], i);
}

void
etherinit(void)
{
	int ctlrno = 0;
	Ctlr *ctlr = softctlr[ctlrno];
	int i;

	if(ctlr->present == 0)
		return;

	ctlr->rh = 0;
	ctlr->ri = 0;
	for(i = 0; i < ctlr->nrb; i++)
		ctlr->rb[i].owner = Interface;

	ctlr->th = 0;
	ctlr->ti = 0;
	for(i = 0; i < ctlr->ntb; i++)
		ctlr->tb[i].owner = Host;
}

Chan*
etherattach(char *spec)
{
	int ctlrno = 0;
	Ctlr *ctlr = softctlr[ctlrno];

	if(ctlr->present == 0)
		error(Enodev);

	/*
	 * Enable the interface
	 * and start the kproc.
	 */	
	(*ctlr->board->attach)(ctlr);
	if(ctlr->kproc == 0){
		sprint(ctlr->name, "ether%dkproc", ctlrno);
		ctlr->kproc = 1;
		kproc(ctlr->name, etherkproc, ctlr);
	}
	return devattach('l', spec);
}
