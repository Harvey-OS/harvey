#include "stdinc.h"
#include "dat.h"
#include "fns.h"

/*
 * disk structure conversion routines
 */
#define	U8GET(p)	((p)[0])
#define	U16GET(p)	(((p)[0]<<8)|(p)[1])
#define	U32GET(p)	(((p)[0]<<24)|((p)[1]<<16)|((p)[2]<<8)|(p)[3])
#define	U64GET(p)	(((vlong)U32GET(p)<<32)|(vlong)U32GET((p)+4))

#define	U8PUT(p,v)	(p)[0]=(v)
#define	U16PUT(p,v)	(p)[0]=(v)>>8;(p)[1]=(v)
#define	U32PUT(p,v)	(p)[0]=(v)>>24;(p)[1]=(v)>>16;(p)[2]=(v)>>8;(p)[3]=(v)
#define	U64PUT(p,v,t32)	t32=(v)>>32;U32PUT(p,t32);t32=(v);U32PUT((p)+4,t32)

u32int
unpackMagic(u8int *buf)
{
	return U32GET(buf);
}

void
packMagic(u32int magic, u8int *buf)
{
	U32PUT(buf, magic);
}

int
unpackArenaPart(ArenaPart *ap, u8int *buf)
{
	u8int *p;
	u32int m;

	p = buf;

	m = U32GET(p);
	if(m != ArenaPartMagic){
		setErr(ECorrupt, "arena set has wrong magic number: %lux expected %lux", m, ArenaPartMagic);
		return 0;
	}
	p += U32Size;
	ap->version = U32GET(p);
	p += U32Size;
	ap->blockSize = U32GET(p);
	p += U32Size;
	ap->arenaBase = U32GET(p);
	p += U32Size;

	if(buf + ArenaPartSize != p)
		fatal("unpackArenaPart unpacked wrong amount");

	return 1;
}

int
packArenaPart(ArenaPart *ap, u8int *buf)
{
	u8int *p;

	p = buf;

	U32PUT(p, ArenaPartMagic);
	p += U32Size;
	U32PUT(p, ap->version);
	p += U32Size;
	U32PUT(p, ap->blockSize);
	p += U32Size;
	U32PUT(p, ap->arenaBase);
	p += U32Size;

	if(buf + ArenaPartSize != p)
		fatal("packArenaPart packed wrong amount");

	return 1;
}

int
unpackArena(Arena *arena, u8int *buf)
{
	u8int *p;
	u32int m;

	p = buf;

	m = U32GET(p);
	if(m != ArenaMagic){
		setErr(ECorrupt, "arena has wrong magic number: %lux expected %lux", m, ArenaMagic);
		return 0;
	}
	p += U32Size;
	arena->version = U32GET(p);
	p += U32Size;
	nameCp(arena->name, (char*)p);
	p += ANameSize;
	arena->clumps = U32GET(p);
	p += U32Size;
	arena->cclumps = U32GET(p);
	p += U32Size;
	arena->ctime = U32GET(p);
	p += U32Size;
	arena->wtime = U32GET(p);
	p += U32Size;
	arena->used = U64GET(p);
	p += U64Size;
	arena->uncsize = U64GET(p);
	p += U64Size;
	arena->sealed = U8GET(p);
	p += U8Size;

	if(buf + ArenaSize != p)
		fatal("unpackArena unpacked wrong amount");

	return 1;
}

int
packArena(Arena *arena, u8int *buf)
{
	u8int *p;
	u32int t32;

	p = buf;

	U32PUT(p, ArenaMagic);
	p += U32Size;
	U32PUT(p, arena->version);
	p += U32Size;
	nameCp((char*)p, arena->name);
	p += ANameSize;
	U32PUT(p, arena->clumps);
	p += U32Size;
	U32PUT(p, arena->cclumps);
	p += U32Size;
	U32PUT(p, arena->ctime);
	p += U32Size;
	U32PUT(p, arena->wtime);
	p += U32Size;
	U64PUT(p, arena->used, t32);
	p += U64Size;
	U64PUT(p, arena->uncsize, t32);
	p += U64Size;
	U8PUT(p, arena->sealed);
	p += U8Size;

	if(buf + ArenaSize != p)
		fatal("packArena packed wrong amount");

	return 1;
}

int
unpackArenaHead(ArenaHead *head, u8int *buf)
{
	u8int *p;
	u32int m;

	p = buf;

	m = U32GET(p);
	if(m != ArenaHeadMagic){
		setErr(ECorrupt, "arena has wrong magic number: %lux expected %lux", m, ArenaHeadMagic);
		return 0;
	}
	p += U32Size;
	head->version = U32GET(p);
	p += U32Size;
	nameCp(head->name, (char*)p);
	p += ANameSize;
	head->blockSize = U32GET(p);
	p += U32Size;
	head->size = U64GET(p);
	p += U64Size;

	if(buf + ArenaHeadSize != p)
		fatal("unpackArenaHead unpacked wrong amount");

	return 1;
}

int
packArenaHead(ArenaHead *head, u8int *buf)
{
	u8int *p;
	u32int t32;

	p = buf;

	U32PUT(p, ArenaHeadMagic);
	p += U32Size;
	U32PUT(p, head->version);
	p += U32Size;
	nameCp((char*)p, head->name);
	p += ANameSize;
	U32PUT(p, head->blockSize);
	p += U32Size;
	U64PUT(p, head->size, t32);
	p += U64Size;

	if(buf + ArenaHeadSize != p)
		fatal("packArenaHead packed wrong amount");

	return 1;
}

static int
checkClump(Clump *w)
{
	if(w->encoding == ClumpENone){
		if(w->info.size != w->info.uncsize){
			setErr(ECorrupt, "uncompressed wad size mismatch");
			return 0;
		}
	}else if(w->encoding == ClumpECompress){
		if(w->info.size >= w->info.uncsize){
			setErr(ECorrupt, "compressed lump has inconsistent block sizes %d %d", w->info.size, w->info.uncsize);
			return 0;
		}
	}else{
		setErr(ECorrupt, "clump has illegal encoding");
		return 0;
	}

	return 1;
}

int
unpackClump(Clump *c, u8int *buf)
{
	u8int *p;
	u32int magic;

	p = buf;
	magic = U32GET(p);
	if(magic != ClumpMagic){
		setErr(ECorrupt, "clump has bad magic number=%#8.8ux", magic);
		return 0;
	}
	p += U32Size;

	c->info.type = U8GET(p);
	p += U8Size;
	c->info.size = U16GET(p);
	p += U16Size;
	c->info.uncsize = U16GET(p);
	p += U16Size;
	scoreCp(c->info.score, p);
	p += VtScoreSize;

	c->encoding = U8GET(p);
	p += U8Size;
	c->creator = U32GET(p);
	p += U32Size;
	c->time = U32GET(p);
	p += U32Size;

	if(buf + ClumpSize != p)
		fatal("unpackClump unpacked wrong amount");

	return checkClump(c);
}

int
packClump(Clump *c, u8int *buf)
{
	u8int *p;

	p = buf;
	U32PUT(p, ClumpMagic);
	p += U32Size;

	U8PUT(p, c->info.type);
	p += U8Size;
	U16PUT(p, c->info.size);
	p += U16Size;
	U16PUT(p, c->info.uncsize);
	p += U16Size;
	scoreCp(p, c->info.score);
	p += VtScoreSize;

	U8PUT(p, c->encoding);
	p += U8Size;
	U32PUT(p, c->creator);
	p += U32Size;
	U32PUT(p, c->time);
	p += U32Size;

	if(buf + ClumpSize != p)
		fatal("packClump packed wrong amount");

	return checkClump(c);
}

void
unpackClumpInfo(ClumpInfo *ci, u8int *buf)
{
	u8int *p;

	p = buf;
	ci->type = U8GET(p);
	p += U8Size;
	ci->size = U16GET(p);
	p += U16Size;
	ci->uncsize = U16GET(p);
	p += U16Size;
	scoreCp(ci->score, p);
	p += VtScoreSize;

	if(buf + ClumpInfoSize != p)
		fatal("unpackClumpInfo unpacked wrong amount");
}

void
packClumpInfo(ClumpInfo *ci, u8int *buf)
{
	u8int *p;

	p = buf;
	U8PUT(p, ci->type);
	p += U8Size;
	U16PUT(p, ci->size);
	p += U16Size;
	U16PUT(p, ci->uncsize);
	p += U16Size;
	scoreCp(p, ci->score);
	p += VtScoreSize;

	if(buf + ClumpInfoSize != p)
		fatal("packClumpInfo packed wrong amount");
}

int
unpackISect(ISect *is, u8int *buf)
{
	u8int *p;
	u32int m;

	p = buf;


	m = U32GET(p);
	if(m != ISectMagic){
		setErr(ECorrupt, "index section has wrong magic number: %lux expected %lux", m, ISectMagic);
		return 0;
	}
	p += U32Size;
	is->version = U32GET(p);
	p += U32Size;
	nameCp(is->name, (char*)p);
	p += ANameSize;
	nameCp(is->index, (char*)p);
	p += ANameSize;
	is->blockSize = U32GET(p);
	p += U32Size;
	is->blockBase = U32GET(p);
	p += U32Size;
	is->blocks = U32GET(p);
	p += U32Size;
	is->start = U32GET(p);
	p += U32Size;
	is->stop = U32GET(p);
	p += U32Size;

	if(buf + ISectSize != p)
		fatal("unpackISect unpacked wrong amount");

	return 1;
}

int
packISect(ISect *is, u8int *buf)
{
	u8int *p;

	p = buf;

	U32PUT(p, ISectMagic);
	p += U32Size;
	U32PUT(p, is->version);
	p += U32Size;
	nameCp((char*)p, is->name);
	p += ANameSize;
	nameCp((char*)p, is->index);
	p += ANameSize;
	U32PUT(p, is->blockSize);
	p += U32Size;
	U32PUT(p, is->blockBase);
	p += U32Size;
	U32PUT(p, is->blocks);
	p += U32Size;
	U32PUT(p, is->start);
	p += U32Size;
	U32PUT(p, is->stop);
	p += U32Size;

	if(buf + ISectSize != p)
		fatal("packISect packed wrong amount");

	return 1;
}

void
unpackIEntry(IEntry *ie, u8int *buf)
{
	u8int *p;

	p = buf;

	scoreCp(ie->score, p);
	p += VtScoreSize;
	ie->wtime = U32GET(p);
	p += U32Size;
	ie->train = U16GET(p);
	p += U16Size;
	ie->ia.addr = U64GET(p);
	p += U64Size;
	ie->ia.size = U16GET(p);
	p += U16Size;
	if(p - buf != IEntryTypeOff)
		fatal("unpackIEntry bad IEntryTypeOff amount");
	ie->ia.type = U8GET(p);
	p += U8Size;
	ie->ia.blocks = U8GET(p);
	p += U8Size;

	if(p - buf != IEntrySize)
		fatal("unpackIEntry unpacked wrong amount");
}

void
packIEntry(IEntry *ie, u8int *buf)
{
	u32int t32;
	u8int *p;

	p = buf;

	scoreCp(p, ie->score);
	p += VtScoreSize;
	U32PUT(p, ie->wtime);
	p += U32Size;
	U16PUT(p, ie->train);
	p += U16Size;
	U64PUT(p, ie->ia.addr, t32);
	p += U64Size;
	U16PUT(p, ie->ia.size);
	p += U16Size;
	U8PUT(p, ie->ia.type);
	p += U8Size;
	U8PUT(p, ie->ia.blocks);
	p += U8Size;

	if(p - buf != IEntrySize)
		fatal("packIEntry packed wrong amount");
}

void
unpackIBucket(IBucket *b, u8int *buf)
{
	b->n = U16GET(buf);
	b->next = U32GET(&buf[U16Size]);
	b->data = buf + IBucketSize;
}

void
packIBucket(IBucket *b, u8int *buf)
{
	U16PUT(buf, b->n);
	U32PUT(&buf[U16Size], b->next);
}
