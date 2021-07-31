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

	if(p && p->dev->type != Devro) {
		p->flags |= Bmod;
		t = time();
		if(f & (FREAD|FWRITE))
			d->atime = t;
		if(f & FWRITE) {
			d->mtime = t;
			d->muid = uid;
			d->qid.version++;
		}
	}
}

void
preread(Device *d, long addr)
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
	rb->dev = d;
	rb->addr = addr;
	send(raheadq, rb);
	cons.brahead[0].count++;
	cons.brahead[1].count++;
	cons.brahead[2].count++;
}

long
rel2abs(Iobuf *p, Dentry *d, long a, int tag, int putb, int uid)
{
	long addr, qpath;
	Device *dev;

	if(a < 0) {
		print("dnodebuf: neg\n");
		return 0;
	}
	dev = p->dev;
	qpath = d->qid.path;
	if(a < NDBLOCK) {
		addr = d->dblock[a];
		if(!addr && tag) {
			addr = bufalloc(dev, tag, qpath, uid);
			d->dblock[a] = addr;
			p->flags |= Bmod|Bimm;
		}
		if(putb)
			putbuf(p);
		return addr;
	}
	a -= NDBLOCK;
	if(a < INDPERBUF) {
		addr = d->iblock;
		if(!addr && tag) {
			addr = bufalloc(dev, Tind1, qpath, uid);
			d->iblock = addr;
			p->flags |= Bmod|Bimm;
		}
		if(putb)
			putbuf(p);
		addr = indfetch(dev, qpath, addr, a, Tind1, tag, uid);
		return addr;
	}
	a -= INDPERBUF;
	if(a < INDPERBUF2) {
		addr = d->diblock;
		if(!addr && tag) {
			addr = bufalloc(dev, Tind2, qpath, uid);
			d->diblock = addr;
			p->flags |= Bmod|Bimm;
		}
		if(putb)
			putbuf(p);
		addr = indfetch(dev, qpath, addr, a/INDPERBUF, Tind2, Tind1, uid);
		addr = indfetch(dev, qpath, addr, a%INDPERBUF, Tind1, tag, uid);
		return addr;
	}
	if(putb)
		putbuf(p);
	print("dnodebuf: trip indirect\n");
	return 0;
}

/*
 * read-ahead strategy
 * on second block, read RAGAP blocks,
 * thereafter, read RAGAP ahead of current pos
 */
long
dbufread(Iobuf *p, Dentry *d, long a, long ra, int uid)
{
	long addr;

	if(a == 0)
		return 1;
	if(a == 1 && ra == 1) {
		while(ra < a+RAGAP) {
			ra++;
			addr = rel2abs(p, d, ra, 0, 0, uid);
			if(!addr)
				return 0;
			preread(p->dev, addr);
		}
		return ra+1;
	}
	if(ra == a+RAGAP) {
		addr = rel2abs(p, d, ra, 0, 0, uid);
		if(!addr)
			return 0;
		preread(p->dev, addr);
		return ra+1;
	}
	return ra;
}

Iobuf*
dnodebuf(Iobuf *p, Dentry *d, long a, int tag, int uid)
{
	long addr;

	addr = rel2abs(p, d, a, tag, 0, uid);
	if(addr)
		return getbuf(p->dev, addr, Bread);
	return 0;
}

/*
 * same as dnodebuf but it calls putpuf(p)
 * to reduce interference.
 */
Iobuf*
dnodebuf1(Iobuf *p, Dentry *d, long a, int tag, int uid)
{
	long addr;
	Device *dev;

	dev = p->dev;
	addr = rel2abs(p, d, a, tag, 1, uid);
	if(addr)
		return getbuf(dev, addr, Bread);
	return 0;

}

long
indfetch(Device* d, long qpath, long addr, long a, int itag, int tag, int uid)
{
	Iobuf *bp;

	if(!addr)
		return 0;
	bp = getbuf(d, addr, Bread);
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
		addr = bufalloc(d, tag, qpath, uid);
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
