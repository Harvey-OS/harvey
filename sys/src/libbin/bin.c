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
#include <bin.h>

enum
{
	StructAlign = sizeof(union {int64_t vl; double d; uint32_t p; void *v;
				struct{int64_t v;}
vs;
struct {
	double d;
} ds;
struct {
	uint32_t p;
} ss;
struct {
	void *v;
} xs;
})
}
;

enum {
	BinSize = 8 * 1024
};

struct Bin {
	Bin *next;
	uint32_t total; /* total bytes allocated in can->next */
	uintptr pos;
	uintptr end;
	uintptr v; /* last value allocated */
	uint8_t body[BinSize];
};

/*
 * allocator which allows an entire set to be freed at one time
 */
static Bin *
mkbin(Bin *bin, uint32_t size)
{
	Bin *b;

	size = ((size << 1) + (BinSize - 1)) & ~(BinSize - 1);
	b = malloc(sizeof(Bin) + size - BinSize);
	if(b == nil)
		return nil;
	b->next = bin;
	b->total = 0;
	if(bin != nil)
		b->total = bin->total + bin->pos - (uintptr)bin->body;
	b->pos = (uintptr)b->body;
	b->end = b->pos + size;
	return b;
}

void *
binalloc(Bin **bin, uint32_t size, int zero)
{
	Bin *b;
	uintptr p;

	if(size == 0)
		size = 1;
	b = *bin;
	if(b == nil) {
		b = mkbin(nil, size);
		if(b == nil)
			return nil;
		*bin = b;
	}
	p = b->pos;
	p = (p + (StructAlign - 1)) & ~(StructAlign - 1);
	if(p + size > b->end) {
		b = mkbin(b, size);
		if(b == nil)
			return nil;
		*bin = b;
		p = b->pos;
	}
	b->pos = p + size;
	b->v = p;
	if(zero)
		memset((void *)p, 0, size);
	return (void *)p;
}

void *
bingrow(Bin **bin, void *op, uint32_t osize, uint32_t size, int zero)
{
	Bin *b;
	void *np;
	uintptr p;

	p = (uintptr)op;
	b = *bin;
	if(b != nil && p == b->v && p + size <= b->end) {
		b->pos = p + size;
		if(zero)
			memset((char *)p + osize, 0, size - osize);
		return op;
	}
	np = binalloc(bin, size, zero);
	if(np == nil)
		return nil;
	memmove(np, op, osize);
	return np;
}

void
binfree(Bin **bin)
{
	Bin *last;

	while(*bin != nil) {
		last = *bin;
		*bin = (*bin)->next;
		last->pos = (uintptr)last->body;
		free(last);
	}
}
