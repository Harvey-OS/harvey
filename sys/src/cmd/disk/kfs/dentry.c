/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
		if(!(f & (FWRITE|FWSTAT)) && noatime)
			return;
		t = time(nil);
		if(f & (FREAD|FWRITE|FWSTAT)){
			d->atime = t;
			p->flags |= Bmod;
		}
		if(f & FWRITE) {
			d->mtime = t;
			d->qid.version++;
			p->flags |= Bmod;
		}
	}
}

void
dbufread(Iobuf *p, Dentry *d, long a)
{
	USED(p, d, a);
}

long
rel2abs(Iobuf *p, Dentry *d, long a, int tag, int putb)
{
	long addr, qpath;
	Device dev;

	if(a < 0) {
		print("dnodebuf: neg\n");
		return 0;
	}
	qpath = d->qid.path;
	dev = p->dev;
	if(a < NDBLOCK) {
		addr = d->dblock[a];
		if(!addr && tag) {
			addr = balloc(dev, tag, qpath);
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
			addr = balloc(dev, Tind1, qpath);
			d->iblock = addr;
			p->flags |= Bmod|Bimm;
		}
		if(putb)
			putbuf(p);
		addr = indfetch(p, d, addr, a, Tind1, tag);
		return addr;
	}
	a -= INDPERBUF;
	if(a < INDPERBUF2) {
		addr = d->diblock;
		if(!addr && tag) {
			addr = balloc(dev, Tind2, qpath);
			d->diblock = addr;
			p->flags |= Bmod|Bimm;
		}
		if(putb)
			putbuf(p);
		addr = indfetch(p, d, addr, a/INDPERBUF, Tind2, Tind1);
		addr = indfetch(p, d, addr, a%INDPERBUF, Tind1, tag);
		return addr;
	}
	if(putb)
		putbuf(p);
	print("dnodebuf: trip indirect\n");
	return 0;
}

Iobuf*
dnodebuf(Iobuf *p, Dentry *d, long a, int tag)
{
	long addr;

	addr = rel2abs(p, d, a, tag, 0);
	if(addr)
		return getbuf(p->dev, addr, Bread);
	return 0;
}

/*
 * same as dnodebuf but it calls putpuf(p)
 * to reduce interference.
 */
Iobuf*
dnodebuf1(Iobuf *p, Dentry *d, long a, int tag)
{
	long addr;
	Device dev;

	dev = p->dev;
	addr = rel2abs(p, d, a, tag, 1);
	if(addr)
		return getbuf(dev, addr, Bread);
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
			bp->flags |= Bmod;
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
