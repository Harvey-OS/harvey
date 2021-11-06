#include "all.h"

Dentry*
getdir(Iobuf *p, int slot)
{
	if(!p)
		return 0;
	return (Dentry*)p->iobuf + slot%DIRPERBUF;
}

void
accessdir(Iobuf *p, Dentry *d, int f, int uid)
{
	Timet t;

	if(p && p->dev->type != Devro) {
		p->flags |= Bmod;
		t = time(nil);
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
preread(Device *d, Off addr)
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
	fs_send(raheadq, rb);
}

Off
rel2abs(Iobuf *p, Dentry *d, Off a, int tag, int putb, int uid)
{
	int i;
	Off addr, qpath, indaddrs = 1, div;
	Device *dev;

	if(a < 0) {
		print("rel2abs: neg offset\n");
		if(putb)
			putbuf(p);
		return 0;
	}
	dev = p->dev;
	qpath = d->qid.path;

	/* is `a' a direct block? */
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

	/*
	 * loop through indirect block depths.
	 */
	for (i = 0; i < NIBLOCK; i++) {
		indaddrs *= INDPERBUF;
		/* is a's disk addr in this indir block or one of its kids? */
		if (a < indaddrs) {
			addr = d->iblocks[i];
			if(!addr && tag) {
				addr = bufalloc(dev, Tind1+i, qpath, uid);
				d->iblocks[i] = addr;
				p->flags |= Bmod|Bimm;
			}
			if(putb)
				putbuf(p);

			div = indaddrs;
			for (; i >= 0; i--) {
				div /= INDPERBUF;
				if (div <= 0)
					panic("rel2abs: non-positive divisor");
				addr = indfetch(dev, qpath, addr,
					(a/div)%INDPERBUF, Tind1+i,
					(i == 0? tag: Tind1+i-1), uid);
			}
			return addr;
		}
		a -= indaddrs;
	}
	if(putb)
		putbuf(p);

	/* quintuple-indirect blocks not implemented. */
	print("rel2abs: no %d-deep indirect\n", NIBLOCK+1);
	return 0;
}

/*
 * read-ahead strategy
 * on second block, read RAGAP blocks,
 * thereafter, read RAGAP ahead of current pos
 */
Off
dbufread(Iobuf *p, Dentry *d, Off a, Off ra, int uid)
{
	Off addr;

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
dnodebuf(Iobuf *p, Dentry *d, Off a, int tag, int uid)
{
	Off addr;

	addr = rel2abs(p, d, a, tag, 0, uid);
	if(addr)
		return getbuf(p->dev, addr, Brd);
	return 0;
}

/*
 * same as dnodebuf but it calls putbuf(p)
 * to reduce interference.
 */
Iobuf*
dnodebuf1(Iobuf *p, Dentry *d, Off a, int tag, int uid)
{
	Off addr;
	Device *dev;

	dev = p->dev;
	addr = rel2abs(p, d, a, tag, 1, uid);
	if(addr)
		return getbuf(dev, addr, Brd);
	return 0;

}

Off
indfetch(Device* d, Off qpath, Off addr, Off a, int itag, int tag, int uid)
{
	Iobuf *bp;

	if(!addr)
		return 0;
	bp = getbuf(d, addr, Brd);
	if(!bp || checktag(bp, itag, qpath)) {
		if(!bp) {
			print("ind fetch bp = 0\n");
			return 0;
		}
		print("ind fetch tag\n");
		putbuf(bp);
		return 0;
	}
	addr = ((Off *)bp->iobuf)[a];
	if(!addr && tag) {
		addr = bufalloc(d, tag, qpath, uid);
		if(addr) {
			((Off *)bp->iobuf)[a] = addr;
			bp->flags |= Bmod;
			if(tag == Tdir)
				bp->flags |= Bimm;
			settag(bp, itag, qpath);
		}
	}
	putbuf(bp);
	return addr;
}

/* return INDPERBUF^exp */
Off
ibbpow(int exp)
{
	static Off pows[] = {
		1,
		INDPERBUF,
		(Off)INDPERBUF*INDPERBUF,
		(Off)INDPERBUF*(Off)INDPERBUF*INDPERBUF,
		(Off)INDPERBUF*(Off)INDPERBUF*(Off)INDPERBUF*INDPERBUF,
	};

	if (exp < 0)
		return 0;
	else if (exp >= nelem(pows)) {	/* not in table? do it long-hand */
		Off indpow = 1;

		while (exp-- > 0 && indpow > 0)
			indpow *= INDPERBUF;
		return indpow;
	} else
		return pows[exp];
}

/* return sum of INDPERBUF^n for 1 ≤ n ≤ exp */
Off
ibbpowsum(int exp)
{
	Off indsum = 0;

	for (; exp > 0; exp--)
		indsum += ibbpow(exp);
	return indsum;
}

/* zero bytes past new file length; return an error code */
int
trunczero(Truncstate *ts)
{
	int blkoff = ts->newsize % BUFSIZE;
	Iobuf *pd;

	pd = dnodebuf(ts->p, ts->d, ts->lastblk, Tfile, ts->uid);
	if (pd == nil || checktag(pd, Tfile, QPNONE)) {
		if (pd != nil)
			putbuf(pd);
		ts->err = Ephase;
		return Ephase;
	}
	memset(pd->iobuf+blkoff, 0, BUFSIZE - blkoff);
	putbuf(pd);
	return 0;
}

/*
 * truncate d (in p) to length `newsize'.
 * if larger, just increase size.
 * if smaller, deallocate blocks after last one
 * still in file at new size.  last byte to keep
 * is newsize-1, due to zero origin.
 * we free in forward order because it's simpler to get right.
 * if the final block at the new size is partially-filled,
 * zero the remainder.
 */
int
dtrunclen(Iobuf *p, Dentry *d, Off newsize, int uid)
{
	int i, pastlast;
	Truncstate trunc;

	if (newsize <= 0) {
		dtrunc(p, d, uid);
		return 0;
	}
	memset(&trunc, 0, sizeof trunc);
	trunc.d = d;
	trunc.p = p;
	trunc.uid = uid;
	trunc.newsize = newsize;
	trunc.lastblk = newsize/BUFSIZE;
	if (newsize % BUFSIZE == 0)
		trunc.lastblk--;
	else
		trunczero(&trunc);
	for (i = 0; i < NDBLOCK; i++)
		if (trunc.pastlast) {
			trunc.relblk = i;
			buffree(p->dev, d->dblock[i], 0, &trunc);
			d->dblock[i] = 0;
		} else if (i == trunc.lastblk)
			trunc.pastlast = 1;
	trunc.relblk = NDBLOCK;
	for (i = 0; i < NIBLOCK; i++) {
		pastlast = trunc.pastlast;
		buffree(p->dev, d->iblocks[i], i+1, &trunc);
		if (pastlast)
			d->iblocks[i] = 0;
	}

	d->size = newsize;
	p->flags |= Bmod|Bimm;
	accessdir(p, d, FWRITE, uid);
	return trunc.err;
}

/*
 * truncate d (in p) to zero length.
 * freeing blocks in reverse order is traditional, from Unix,
 * in an attempt to keep the free list contiguous.
 */
void
dtrunc(Iobuf *p, Dentry *d, int uid)
{
	int i;

	for (i = NIBLOCK-1; i >= 0; i--) {
		buffree(p->dev, d->iblocks[i], i+1, nil);
		d->iblocks[i] = 0;
	}
	for (i = NDBLOCK-1; i >= 0; i--) {
		buffree(p->dev, d->dblock[i], 0, nil);
		d->dblock[i] = 0;
	}
	d->size = 0;
	p->flags |= Bmod|Bimm;
	accessdir(p, d, FWRITE, uid);
}
