/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "sam.h"

static	Block	*blist;

static int
tempdisk(void)
{
	char buf[128];
	int i, fd;

	snprint(buf, sizeof buf, "/tmp/X%d.%.4ssam", getpid(), getuser());
	for(i='A'; i<='Z'; i++){
		buf[5] = i;
		if(access(buf, AEXIST) == 0)
			continue;
		fd = create(buf, ORDWR|ORCLOSE|OCEXEC, 0600);
		if(fd >= 0)
			return fd;
	}
	return -1;
}

Disk*
diskinit()
{
	Disk *d;

	d = emalloc(sizeof(Disk));
	d->fd = tempdisk();
	if(d->fd < 0){
		fprint(2, "sam: can't create temp file: %r\n");
		exits("diskinit");
	}
	return d;
}

static
uint
ntosize(uint n, uint *ip)
{
	uint size;

	if(n > Maxblock)
		panic("internal error: ntosize");
	size = n;
	if(size & (Blockincr-1))
		size += Blockincr - (size & (Blockincr-1));
	/* last bucket holds blocks of exactly Maxblock */
	if(ip)
		*ip = size/Blockincr;
	return size * sizeof(Rune);
}

Block*
disknewblock(Disk *d, uint n)
{
	uint i, j, size;
	Block *b;

	size = ntosize(n, &i);
	b = d->free[i];
	if(b)
		d->free[i] = b->next;
	else{
		/* allocate in chunks to reduce malloc overhead */
		if(blist == nil){
			blist = emalloc(100*sizeof(Block));
			for(j=0; j<100-1; j++)
				blist[j].next = &blist[j+1];
		}
		b = blist;
		blist = b->next;
		b->addr = d->addr;
		d->addr += size;
	}
	b->n = n;
	return b;
}

void
diskrelease(Disk *d, Block *b)
{
	uint i;

	ntosize(b->n, &i);
	b->next = d->free[i];
	d->free[i] = b;
}

void
diskwrite(Disk *d, Block **bp, Rune *r, uint n)
{
	int size, nsize;
	Block *b;

	b = *bp;
	size = ntosize(b->n, nil);
	nsize = ntosize(n, nil);
	if(size != nsize){
		diskrelease(d, b);
		b = disknewblock(d, n);
		*bp = b;
	}
	if(pwrite(d->fd, r, n*sizeof(Rune), b->addr) != n*sizeof(Rune))
		panic("write error to temp file");
	b->n = n;
}

void
diskread(Disk *d, Block *b, Rune *r, uint n)
{
	if(n > b->n)
		panic("internal error: diskread");

	ntosize(b->n, nil);	/* called only for sanity check on Maxblock */
	if(pread(d->fd, r, n*sizeof(Rune), b->addr) != n*sizeof(Rune))
		panic("read error from temp file");
}
