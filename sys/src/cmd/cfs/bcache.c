/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include "cformat.h"
#include "lru.h"
#include "bcache.h"

int
bcinit(Bcache *bc, int f, int bsize)
{
	Bbuf *b;

	/*
	 *  allocate space for all buffers
	 *  point all buffers into outer space
	 */
	bc->dfirst = 0;
	bc->bsize = bsize;
	bc->f = f;
	lruinit(&bc->lru);
	for(b = bc->bb; b < &bc->bb[Nbcache]; b++){
		b->inuse = 0;
		b->next = 0;
		b->dirty = 0;
		if(b->data == 0)
			b->data = (char *)malloc(bc->bsize);
		if(b->data == 0)
			return -1;
		lruadd(&bc->lru, &b->lru);
	}

	return 0;
}

/*
 *  Find a buffer for block b.  If it's dirty, write it out.
 */
Bbuf *
bcfind(Bcache *bc, uint32_t bno)
{
	Bbuf *b;

	if(bno == Notabno)
		error("bcfind: Notabno");
	bno &= ~Indbno;

	/*
	 *  if we already have a buffer for this bno, use it
	 */
	for(b = bc->bb; b < &bc->bb[Nbcache]; b++)
		if(b->inuse && b->bno==bno)
			goto out;

	/*
	 *  get least recently used block
	 */
	b = (Bbuf*)bc->lru.lnext;
out:
	/*
	 *  if dirty, write it out
	 */
	if(b->dirty)
		if(bcwrite(bc, b) < 0)
			warning("writing dirty page");
	lruref(&bc->lru, &b->lru);
	return b;
}

/*
 *  allocate a buffer block for a block.  it's guaranteed to be there till
 *  the next Nbcache bcread's.
 */
Bbuf *
bcalloc(Bcache *bc, uint32_t bno)
{
	Bbuf *b;

	b = bcfind(bc, bno);
	bno &= ~Indbno;
	b->bno = bno;
	b->inuse = 1;
	return b;
}

/*
 *  read a block into a buffer cache.  it's guaranteed to be there till
 *  the next Nbcache bcread's.
 */
Bbuf *
bcread(Bcache *bc, uint32_t bno)
{
	Bbuf *b;

	b = bcfind(bc, bno);
	bno &= ~Indbno;
	if(b->bno!=bno || !b->inuse)
		/*
		 *  read in the one we really want
		 */
		if(bread(bc, bno, b->data) < 0){
			b->inuse = 0;
			return 0;
		}
	b->bno = bno;
	b->inuse = 1;
	return b;
}

/*
 *  mark a page dirty, if it's already dirty force a write
 *
 *	N.B: ordering is important.
 */
void
bcmark(Bcache *bc, Bbuf *b)
{
	lruref(&bc->lru, &b->lru);

	if(b->dirty){
		bcwrite(bc, b);
		return;
	}

	b->dirty = 1;
	if(bc->dfirst)
		bc->dlast->next = b;
	else
		bc->dfirst = b;
	bc->dlast = b;
}

/*
 *  write out a page (and all preceding dirty ones)
 */
int
bcwrite(Bcache *bc, Bbuf *b)
{
	Bbuf *nb;

	/*
	 *  write out all preceding pages
	 */
	while((nb = bc->dfirst)){
		if(bwrite(bc, nb->bno, nb->data) < 0)
			return -1;
		nb->dirty = 0;
		bc->dfirst = nb->next;
		nb->next = 0;
		if(nb == b)
			return 0;
	}

	/*
	 *  write out this page
	 */
	if(bwrite(bc, b->bno, b->data) < 0)
		return -1;
	b->dirty = 0;
	b->next = 0;
	return 0;
}

/*
 *  write out all dirty pages (in order)
 */
int
bcsync(Bcache *bc)
{
	if(bc->dfirst)
		return bcwrite(bc, bc->dlast);
	return 0;
}

/*
 *  read a block from disk
 */
int
bread(Bcache *bc, uint32_t bno, void *buf)
{
	uint64_t x = (uint64_t)bno * bc->bsize;

	if(pread(bc->f, buf, bc->bsize, x) != bc->bsize)
		return -1;
	return 0;
}

/*
 *  write a block to disk
 */
int
bwrite(Bcache *bc, uint32_t bno, void *buf)
{
	uint64_t x = (uint64_t)bno * bc->bsize;

	if(pwrite(bc->f, buf, bc->bsize, x) != bc->bsize)
		return -1;
	return 0;
}
