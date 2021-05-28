#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"
#include "etherif.h"

/*
 * virtio ethernet driver
 * http://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.html
 */

typedef struct Vring Vring;
typedef struct Vdesc Vdesc;
typedef struct Vused Vused;
typedef struct Vheader Vheader;
typedef struct Vqueue Vqueue;
typedef struct Ctlr Ctlr;

enum {
	/* §2.1 Device Status Field */
	Sacknowledge = 1,
	Sdriver = 2,
	Sdriverok = 4,
	Sfeatureok = 8,
	Sfailed = 128,

	/* §4.1.4.8 Legacy Interfaces: A Note on PCI Device Layout */
	Qdevfeat = 0,
	Qdrvfeat = 4,
	Qaddr = 8,
	Qsize = 12,
	Qselect = 14,
	Qnotify = 16,
	Qstatus = 18,
	Qisr = 19,
	Qmac = 20,
	Qnetstatus = 26,

	/* flags in Qnetstatus */
	Nlinkup = (1<<0),
	Nannounce = (1<<1),

	/* feature bits */
	Fmac = (1<<5),
	Fstatus = (1<<16),
	Fctrlvq = (1<<17),
	Fctrlrx = (1<<18),

	/* vring used flags */
	Unonotify = 1,
	/* vring avail flags */
	Rnointerrupt = 1,

	/* descriptor flags */
	Dnext = 1,
	Dwrite = 2,
	Dindirect = 4,

	/* struct sizes */
	VringSize = 4,
	VdescSize = 16,
	VusedSize = 8,
	VheaderSize = 10,

	/* §4.1.5.1.4.1 says pages are 4096 bytes
	 * for the purposes of the driver.
	 */
	VBY2PG	= 4096,
#define VPGROUND(s)	ROUNDUP(s, VBY2PG)

	Vrxq	= 0,
	Vtxq	= 1,
	Vctlq	= 2,

	/* class/cmd for Vctlq */
	CtrlRx	= 0x00,
		CmdPromisc	= 0x00,
		CmdAllmulti	= 0x01,
	CtrlMac	= 0x01,
		CmdMacTableSet	= 0x00,
	CtrlVlan= 0x02,
		CmdVlanAdd	= 0x00,
		CmdVlanDel	= 0x01,
};

struct Vring
{
	u16int	flags;
	u16int	idx;
};

struct Vdesc
{
	u64int	addr;
	u32int	len;
	u16int	flags;
	u16int	next;
};

struct Vused
{
	u32int	id;
	u32int	len;
};

struct Vheader
{
	u8int	flags;
	u8int	segtype;
	u16int	hlen;
	u16int	seglen;
	u16int	csumstart;
	u16int	csumend;
};

/* §2.4 Virtqueues */
struct Vqueue
{
	Rendez;

	uint	qsize;
	uint	qmask;

	Vdesc	*desc;

	Vring	*avail;
	u16int	*availent;
	u16int	*availevent;

	Vring	*used;
	Vused	*usedent;
	u16int	*usedevent;
	u16int	lastused;

	uint	nintr;
	uint	nnote;
};

struct Ctlr {
	Lock;

	QLock	ctllock;

	int	attached;

	int	port;
	Pcidev	*pcidev;
	Ctlr	*next;
	int	active;
	int	id;
	int	typ;
	ulong	feat;
	int	nqueue;

	/* virtioether has 3 queues: rx, tx and ctl */
	Vqueue	queue[3];
};

static Ctlr *ctlrhead;

static int
vhasroom(void *v)
{
	Vqueue *q = v;
	return q->lastused != q->used->idx;
}

static void
vqnotify(Ctlr *ctlr, int x)
{
	Vqueue *q;

	coherence();
	q = &ctlr->queue[x];
	if(q->used->flags & Unonotify)
		return;
	q->nnote++;
	outs(ctlr->port+Qnotify, x);
}

static void
txproc(void *v)
{
	Vheader *header;
	Block **blocks;
	Ether *edev;
	Ctlr *ctlr;
	Vqueue *q;
	Vused *u;
	Block *b;
	int i, j;

	edev = v;
	ctlr = edev->ctlr;
	q = &ctlr->queue[Vtxq];

	header = smalloc(VheaderSize);
	blocks = smalloc(sizeof(Block*) * (q->qsize/2));

	for(i = 0; i < q->qsize/2; i++){
		j = i << 1;
		q->desc[j].addr = PADDR(header);
		q->desc[j].len = VheaderSize;
		q->desc[j].next = j | 1;
		q->desc[j].flags = Dnext;

		q->availent[i] = q->availent[i + q->qsize/2] = j;

		j |= 1;
		q->desc[j].next = 0;
		q->desc[j].flags = 0;
	}

	q->avail->flags &= ~Rnointerrupt;

	while(waserror())
		;

	while((b = qbread(edev->oq, 1000000)) != nil){
		for(;;){
			/* retire completed packets */
			while((i = q->lastused) != q->used->idx){
				u = &q->usedent[i & q->qmask];
				i = (u->id & q->qmask) >> 1;
				if(blocks[i] == nil)
					break;
				freeb(blocks[i]);
				blocks[i] = nil;
				q->lastused++;
			}

			/* have free slot? */
			i = q->avail->idx & (q->qmask >> 1);
			if(blocks[i] == nil)
				break;

			/* ring full, wait and retry */
			if(!vhasroom(q))
				sleep(q, vhasroom, q);
		}

		/* slot is free, fill in descriptor */
		blocks[i] = b;
		j = (i << 1) | 1;
		q->desc[j].addr = PADDR(b->rp);
		q->desc[j].len = BLEN(b);
		coherence();
		q->avail->idx++;
		vqnotify(ctlr, Vtxq);
	}

	pexit("ether out queue closed", 1);
}

static void
rxproc(void *v)
{
	Vheader *header;
	Block **blocks;
	Ether *edev;
	Ctlr *ctlr;
	Vqueue *q;
	Vused *u;
	Block *b;
	int i, j;

	edev = v;
	ctlr = edev->ctlr;
	q = &ctlr->queue[Vrxq];

	header = smalloc(VheaderSize);
	blocks = smalloc(sizeof(Block*) * (q->qsize/2));

	for(i = 0; i < q->qsize/2; i++){
		j = i << 1;
		q->desc[j].addr = PADDR(header);
		q->desc[j].len = VheaderSize;
		q->desc[j].next = j | 1;
		q->desc[j].flags = Dwrite|Dnext;

		q->availent[i] = q->availent[i + q->qsize/2] = j;

		j |= 1;
		q->desc[j].next = 0;
		q->desc[j].flags = Dwrite;
	}

	q->avail->flags &= ~Rnointerrupt;

	while(waserror())
		;

	for(;;){
		/* replenish receive ring */
		do {
			i = q->avail->idx & (q->qmask >> 1);
			if(blocks[i] != nil)
				break;
			if((b = iallocb(ETHERMAXTU)) == nil)
				break;
			blocks[i] = b;
			j = (i << 1) | 1;
			q->desc[j].addr = PADDR(b->rp);
			q->desc[j].len = BALLOC(b);
			coherence();
			q->avail->idx++;
		} while(q->avail->idx != q->used->idx);
		vqnotify(ctlr, Vrxq);

		/* wait for any packets to complete */
		if(!vhasroom(q))
			sleep(q, vhasroom, q);

		/* retire completed packets */
		while((i = q->lastused) != q->used->idx) {
			u = &q->usedent[i & q->qmask];
			i = (u->id & q->qmask) >> 1;
			if((b = blocks[i]) == nil)
				break;

			blocks[i] = nil;

			b->wp = b->rp + u->len - VheaderSize;
			etheriq(edev, b, 1);
			q->lastused++;
		}
	}
}

static int
vctlcmd(Ether *edev, uchar class, uchar cmd, uchar *data, int ndata)
{
	uchar hdr[2], ack[1];
	Ctlr *ctlr;
	Vqueue *q;
	Vdesc *d;
	int i;

	ctlr = edev->ctlr;
	q = &ctlr->queue[Vctlq];
	if(q->qsize < 3)
		return -1;

	qlock(&ctlr->ctllock);
	while(waserror())
		;

	ack[0] = 0x55;
	hdr[0] = class;
	hdr[1] = cmd;

	d = &q->desc[0];
	d->addr = PADDR(hdr);
	d->len = sizeof(hdr);
	d->next = 1;
	d->flags = Dnext;
	d++;
	d->addr = PADDR(data);
	d->len = ndata;
	d->next = 2;
	d->flags = Dnext;
	d++;
	d->addr = PADDR(ack);
	d->len = sizeof(ack);
	d->next = 0;
	d->flags = Dwrite;

	i = q->avail->idx & q->qmask;
	q->availent[i] = 0;
	coherence();

	q->avail->flags &= ~Rnointerrupt;
	q->avail->idx++;
	vqnotify(ctlr, Vctlq);
	while(!vhasroom(q))
		sleep(q, vhasroom, q);
	q->lastused = q->used->idx;
	q->avail->flags |= Rnointerrupt;

	qunlock(&ctlr->ctllock);
	poperror();

	if(ack[0] != 0)
		print("#l%d: vctlcmd: %ux.%ux -> %ux\n", edev->ctlrno, class, cmd, ack[0]);

	return ack[0];
}

static void
interrupt(Ureg*, void* arg)
{
	Ether *edev;
	Ctlr *ctlr;
	Vqueue *q;
	int i;

	edev = arg;
	ctlr = edev->ctlr;
	if(inb(ctlr->port+Qisr) & 1){
		for(i = 0; i < ctlr->nqueue; i++){
			q = &ctlr->queue[i];
			if(vhasroom(q)){
				q->nintr++;
				wakeup(q);
			}
		}
	}
}

static void
attach(Ether* edev)
{
	char name[KNAMELEN];
	Ctlr* ctlr;

	ctlr = edev->ctlr;
	lock(ctlr);
	if(ctlr->attached){
		unlock(ctlr);
		return;
	}
	ctlr->attached = 1;
	unlock(ctlr);

	/* ready to go */
	outb(ctlr->port+Qstatus, inb(ctlr->port+Qstatus) | Sdriverok);

	/* start kprocs */
	snprint(name, sizeof name, "#l%drx", edev->ctlrno);
	kproc(name, rxproc, edev);
	snprint(name, sizeof name, "#l%dtx", edev->ctlrno);
	kproc(name, txproc, edev);
}

static long
ifstat(Ether *edev, void *a, long n, ulong offset)
{
	int i, l;
	char *p;
	Ctlr *ctlr;
	Vqueue *q;

	ctlr = edev->ctlr;

	p = smalloc(READSTR);

	l = snprint(p, READSTR, "devfeat %4.4luX\n", ctlr->feat);
	l += snprint(p+l, READSTR-l, "drvfeat %4.4luX\n", inl(ctlr->port+Qdrvfeat));
	l += snprint(p+l, READSTR-l, "devstatus %uX\n", inb(ctlr->port+Qstatus));
	if(ctlr->feat & Fstatus)
		l += snprint(p+l, READSTR-l, "netstatus %uX\n",  inb(ctlr->port+Qnetstatus));

	for(i = 0; i < ctlr->nqueue; i++){
		q = &ctlr->queue[i];
		l += snprint(p+l, READSTR-l,
			"vq%d %#p size %d avail->idx %d used->idx %d lastused %hud nintr %ud nnote %ud\n",
			i, q, q->qsize, q->avail->idx, q->used->idx, q->lastused, q->nintr, q->nnote);
	}

	n = readstr(offset, a, n, p);
	free(p);

	return n;
}

static void
shutdown(Ether* edev)
{
	Ctlr *ctlr = edev->ctlr;
	outb(ctlr->port+Qstatus, 0);
	pciclrbme(ctlr->pcidev);
}

static void
promiscuous(void *arg, int on)
{
	Ether *edev = arg;
	uchar b[1];

	b[0] = on != 0;
	vctlcmd(edev, CtrlRx, CmdPromisc, b, sizeof(b));
}

static void
multicast(void *arg, uchar*, int)
{
	Ether *edev = arg;
	uchar b[1];

	b[0] = edev->nmaddr > 0;
	vctlcmd(edev, CtrlRx, CmdAllmulti, b, sizeof(b));
}

/* §2.4.2 Legacy Interfaces: A Note on Virtqueue Layout */
static ulong
queuesize(ulong size)
{
	return VPGROUND(VdescSize*size + sizeof(u16int)*(3+size))
		+ VPGROUND(sizeof(u16int)*3 + VusedSize*size);
}

static int
initqueue(Vqueue *q, int size)
{
	uchar *p;

	/* §2.4: Queue Size value is always a power of 2 and <= 32768 */
	assert(!(size & (size - 1)) && size <= 32768);

	p = mallocalign(queuesize(size), VBY2PG, 0, 0);
	if(p == nil){
		print("ethervirtio: no memory for Vqueue\n");
		free(p);
		return -1;
	}

	q->desc = (void*)p;
	p += VdescSize*size;
	q->avail = (void*)p;
	p += VringSize;
	q->availent = (void*)p;
	p += sizeof(u16int)*size;
	q->availevent = (void*)p;
	p += sizeof(u16int);

	p = (uchar*)VPGROUND((uintptr)p);
	q->used = (void*)p;
	p += VringSize;
	q->usedent = (void*)p;
	p += VusedSize*size;
	q->usedevent = (void*)p;

	q->qsize = size;
	q->qmask = q->qsize - 1;

	q->lastused = q->avail->idx = q->used->idx = 0;

	q->avail->flags |= Rnointerrupt;

	return 0;
}

static Ctlr*
pciprobe(int typ)
{
	Ctlr *c, *h, *t;
	Pcidev *p;
	int n, i;

	h = t = nil;

	/* §4.1.2 PCI Device Discovery */
	for(p = nil; p = pcimatch(p, 0, 0);){
		if(p->vid != 0x1AF4)
			continue;
		/* the two possible DIDs for virtio-net */
		if(p->did != 0x1000 && p->did != 0x1041)
			continue;
		/* non-transitional devices will have a revision > 0 */
		if(p->rid != 0)
			continue;
		/* first membar needs to be I/O */
		if((p->mem[0].bar & 1) == 0)
			continue;
		/* non-transitional device will have typ+0x40 */
		if(pcicfgr16(p, 0x2E) != typ)
			continue;
		if((c = mallocz(sizeof(Ctlr), 1)) == nil){
			print("ethervirtio: no memory for Ctlr\n");
			break;
		}
		c->port = p->mem[0].bar & ~3;
		if(ioalloc(c->port, p->mem[0].size, 0, "ethervirtio") < 0){
			print("ethervirtio: port %ux in use\n", c->port);
			free(c);
			continue;
		}

		c->typ = typ;
		c->pcidev = p;
		c->id = (p->did<<16)|p->vid;

		/* §3.1.2 Legacy Device Initialization */
		outb(c->port+Qstatus, 0);
		outb(c->port+Qstatus, Sacknowledge|Sdriver);

		/* negotiate feature bits */
		c->feat = inl(c->port+Qdevfeat);
		outl(c->port+Qdrvfeat, c->feat & (Fmac|Fstatus|Fctrlvq|Fctrlrx));

		/* §4.1.5.1.4 Virtqueue Configuration */
		for(i=0; i<nelem(c->queue); i++){
			outs(c->port+Qselect, i);
			n = ins(c->port+Qsize);
			if(n == 0 || (n & (n-1)) != 0){
				if(i < 2)
					print("ethervirtio: queue %d has invalid size %d\n", i, n);
				break;
			}
			if(initqueue(&c->queue[i], n) < 0)
				break;
			coherence();
			outl(c->port+Qaddr, PADDR(c->queue[i].desc)/VBY2PG);
		}
		if(i < 2){
			print("ethervirtio: no queues\n");
			free(c);
			continue;
		}
		c->nqueue = i;		
	
		if(h == nil)
			h = c;
		else
			t->next = c;
		t = c;
	}

	return h;
}


static int
reset(Ether* edev)
{
	static uchar zeros[Eaddrlen];
	Ctlr *ctlr;
	int i;

	if(ctlrhead == nil)
		ctlrhead = pciprobe(1);

	for(ctlr = ctlrhead; ctlr != nil; ctlr = ctlr->next){
		if(ctlr->active)
			continue;
		if(edev->port == 0 || edev->port == ctlr->port){
			ctlr->active = 1;
			break;
		}
	}

	if(ctlr == nil)
		return -1;

	edev->ctlr = ctlr;
	edev->port = ctlr->port;
	edev->irq = ctlr->pcidev->intl;
	edev->tbdf = ctlr->pcidev->tbdf;
	edev->mbps = 1000;
	edev->link = 1;

	if((ctlr->feat & Fmac) != 0 && memcmp(edev->ea, zeros, Eaddrlen) == 0){
		for(i = 0; i < Eaddrlen; i++)
			edev->ea[i] = inb(ctlr->port+Qmac+i);
	} else {
		for(i = 0; i < Eaddrlen; i++)
			outb(ctlr->port+Qmac+i, edev->ea[i]);
	}

	edev->arg = edev;

	edev->attach = attach;
	edev->shutdown = shutdown;
	edev->ifstat = ifstat;

	if((ctlr->feat & (Fctrlvq|Fctrlrx)) == (Fctrlvq|Fctrlrx)){
		edev->multicast = multicast;
		edev->promiscuous = promiscuous;
	}

	pcisetbme(ctlr->pcidev);
	intrenable(edev->irq, interrupt, edev, edev->tbdf, edev->name);

	return 0;
}

void
ethervirtiolink(void)
{
	addethercard("virtio", reset);
}

