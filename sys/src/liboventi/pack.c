#include <u.h>
#include <libc.h>
#include <oventi.h>

/*
 * integer conversion routines
 */
#define	U8GET(p)	((p)[0])
#define	U16GET(p)	(((p)[0]<<8)|(p)[1])
#define	U32GET(p)	((u32int)(((p)[0]<<24)|((p)[1]<<16)|((p)[2]<<8)|(p)[3]))
#define	U48GET(p)	(((vlong)U16GET(p)<<32)|(vlong)U32GET((p)+2))
#define	U64GET(p)	(((vlong)U32GET(p)<<32)|(vlong)U32GET((p)+4))

#define	U8PUT(p,v)	(p)[0]=(v)
#define	U16PUT(p,v)	(p)[0]=(v)>>8;(p)[1]=(v)
#define	U32PUT(p,v)	(p)[0]=(v)>>24;(p)[1]=(v)>>16;(p)[2]=(v)>>8;(p)[3]=(v)
#define	U48PUT(p,v,t32)	t32=(v)>>32;U16PUT(p,t32);t32=(v);U32PUT((p)+2,t32)
#define	U64PUT(p,v,t32)	t32=(v)>>32;U32PUT(p,t32);t32=(v);U32PUT((p)+4,t32)

static int
checkSize(int n)
{
	if(n < 256 || n > VtMaxLumpSize) {
		vtSetError("bad block size");
		return 0;
	}
	return 1;
}
		

void
vtRootPack(VtRoot *r, uchar *p)
{
	uchar *op = p;

	U16PUT(p, r->version);
	p += 2;
	memmove(p, r->name, sizeof(r->name));
	p += sizeof(r->name);
	memmove(p, r->type, sizeof(r->type));
	p += sizeof(r->type);
	memmove(p, r->score, VtScoreSize);
	p +=  VtScoreSize;
	U16PUT(p, r->blockSize);
	p += 2;
	memmove(p, r->prev, VtScoreSize);
	p += VtScoreSize;

	assert(p-op == VtRootSize);
}

int
vtRootUnpack(VtRoot *r, uchar *p)
{
	uchar *op = p;

	memset(r, 0, sizeof(*r));

	r->version = U16GET(p);
	if(r->version != VtRootVersion) {
		vtSetError("unknown root version");
		return 0;
	}
	p += 2;
	memmove(r->name, p, sizeof(r->name));
	r->name[sizeof(r->name)-1] = 0;
	p += sizeof(r->name);
	memmove(r->type, p, sizeof(r->type));
	r->type[sizeof(r->type)-1] = 0;
	p += sizeof(r->type);
	memmove(r->score, p, VtScoreSize);
	p +=  VtScoreSize;
	r->blockSize = U16GET(p);
	if(!checkSize(r->blockSize))
		return 0;
	p += 2;
	memmove(r->prev, p, VtScoreSize);
	p += VtScoreSize;

	assert(p-op == VtRootSize);
	return 1;
}

void
vtEntryPack(VtEntry *e, uchar *p, int index)
{
	ulong t32;
	int flags;
	uchar *op;

	p += index * VtEntrySize;
	op = p;

	U32PUT(p, e->gen);
	p += 4;
	U16PUT(p, e->psize);
	p += 2;
	U16PUT(p, e->dsize);
	p += 2;
	flags = e->flags | ((e->depth << VtEntryDepthShift) & VtEntryDepthMask);
	U8PUT(p, flags);
	p++;
	memset(p, 0, 5);
	p += 5;
	U48PUT(p, e->size, t32);
	p += 6;
	memmove(p, e->score, VtScoreSize);
	p += VtScoreSize;

	assert(p-op == VtEntrySize);
}

int
vtEntryUnpack(VtEntry *e, uchar *p, int index)
{
	uchar *op;

	p += index * VtEntrySize;
	op = p;

	e->gen = U32GET(p);
	p += 4;
	e->psize = U16GET(p);
	p += 2;
	e->dsize = U16GET(p);
	p += 2;
	e->flags = U8GET(p);
	e->depth = (e->flags & VtEntryDepthMask) >> VtEntryDepthShift;
	e->flags &= ~VtEntryDepthMask;
	p++;
	p += 5;
	e->size = U48GET(p);
	p += 6;
	memmove(e->score, p, VtScoreSize);
	p += VtScoreSize;

	assert(p-op == VtEntrySize);
	
	if(!(e->flags & VtEntryActive))
		return 1;

	if(!checkSize(e->psize) || !checkSize(e->dsize))
		return 0;

	return 1;
}

