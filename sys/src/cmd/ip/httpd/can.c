#include <u.h>
#include <libc.h>
#include "can.h"

enum
{
	StructAlign = sizeof(union {vlong v; double d; ulong p; void *v;
				struct{vlong v;}vs; struct{double d;}ds; struct{ulong p;}ss; struct{void *v;}vs;})
};

enum
{
	CanSize	= 8*1024
};

struct Can
{
	Can	*next;
	ulong	total;			/* total bytes allocated in can->next */
	ulong	pos;
	ulong	end;
	ulong	v;			/* last value allocated */
	uchar	body[CanSize];
};

/*
 * allocator which allows an entire set to be freed at one time
 */
static Can*
mkCan(Can *can, ulong size)
{
	Can *c;

	size = ((size << 1) + (CanSize - 1)) & ~(CanSize - 1);
	c = malloc(sizeof(Can) + size - CanSize);
	if(c == nil)
		return nil;
	c->next = can;
	c->total = 0;
	if(can != nil)
		c->total = can->total + can->pos - (ulong)can->body;
	c->pos = (ulong)c->body;
	c->end = c->pos + size;
	return c;
}

void*
canAlloc(Can **can, ulong size, int zero)
{
	Can *c;
	ulong p;

	c = *can;
	if(c == nil){
		c = mkCan(nil, size);
		if(c == nil)
			return nil;
		*can = c;
	}
	p = c->pos;
	p = (p + (StructAlign - 1)) & ~(StructAlign - 1);
	if(p + size > c->end){
		c = mkCan(c, size);
		if(c == nil)
			return nil;
		*can = c;
		p = c->pos;
	}
	c->pos = p + size;
	c->v = p;
	if(zero)
		memset((void*)p, 0, size);
	return (void*)p;
}

/*
 * make a piece of allocated date bigger
 */
void*
canGrow(Can **can, void *op, ulong osize, ulong size, int zero)
{
	Can *c;
	void *np;
	ulong p;

	p = (ulong)op;
	c = *can;
	if(c != nil && p == c->v && p + size <= c->end){
		c->pos = p + size;
		if(zero)
			memset((char*)p + osize, 0, size - osize);
		return op;
	}
	np = canAlloc(can, size, zero);
	if(np == nil)
		return nil;
	memmove(np, op, osize);
	return np;
}

void
freeCan(Can *can)
{
	Can *last;

	while(can != nil){
		last = can;
		can = can->next;
		last->pos = (ulong)last->body;
		free(last);
	}
}
