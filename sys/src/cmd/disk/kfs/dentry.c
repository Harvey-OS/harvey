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
accessdir(Iobuf *p, Dentry *d, int f)
{
	long t;

	if(p && !isro(p->dev)) {
		p->flags |= Bmod;
		t = time();
		if(f & (FREAD|FWRITE))
			d->atime = t;
		if(f & FWRITE) {
			d->mtime = t;
			d->qid.version++;
		}
	}
}

void
dbufread(Iobuf *p, Dentry *d, long a)
{
	USED(p, d, a);
}

Iobuf*
dnodebuf(Iobuf *p, Dentry *d, long a, int tag)
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
			addr = balloc(p->dev, tag, d->qid.path);
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
			addr = balloc(p->dev, Tind1, d->qid.path);
			d->iblock = addr;
			p->flags |= Bmod|Bimm;
		}
		addr = indfetch(p, d, addr, a, Tind1, tag);
		if(addr)
			bp = getbuf(p->dev, addr, Bread);
		return bp;
	}
	a -= INDPERBUF;
	if(a < INDPERBUF2) {
		addr = d->diblock;
		if(!addr && tag) {
			addr = balloc(p->dev, Tind2, d->qid.path);
			d->diblock = addr;
			p->flags |= Bmod|Bimm;
		}
		addr = indfetch(p, d, addr, a/INDPERBUF, Tind2, Tind1);
		addr = indfetch(p, d, addr, a%INDPERBUF, Tind1, tag);
		if(addr)
			bp = getbuf(p->dev, addr, Bread);
		return bp;
	}
	print("dnodebuf: trip indirect\n");
	return 0;
}

long
indfetch(Iobuf *p, Dentry *d, long addr, long a, int itag, int tag)
{
	Iobuf *bp;

	if(!addr)
		return 0;
	bp = getbuf(p->dev, addr, Bread);
	if(!bp || checktag(bp, itag, d->qid.path)) {
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
		addr = balloc(p->dev, tag, d->qid.path);
		if(addr) {
			((long*)bp->iobuf)[a] = addr;
			p->flags |= Bmod;
			if(localfs || tag == Tdir)
				bp->flags |= Bimm;
			settag(bp, itag, d->qid.path);
		}
	}
	putbuf(bp);
	return addr;
}

void
dtrunc(Iobuf *p, Dentry *d)
{
	int i;

	bfree(p->dev, d->diblock, 2);
	d->diblock = 0;
	bfree(p->dev, d->iblock, 1);
	d->iblock = 0;
	for(i=NDBLOCK-1; i>=0; i--) {
		bfree(p->dev, d->dblock[i], 0);
		d->dblock[i] = 0;
	}
	d->size = 0;
	p->flags |= Bmod|Bimm;
	accessdir(p, d, FWRITE);
}
