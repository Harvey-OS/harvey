#include	"all.h"

Dentry*
getdir(Iobuf *p, int slot)
{
	Dentry *d;

	if(!p)
		return 0;
	d = (Dentry*)p->iobuf + slot%DIRPERBUF;
	return d;
}

void
accessdir(Iobuf *p, Dentry *d, int f, int uid)
{
	long t;

	if(p && p->dev.type != Devro) {
		p->flags |= Bmod;
		t = time();
		if(f & (FREAD|FWRITE))
			d->atime = t;
		if(f & FWRITE) {
			d->mtime = t;
			d->wuid = uid;
			d->qid.version++;
		}
	}
}

void
preread(Device dev, long addr)
{
	Rabuf *rb;

	if(addr == 0)
		return;
	if(raheadq->count+10 >= raheadq->size)	/* ugly knowing layout */
		return;
	lock(&rabuflock);
	rb = rabuffree;
	if(rb == 0) {
		unlock(&rabuflock);
		return;
	}
	rabuffree = rb->link;
	unlock(&rabuflock);
	rb->dev = dev;
	rb->addr = addr;
	send(raheadq, rb);
	cons.brahead.count++;
}

void
dbufread(Iobuf *p, Dentry *d, long ad, int uid)
{
	long addr, last, a;

	last = ad + RACHUNK + RAOVERLAP;

loop:
	a = ad;
	if(a < 0 || a >= last)
		return;
	ad++;
	if(a < NDBLOCK) {
		addr = d->dblock[a];
		if(!addr)
			return;
		preread(p->dev, addr);
		goto loop;
	}
	a -= NDBLOCK;
	if(a < INDPERBUF) {
		addr = d->iblock;
		if(!addr)
			return;
		addr = indfetch(p->dev, d->qid.path, addr, a, Tind1, 0, uid);
		if(!addr)
			return;
		preread(p->dev, addr);
		goto loop;
	}
	a -= INDPERBUF;
	if(a < INDPERBUF2) {
		addr = d->diblock;
		if(!addr)
			return;
		addr = indfetch(p->dev, d->qid.path, addr, a/INDPERBUF, Tind2, Tind1, uid);
		if(!addr)
			return;
		addr = indfetch(p->dev, d->qid.path, addr, a%INDPERBUF, Tind1, 0, uid);
		if(!addr)
			return;
		preread(p->dev, addr);
		goto loop;
	}
}

Iobuf*
dnodebuf(Iobuf *p, Dentry *d, long a, int tag, int uid)
{
	Iobuf *bp;
	long addr;

	if(a < 0) {
		print("dnodebuf: neg\n");
		return 0;
	}
	bp = 0;
	if(a < NDBLOCK) {
		addr = d->dblock[a];
		if(addr)
			return getbuf(p->dev, addr, Bread);
		if(tag) {
			addr = bufalloc(p->dev, tag, d->qid.path, uid);
			if(addr) {
				d->dblock[a] = addr;
				p->flags |= Bmod|Bimm;
				bp = getbuf(p->dev, addr, Bmod);
			}
		}
		return bp;
	}
	a -= NDBLOCK;
	if(a < INDPERBUF) {
		addr = d->iblock;
		if(!addr && tag) {
			addr = bufalloc(p->dev, Tind1, d->qid.path, uid);
			d->iblock = addr;
			p->flags |= Bmod|Bimm;
		}
		addr = indfetch(p->dev, d->qid.path, addr, a, Tind1, tag, uid);
		if(addr)
			bp = getbuf(p->dev, addr, Bread);
		return bp;
	}
	a -= INDPERBUF;
	if(a < INDPERBUF2) {
		addr = d->diblock;
		if(!addr && tag) {
			addr = bufalloc(p->dev, Tind2, d->qid.path, uid);
			d->diblock = addr;
			p->flags |= Bmod|Bimm;
		}
		addr = indfetch(p->dev, d->qid.path, addr, a/INDPERBUF, Tind2, Tind1, uid);
		addr = indfetch(p->dev, d->qid.path, addr, a%INDPERBUF, Tind1, tag, uid);
		if(addr)
			bp = getbuf(p->dev, addr, Bread);
		return bp;
	}
	print("dnodebuf: trip indirect\n");
	return 0;
}

/*
 * same as dnodebuf but it calls putpuf(p)
 * to reduce interference.
 */
Iobuf*
dnodebuf1(Iobuf *p, Dentry *d, long a, int tag, int uid)
{
	Iobuf *bp;
	long addr;
	Device dev;
	long qpath;

	if(a < 0) {
		putbuf(p);
		print("dnodebuf1: neg\n");
		return 0;
	}

	bp = 0;
	dev = p->dev;
	qpath = d->qid.path;
	if(a < NDBLOCK) {
		addr = d->dblock[a];
		if(addr) {
			putbuf(p);
			return getbuf(dev, addr, Bread);
		}
		if(tag) {
			addr = bufalloc(dev, tag, qpath, uid);
			if(addr) {
				d->dblock[a] = addr;
				p->flags |= Bmod|Bimm;
				putbuf(p);
				bp = getbuf(dev, addr, Bmod);
				return bp;
			}
		}
		putbuf(p);
		return 0;
	}

	a -= NDBLOCK;
	if(a < INDPERBUF) {
		addr = d->iblock;
		if(!addr && tag) {
			addr = bufalloc(dev, Tind1, d->qid.path, uid);
			d->iblock = addr;
			p->flags |= Bmod|Bimm;
		}
		putbuf(p);
		addr = indfetch(dev, qpath, addr, a, Tind1, tag, uid);
		if(addr)
			bp = getbuf(dev, addr, Bread);
		return bp;
	}
	a -= INDPERBUF;
	if(a < INDPERBUF2) {
		addr = d->diblock;
		if(!addr && tag) {
			addr = bufalloc(dev, Tind2, d->qid.path, uid);
			d->diblock = addr;
			p->flags |= Bmod|Bimm;
		}
		putbuf(p);
		addr = indfetch(dev, qpath, addr, a/INDPERBUF, Tind2, Tind1, uid);
		addr = indfetch(dev, qpath, addr, a%INDPERBUF, Tind1, tag, uid);
		if(addr)
			bp = getbuf(dev, addr, Bread);
		return bp;
	}
	print("dnodebuf1: trip indirect\n");
	putbuf(p);
	return 0;
}

long
indfetch(Device dev, long qpath, long addr, long a, int itag, int tag, int uid)
{
	Iobuf *bp;

	if(!addr)
		return 0;
	bp = getbuf(dev, addr, Bread);
	if(!bp || checktag(bp, itag, qpath)) {
		if(!bp) {
			print("ind fetch bp = 0\n");
			return 0;
		}
		print("ind fetch tag\n");
		putbuf(bp);
		return 0;
	}
	addr = ((long*)bp->iobuf)[a];
	if(!addr && tag) {
		addr = bufalloc(dev, tag, qpath, uid);
		if(addr) {
			((long*)bp->iobuf)[a] = addr;
			bp->flags |= Bmod;
			if(tag == Tdir)
				bp->flags |= Bimm;
			settag(bp, itag, qpath);
		}
	}
	putbuf(bp);
	return addr;
}

void
dtrunc(Iobuf *p, Dentry *d, int uid)
{
	int i;

	buffree(p->dev, d->diblock, 2);
	d->diblock = 0;
	buffree(p->dev, d->iblock, 1);
	d->iblock = 0;
	for(i=NDBLOCK-1; i>=0; i--) {
		buffree(p->dev, d->dblock[i], 0);
		d->dblock[i] = 0;
	}
	d->size = 0;
	p->flags |= Bmod|Bimm;
	accessdir(p, d, FWRITE, uid);
}
