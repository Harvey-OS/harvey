#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"

#include "../port/sd.h"

typedef struct Vring Vring;
typedef struct Vdesc Vdesc;
typedef struct Vused Vused;
typedef struct Vqueue Vqueue;
typedef struct Vdev Vdev;

typedef struct ScsiCfg ScsiCfg;

/* device types */
enum {
	TypBlk	= 2,
	TypSCSI	= 8,
};

/* status flags */
enum {
	Acknowledge = 1,
	Driver = 2,
	DriverOk = 4,
	Failed = 0x80,
};

/* virtio ports */
enum {
	Devfeat = 0,
	Drvfeat = 4,
	Qaddr = 8,
	Qsize = 12,
	Qselect = 14,
	Qnotify = 16,
	Status = 18,
	Isr = 19,

	Devspec = 20,
};

/* descriptor flags */
enum {
	_Next = 1,
	Write = 2,
	Indirect = 4,
};

/* struct sizes */
enum {
	VringSize = 4,
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

struct Vqueue
{
	Lock;

	Vdev	*dev;
	int	idx;

	int	size;

	int	free;
	int	nfree;

	Vdesc	*desc;

	Vring	*avail;
	u16int	*availent;
	u16int	*availevent;

	Vring	*used;
	Vused	*usedent;
	u16int	*usedevent;
	u16int	lastused;

	void	*rock[];
};

struct Vdev
{
	int	typ;

	Pcidev	*pci;
	void*	vector;

	ulong	port;
	ulong	feat;

	int	nqueue;
	Vqueue	*queue[16];

	void	*cfg;	/* device specific config (for scsi) */

	Vdev	*next;
};

enum {
	CDBSIZE		= 32,
	SENSESIZE	= 96,
};

struct ScsiCfg
{
	u32int	num_queues;
	u32int	seg_max;
	u32int	max_sectors;
	u32int	cmd_per_lun;
	u32int	event_info_size;
	u32int	sense_size;
	u32int	cdb_size;
	u16int	max_channel;
	u16int	max_target;
	u32int	max_lun;
};

static Vqueue*
mkvqueue(int size)
{
	Vqueue *q;
	uchar *p;
	int i;

	q = malloc(sizeof(*q) + sizeof(void*)*size);
	p = mallocalign(
		ROUNDUP(sizeof(Vdesc)*size +
			VringSize +
			sizeof(u16int)*size +
			sizeof(u16int),
			4*KiB) +
		ROUNDUP(VringSize +
			sizeof(Vused)*size +
			sizeof(u16int),
			4*KiB),
		PGSZ, 0, 0);
	if(p == nil || q == nil){
		print("virtio: no memory for Vqueue\n");
		free(p);
		free(q);
		return nil;
	}

	q->desc = (void*)p;
	p += sizeof(Vdesc)*size;
	q->avail = (void*)p;
	p += VringSize;
	q->availent = (void*)p;
	p += sizeof(u16int)*size;
	q->availevent = (void*)p;
	p += sizeof(u16int);

	p = (uchar*)ROUNDUP((uintptr)p, 4*KiB);
	q->used = (void*)p;
	p += VringSize;
	q->usedent = (void*)p;
	p += sizeof(Vused)*size;
	q->usedevent = (void*)p;

	q->free = -1;
	q->nfree = q->size = size;
	for(i=0; i<size; i++){
		q->desc[i].next = q->free;
		q->free = i;
	}

	return q;
}

static Vdev*
viopnpdevs(int typ)
{
	Vdev *vd, *h, *t;
	Vqueue *q;
	Pcidev *p;
	int n, i, size;

	h = t = nil;
	for(p = nil; p = pcimatch(p, 0x1AF4, 0);){
		if((p->did < 0x1000) || (p->did > 0x103F))
			continue;
		if(p->rid != 0)
			continue;
		//if((p->mem[0].bar & 1) == 0) /* zero on gce */
		//	continue;
		if(pcicfgr16(p, 0x2E) != typ)
			continue;
		if((vd = malloc(sizeof(*vd))) == nil){
			print("virtio: no memory for Vdev\n");
			break;
		}
		vd->port = p->mem[0].bar & ~3;
		size = p->mem[0].size;
		if(vd->port == 0){ /* gce */
			vd->port = 0xc000;
			size = 0x40;
		}
		if(ioalloc(vd->port, size, 0, "virtio") < 0){
			print("virtio: port %lux in use\n", vd->port);
			free(vd);
			continue;
		}
		vd->typ = typ;
		vd->pci = p;

		/* reset */
		outb(vd->port+Status, 0);

		vd->feat = inl(vd->port+Devfeat);
		outb(vd->port+Status, Acknowledge|Driver);
		for(i=0; i<nelem(vd->queue); i++){
			outs(vd->port+Qselect, i);
			n = ins(vd->port+Qsize);
			if(n == 0 || (n & (n-1)) != 0)
				break;
			if((q = mkvqueue(n)) == nil)
				break;
			q->dev = vd;
			q->idx = i;
			vd->queue[i] = q;
			coherence();
			outl(vd->port+Qaddr, PADDR(vd->queue[i]->desc)/PGSZ);
		}
		vd->nqueue = i;

		if(h == nil)
			h = vd;
		else
			t->next = vd;
		t = vd;
	}

	return h;
}

struct Rock {
	int done;
	Rendez *sleep;
};

static void
vqinterrupt(Vqueue *q)
{
	int id, free, m;
	struct Rock *r;
	Rendez *z;

	m = q->size-1;

	ilock(q);
	while((q->lastused ^ q->used->idx) & m){
		id = q->usedent[q->lastused++ & m].id;
		if(r = q->rock[id]){
			q->rock[id] = nil;
			z = r->sleep;
			r->done = 1;	/* hands off */
			if(z != nil)
				wakeup(z);
		}
		do {
			free = id;
			id = q->desc[free].next;
			q->desc[free].next = q->free;
			q->free = free;
			q->nfree++;
		} while(q->desc[free].flags & _Next);
	}
	iunlock(q);
}

static void
viointerrupt(Ureg *, void *arg)
{
	Vdev *vd = arg;

	if(inb(vd->port+Isr) & 1)
		vqinterrupt(vd->queue[vd->typ == TypSCSI ? 2 : 0]);
}

static int
viodone(void *arg)
{
	return ((struct Rock*)arg)->done;
}

static void
vqio(Vqueue *q, int head)
{
	struct Rock rock;

	rock.done = 0;
	rock.sleep = &up->sleep;
	q->rock[head] = &rock;
	q->availent[q->avail->idx & (q->size-1)] = head;
	coherence();
	q->avail->idx++;
	iunlock(q);
	if((q->used->flags & 1) == 0)
		outs(q->dev->port+Qnotify, q->idx);
	while(!rock.done){
		while(waserror())
			;
		tsleep(rock.sleep, viodone, &rock, 1000);
		poperror();

		if(!rock.done)
			vqinterrupt(q);
	}
}

static int
vioblkreq(Vdev *vd, int typ, void *a, long count, long secsize, uvlong lba)
{
	int need, free, head;
	Vqueue *q;
	Vdesc *d;

	u8int status;
	struct Vioblkreqhdr {
		u32int	typ;
		u32int	prio;
		u64int	lba;
	} req;

	need = 2;
	if(a != nil)
		need = 3;

	status = -1;
	req.typ = typ;
	req.prio = 0;
	req.lba = lba;

	q = vd->queue[0];
	ilock(q);
	while(q->nfree < need){
		iunlock(q);

		if(!waserror())
			tsleep(&up->sleep, return0, 0, 500);
		poperror();

		ilock(q);
	}

	head = free = q->free;

	d = &q->desc[free]; free = d->next;
	d->addr = PADDR(&req);
	d->len = sizeof(req);
	d->flags = _Next;

	if(a != nil){
		d = &q->desc[free]; free = d->next;
		d->addr = PADDR(a);
		d->len = secsize*count;
		d->flags = typ ? _Next : (Write|_Next);
	}

	d = &q->desc[free]; free = d->next;
	d->addr = PADDR(&status);
	d->len = sizeof(status);
	d->flags = Write;

	q->free = free;
	q->nfree -= need;

	/* queue io, unlock and wait for completion */
	vqio(q, head);

	return status;
}

static int
vioscsireq(SDreq *r)
{
	u8int resp[4+4+2+2+SENSESIZE];
	u8int req[8+8+3+CDBSIZE];
	int free, head;
	u32int len;
	Vqueue *q;
	Vdesc *d;
	Vdev *vd;
	SDunit *u;
	ScsiCfg *cfg;

	u = r->unit;
	vd = u->dev->ctlr;
	cfg = vd->cfg;

	memset(resp, 0, sizeof(resp));
	memset(req, 0, sizeof(req));
	req[0] = 1;
	req[1] = u->subno;
	req[2] = r->lun>>8;
	req[3] = r->lun&0xFF;
	*(u64int*)(&req[8]) = (uintptr)r;

	memmove(&req[8+8+3], r->cmd, r->clen);

	q = vd->queue[2];
	ilock(q);
	while(q->nfree < 3){
		iunlock(q);

		if(!waserror())
			tsleep(&up->sleep, return0, 0, 500);
		poperror();

		ilock(q);
	}

	head = free = q->free;

	d = &q->desc[free]; free = d->next;
	d->addr = PADDR(req);
	d->len = 8+8+3+cfg->cdb_size;
	d->flags = _Next;

	if(r->write && r->dlen > 0){
		d = &q->desc[free]; free = d->next;
		d->addr = PADDR(r->data);
		d->len = r->dlen;
		d->flags = _Next;
	}

	d = &q->desc[free]; free = d->next;
	d->addr = PADDR(resp);
	d->len = 4+4+2+2+cfg->sense_size;
	d->flags = Write;

	if(!r->write && r->dlen > 0){
		d->flags |= _Next;

		d = &q->desc[free]; free = d->next;
		d->addr = PADDR(r->data);
		d->len = r->dlen;
		d->flags = Write;
	}

	q->free = free;
	q->nfree -= 2 + (r->dlen > 0);

	/* queue io, unlock and wait for completion */
	vqio(q, head);

	/* response+status */
	r->status = resp[10];
	if(resp[11] != 0)
		r->status = SDcheck;

	/* sense_len */
	len = *((u32int*)&resp[0]);
	if(len > 0){
		if(len > sizeof(r->sense))
			len = sizeof(r->sense);
		memmove(r->sense, &resp[4+4+2+2], len);
		r->flags |= SDvalidsense;
	}

	/* data residue */
	len = *((u32int*)&resp[4]);
	if(len > r->dlen)
		r->rlen = 0;
	else
		r->rlen = r->dlen - len;

	return r->status;

}

static long
viobio(SDunit *u, int lun, int write, void *a, long count, uvlong lba)
{
	long ss, cc, max, ret;
	Vdev *vd;

	vd = u->dev->ctlr;
	if(vd->typ == TypSCSI)
		return scsibio(u, lun, write, a, count, lba);

	max = 32;
	ss = u->secsize;
	ret = 0;
	while(count > 0){
		if((cc = count) > max)
			cc = max;
		if(vioblkreq(vd, write != 0, (uchar*)a + ret, cc, ss, lba) != 0)
			error(Eio);
		ret += cc*ss;
		count -= cc;
		lba += cc;
	}
	return ret;
}

enum {
	SDread,
	SDwrite,
};

static int
viorio(SDreq *r)
{
	int i, count, rw;
	uvlong lba;
	SDunit *u;
	Vdev *vd;

	u = r->unit;
	vd = u->dev->ctlr;
	if(vd->typ == TypSCSI)
		return vioscsireq(r);
	if(r->cmd[0] == 0x35 || r->cmd[0] == 0x91){
		if(vioblkreq(vd, 4, nil, 0, 0, 0) != 0)
			return sdsetsense(r, SDcheck, 3, 0xc, 2);
		return sdsetsense(r, SDok, 0, 0, 0);
	}
	if((i = sdfakescsi(r, nil, 0)) != SDnostatus)
		return r->status = i;
	if((i = sdfakescsirw(r, &lba, &count, &rw)) != SDnostatus)
		return i;
	r->rlen = viobio(u, r->lun, rw == SDwrite, r->data, count, lba);
	return r->status = SDok;
}

static int
vioonline(SDunit *u)
{
	uvlong cap;
	Vdev *vd;

	vd = u->dev->ctlr;
	if(vd->typ == TypSCSI)
		return scsionline(u);

	cap = inl(vd->port+Devspec+4);
	cap <<= 32;
	cap |= inl(vd->port+Devspec);
	if(u->sectors != cap){
		u->sectors = cap;
		u->secsize = 512;
		return 2;
	}
	return 1;
}

static int
vioverify(SDunit *u)
{
	Vdev *vd;

	vd = u->dev->ctlr;
	if(vd->typ == TypSCSI)
		return scsiverify(u);

	return 1;
}

SDifc sdvirtioifc;

static int
vioenable(SDev *sd)
{
	char name[32];
	Vdev *vd;

	vd = sd->ctlr;
	pcisetbme(vd->pci);
	snprint(name, sizeof(name), "%s (%s)", sd->name, sd->ifc->name);
	vd->vector = intrenable(vd->pci->intl, viointerrupt, vd, vd->pci->tbdf, name);
	outb(vd->port+Status, inb(vd->port+Status) | DriverOk);
	return 1;
}

static int
viodisable(SDev *sd)
{
	Vdev *vd;

	vd = sd->ctlr;
	intrdisable(vd->vector);
	pciclrbme(vd->pci);
	return 1;
}

static SDev*
viopnp(void)
{
	SDev *s, *h, *t;
	Vdev *vd;
	int id;

	h = t = nil;

	id = 'F';
	for(vd =  viopnpdevs(TypBlk); vd; vd = vd->next){
		if(vd->nqueue != 1)
			continue;

		if((s = malloc(sizeof(*s))) == nil)
			break;
		s->ctlr = vd;
		s->idno = id++;
		s->ifc = &sdvirtioifc;
		s->nunit = 1;
		if(h)
			t->next = s;
		else
			h = s;
		t = s;
	}

	id = '0';
	for(vd = viopnpdevs(TypSCSI); vd; vd = vd->next){
		ScsiCfg *cfg;

		if(vd->nqueue < 3)
			continue;

		if((cfg = malloc(sizeof(*cfg))) == nil)
			break;
		cfg->num_queues = inl(vd->port+Devspec+4*0);
		cfg->seg_max = inl(vd->port+Devspec+4*1);
		cfg->max_sectors = inl(vd->port+Devspec+4*2);
		cfg->cmd_per_lun = inl(vd->port+Devspec+4*3);
		cfg->event_info_size = inl(vd->port+Devspec+4*4);
		cfg->sense_size = inl(vd->port+Devspec+4*5);
		cfg->cdb_size = inl(vd->port+Devspec+4*6);
		cfg->max_channel = ins(vd->port+Devspec+4*7);
		cfg->max_target = ins(vd->port+Devspec+4*7+2);
		cfg->max_lun = inl(vd->port+Devspec+4*8);

		if(cfg->max_target == 0){
			free(cfg);
			continue;
		}
		if((cfg->cdb_size > CDBSIZE) || (cfg->sense_size > SENSESIZE)){
			print("sdvirtio: cdb %ud or sense size %ud too big\n",
				cfg->cdb_size, cfg->sense_size);
			free(cfg);
			continue;
		}
		vd->cfg = cfg;

		if((s = malloc(sizeof(*s))) == nil)
			break;
		s->ctlr = vd;
		s->idno = id++;
		s->ifc = &sdvirtioifc;
		s->nunit = cfg->max_target;
		if(h)
			t->next = s;
		else
			h = s;
		t = s;
	}

	return h;
}

SDifc sdvirtioifc = {
	"virtio",			/* name */

	viopnp,				/* pnp */
	nil,				/* legacy */
	vioenable,			/* enable */
	viodisable,			/* disable */

	vioverify,			/* verify */
	vioonline,			/* online */
	viorio,				/* rio */
	nil,				/* rctl */
	nil,				/* wctl */

	viobio,				/* bio */
	nil,				/* probe */
	nil,				/* clear */
	nil,				/* rtopctl */
	nil,				/* wtopctl */
};
