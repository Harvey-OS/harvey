/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "stdinc.h"
#include "dat.h"
#include "fns.h"

void
fmtzbinit(Fmt *f, ZBlock *b)
{
	memset(f, 0, sizeof *f);
#ifdef PLAN9PORT
	fmtlocaleinit(f, nil, nil, nil);
#endif
	f->start = b->data;
	f->to = f->start;
	f->stop = (char*)f->start + b->len;
}

#define ROUNDUP(p, n) ((void*)(((uintptr)(p)+(n)-1)&~(uintptr)((n)-1)))

enum {
	OverflowCheck = 32
};
static char zmagic[] = "1234567890abcdefghijklmnopqrstuvxyz";

ZBlock *
alloczblock(uint32_t size, int zeroed, uint blocksize)
{
	uint8_t *p, *data;
	ZBlock *b;
	static ZBlock z;
	int n;

	if(blocksize == 0)
		blocksize = 32;	/* try for cache line alignment */

	n = size+OverflowCheck+sizeof(ZBlock)+blocksize+8;
	p = malloc(n);
	if(p == nil){
		seterr(EOk, "out of memory");
		return nil;
	}

	data = ROUNDUP(p, blocksize);
	b = ROUNDUP(data+size+OverflowCheck, 8);
	if(0) fprint(2, "alloc %p-%p data %p-%p b %p-%p\n",
		p, p+n, data, data+size, b, b+1);
	*b = z;
	b->data = data;
	b->free = p;
	b->len = size;
	b->_size = size;
	if(zeroed)
		memset(b->data, 0, size);
	memmove(b->data+size, zmagic, OverflowCheck);
	return b;
}

void
freezblock(ZBlock *b)
{
	if(b){
		if(memcmp(b->data+b->_size, zmagic, OverflowCheck) != 0)
			abort();
		memset(b->data+b->_size, 0, OverflowCheck);
		free(b->free);
	}
}

ZBlock*
packet2zblock(Packet *p, uint32_t size)
{
	ZBlock *b;

	if(p == nil)
		return nil;
	b = alloczblock(size, 0, 0);
	if(b == nil)
		return nil;
	if(packetcopy(p, b->data, 0, size) < 0){
		freezblock(b);
		return nil;
	}
	return b;
}

Packet*
zblock2packet(ZBlock *zb, uint32_t size)
{
	Packet *p;

	if(zb == nil)
		return nil;
	p = packetalloc();
	packetappend(p, zb->data, size);
	return p;
}
