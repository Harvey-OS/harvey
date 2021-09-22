/*
 * Stripped down virtnet ether device for tinyemu
 *
 * Assume only one interface for now.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"

#include "../port/netif.h"
#include "etherif.h"

#define VIRTNETADDR	0x40011000

typedef struct Ctlr Ctlr;
typedef struct Vhdr Vhdr;
typedef struct Vdesc Vdesc;
typedef struct Vavail Vavail;
typedef struct Vused Vused;
typedef struct Vqueue Vqueue;

enum {
	/* virtio mmio register offsets */
	MagicValue	= 0x00,
		MagicVirt	= 0x74726976,	/* "virt" little-endian */
	DeviceId	= 0x08,
		DeviceNet	= 1,
	QueueSel	= 0x30,
	QueueNumMax	= 0x34,
	QueueNum	= 0x38,
	QueueReady	= 0x44,
	QueueNotify	= 0x50,
	IntStatus	= 0x60,
		UsedUpdate	= 1<<0,
	IntAck		= 0x64,
	Status		= 0x70,		/* writing 0 resets the hw */
	QueueDesc	= 0x80,
	QueueAvail	= 0x90,		/* aka QueueDriver */
	QueueUsed	= 0xA0,		/* aka QueueDevice */
	Config		= 0x100,
		MacAddr		= 0,

	/* Vdesc flags */
	Next		= 1<<0,
	Descwr		= 1<<1,

	Recv	= 0,
	Xmit	= 1,
	Nqueue	= 2,
};

struct Vdesc			/* QueueSize array */
{
	u64int	addr;
	u32int	len;
	u16int	flags;
	u16int	next;
};

struct Vavail
{
	u16int	flags;
	u16int	idx;
	u16int	ring[1];	/* QueueSize elements */
};

typedef struct Vuseddesc Vuseddesc;
struct Vuseddesc {
	u32int	id;
	u32int	len;
};

/* must be 4096-byte aligned? */
struct Vused
{
	u16int	flags;
	u16int	idx;
	u32int	ring[1][2];		/* QueueSize elements */
};
enum { Id = 0, Len = 1 };

struct Vqueue {
	uint	mask;
	uint	lastused;
	Block	*bfirst;
	Block	*blast;
	Vdesc	*desc;
	Vavail	*avail;
	Vused	*used;
};

struct Ctlr {
	Ether	*edev;
	u32int	*mmio;
	uchar	eaddr[Eaddrlen];
	Vqueue	q[Nqueue];
	QLock	alock;
	Rendez	rendez;
	int	attached;
	int	interrupted;

	/* stats */
	uint	rcved;
	uint	xmited;
};

#define	RD(off)		(ctlr->mmio[(off)>>2])
#define WR(off, v)	ctlr->mmio[(off)>>2] = v

static Ctlr virtnet;

static void	temushutdown(Ether *edev);

/* n must be a power of 2 */
static void
vqinit(Vqueue *q, int n)
{
	int d, a, u;
	char *p;

	if (q == nil)
		panic("vqinit: nil q");
	if (n == 0)
		panic("vqinit: zero n");
	if (!ispow2(n))
		print("vqinit: q n %d not power of 2\n", n);
	q->mask = n - 1;
	d = 16*n;
	/* spec says 6+ for both of these, but 4+ works. */
	a = 4 + 2*n;
	u = 4 + 8*n;
	p = mallocalign(d + a + u, 16, 0, 0);
//	memset(p, 0, d + a + u);	/* unneeded; kernel malloc does it */
	q->desc = (Vdesc*)p;
	q->avail = (Vavail*)(p + d);
	/* used ring is supposed to be aligned to a multiple of 4 or 4k? */
	q->used = (Vused*)(p + d + a);
}

static void
venq(Vqueue *q, Block *b, int write)
{
	int idx, ringidx;
	Vdesc *desc;

	if(q->bfirst)
		q->blast->next = b;
	else
		q->bfirst = b;
	q->blast = b;
	idx = q->avail->idx;
	ringidx = idx & q->mask;
	desc = &q->desc[ringidx];
	desc->addr = PADDR(write? b->wp: b->rp);
	desc->len = write ? b->lim - b->wp : BLEN(b);
	desc->flags = write;
	desc->next = 0;
	q->avail->ring[ringidx] = ringidx;
	q->avail->idx = idx + 1;
}

static Block*
vdeq(Vqueue *q)
{
	Block *b;

	b = q->bfirst;
	if(b == nil)
		return b;
	q->bfirst = b->next;
	b->next = nil;
	return b;
}

static void
vbufinit(Vqueue *q, int n)
{
	int i;
	Block *b;

	for(i = 0; i < n; i++){
		b = allocb(1600);
		venq(q, b, Descwr);
	}
}

void
dump(uchar *p, int len)
{
	while(len-- > 0)
		print(" %2.2ux", *p++);
	print("\n");
}

static void
temuinterrupt(Ureg*, void *arg)
{
	Ctlr *ctlr;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;
	if(ctlr == nil)
		panic("ethertemu: interrupt before init");
	wakeup(&ctlr->rendez);
	WR(IntAck, RD(IntStatus));
}

static int
interrupted(void *arg)
{
	Ctlr *ctlr;
	Vqueue *q;

	ctlr = arg;
	q = &ctlr->q[Recv];
	if(q->used->idx != q->lastused)
		return 1;
	q = &ctlr->q[Xmit];
	if(q->used->idx != q->lastused)
		return 1;
	return 0;
}

/* watch for ring and output queue changes */
static void
ethertemuproc(void *arg)
{
	Ether *edev;
	Ctlr *ctlr;
	Vqueue *q;
	Block *bp;
	int i, n, found;

	edev = arg;
	ctlr = edev->ctlr;
	assert(ctlr);
	for(;;){
		found = 0;

		q = &ctlr->q[Recv];
		while(q->used->idx != q->lastused){
			i = q->used->ring[q->lastused & q->mask][Id];
			n = q->used->ring[q->lastused & q->mask][Len];
			bp = vdeq(q);
			if(bp == nil)
				panic("vnet nil recv block");
			if(PADDR(bp->wp) != q->desc[i].addr)
				panic("vnet recv queue out of sync wp %#p addr %#llux",
					PADDR(bp->wp), q->desc[i].addr);
			bp->wp += n;
			bp->rp += 12;	/* skip pointless header */
			etheriq(edev, bp, 1);
			ctlr->rcved++;
			bp = allocb(1600);
			if(bp == nil)
				panic("vnet can't alloc recv buffer");
			venq(q, bp, Descwr);
			found = 1;
			q->lastused++;
		}
		if(found)
			WR(QueueNotify, Recv);

		q = &ctlr->q[Xmit];
		while(q->used->idx != q->lastused){
			i = q->used->ring[q->lastused & q->mask][Id];
			bp = vdeq(q);
			if(bp == nil)
				panic("vnet nil xmit block");
			if(PADDR(bp->rp) != q->desc[i].addr)
				panic("vnet xmit queue out of sync wp %#p addr %#llux",
					PADDR(bp->wp), q->desc[i].addr);
			freeb(bp);
			found = 1;
			q->lastused++;
		}
		if(!found){
			sleep(&ctlr->rendez, interrupted, ctlr);
		}
	}
}

static void
temutransmit(Ether *edev)
{
	Ctlr *ctlr;
	Vqueue *q;
	Block *bp;

	ctlr = edev->ctlr;
	if(ctlr == nil)
		panic("ethertemu: transmit before init");
	q = &ctlr->q[Xmit];
	for(;;){
		/* XXX check for ring full */
		bp = qget(edev->oq);
		if(bp == nil)
			break;
		bp = padblock(bp, 12);	/* pointless virtnet header */
		venq(q, bp, 0);
		WR(QueueNotify, Xmit);
//		wakeup(&ctlr->rendez);
	}
}

static void
temuattach(Ether *edev)
{
	Ctlr *ctlr;
	char name[KNAMELEN];
	Vqueue *q;
	int i, n;

	ctlr = edev->ctlr;
	qlock(&ctlr->alock);

	if(ctlr->attached){
		qunlock(&ctlr->alock);
		return;
	}

	WR(Status, 0);				/* reset */
	for(i = 0; i < Nqueue; i++){
		q = &ctlr->q[i];
		WR(QueueSel, i);
		n = RD(QueueNum);
		vqinit(q, n);
		if(i == Recv)
			vbufinit(q, n);
		WR(QueueDesc, PADDR(q->desc));
		WR(QueueDesc + 4, 0);
		WR(QueueAvail, PADDR(q->avail));
		WR(QueueAvail + 4, 0);
		WR(QueueUsed, PADDR(q->used));
		WR(QueueUsed + 4, 0);
		WR(QueueReady, 1);
	}
	ctlr->attached = 1;
	snprint(name, sizeof name, "#l%d", edev->ctlrno);
	kproc(name, ethertemuproc, edev);
	qunlock(&ctlr->alock);
	WR(QueueNotify, Recv);
}

static void
temushutdown(Ether *edev)
{
	int i;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	WR(Status, 0);				/* reset */
	for(i = 0; i < Nqueue; i++){
		WR(QueueSel, i);
		WR(QueueDesc, 0);
		WR(QueueDesc + 4, 0);
		WR(QueueAvail, 0);
		WR(QueueAvail + 4, 0);
		WR(QueueUsed, 0);
		WR(QueueUsed + 4, 0);
	}
	delay(10);				/* let it settle */
}

static long
temuifstat(Ether *edev, void* a, long n, ulong offset)
{
	char buf[128];
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	snprint(buf, sizeof buf, "rcv %d\nxmit %d\n",
		ctlr->rcved, ctlr->xmited);
	return readstr(offset, a, n, buf);
}

static void
temuprom(void *, int)
{
}

static void
temumcast(void *, uchar *, int)
{
}

static int
pnp(Ether *edev)
{
	Ctlr *ctlr;
	int i;

	ctlr = &virtnet;
	if(ctlr->mmio != 0)
		return -1;
	ctlr->mmio = vmap(VIRTNETADDR, 0x1000);
	if(ctlr->mmio == 0){
		print("ethermtu: can't vmap device\n");
		return -1;
	}

	if(RD(MagicValue) != MagicVirt){
		print("ethertemu: virtio device not found at %#p, magic word is %ux\n",
			(u32int*)VIRTNETADDR, RD(MagicValue));
		return -1;
	}
	if(RD(DeviceId) != DeviceNet){
		print("ethertemu: virtio net not found at %#p, device id is %ux\n",
			(u32int*)VIRTNETADDR, RD(DeviceId));
		return -1;
	}
	for(i = 0; i < Eaddrlen; i++)
		ctlr->eaddr[i] = ((uchar*)ctlr->mmio)[Config+i];

	edev->ctlr = ctlr;
	ctlr->edev = edev;
	edev->port = VIRTNETADDR;	/* physical is more useful */
	edev->irq = ioconf("ether", 0)->irq;
	edev->mbps = 100;
	edev->maxmtu = ETHERMAXTU;
	memmove(edev->ea, ctlr->eaddr, Eaddrlen);

	edev->attach = temuattach;
	edev->interrupt = temuinterrupt;
	edev->ifstat = temuifstat;
	edev->transmit = temutransmit;
	edev->shutdown = temushutdown;
	edev->promiscuous = temuprom;
	edev->multicast = temumcast;		/* enables ipv6 */
	return 0;
}

void
ethertemulink(void)
{
	addethercard("tinyemu", pnp);
}
