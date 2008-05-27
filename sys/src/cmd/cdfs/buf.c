/*
 * Buffered I/O on block devices.
 * Write buffering ignores offset.
 */

#include <u.h>
#include <libc.h>
#include <disk.h>
#include "dat.h"
#include "fns.h"

Buf*
bopen(long (*fn)(Buf*, void*, long, ulong), int omode, int bs, int nblock)
{
	Buf *b;

	assert(omode == OREAD || OWRITE);
	assert(bs > 0 && nblock > 0);
	assert(fn != nil);

	b = emalloc(sizeof(*b));
	b->data = emalloc(bs*nblock);
	b->ndata = 0;
	b->nblock = nblock;
	b->bs = bs;
	b->omode = omode;
	b->fn = fn;		/* function to read or write bs-byte blocks */

	return b;
}

long
bread(Buf *b, void *v, long n, vlong off)
{
	long m;
	vlong noff;

	assert(b->omode == OREAD);

	/* Refill buffer */
	if(b->off > off || off >= b->off+b->ndata) {
		noff = off - off % b->bs;
		if(vflag)
			fprint(2, "try refill at %lld...", noff);
		if((m = b->fn(b, b->data, b->nblock, noff/b->bs)) <= 0) {
			if (vflag)
				fprint(2, "failed\n");
			return m;
		}
		b->ndata = b->bs * m;
		b->off = noff;
		if(vflag)
			fprint(2, "got %ld\n", b->ndata);
	}

//	fprint(2, "read %ld at %ld\n", n, off);
	/* Satisfy request from buffer */
	off -= b->off;
	if(n > b->ndata - off)
		n = b->ndata - off;
	memmove(v, b->data+off, n);
	return n;
}

long
bwrite(Buf *b, void *v, long n)
{
	long on, m, mdata;
	uchar *p;

	p = v;
	on = n;

	/* Fill buffer */
	mdata = b->bs*b->nblock;
	m = mdata - b->ndata;
	if(m > n)
		m = n;
	memmove(b->data+b->ndata, p, m);
	p += m;
	n -= m;
	b->ndata += m;

	/* Flush buffer */
	if(b->ndata == mdata) {
		if(b->fn(b, b->data, b->nblock, 0) < 0) {
			if(vflag)
				fprint(2, "write fails: %r\n");
			return -1;
		}
		b->ndata = 0;
	}

	/* For now, don't worry about big writes; 9P only does 8k */
	assert(n < mdata);

	/* Add remainder to buffer */
	if(n) {
		memmove(b->data, p, n);
		b->ndata = n;
	}

	return on;
}

void
bterm(Buf *b)
{
	/* DVD & BD prefer full ecc blocks (tracks), but can cope with less */
	if(b->omode == OWRITE && b->ndata)
		b->fn(b, b->data, (b->ndata + b->bs - 1)/b->bs, 0); 

	free(b->data);
	free(b);
}
