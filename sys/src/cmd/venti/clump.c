#include "stdinc.h"
#include "dat.h"
#include "fns.h"
#include "whack.h"

/*
 * writes a lump to disk
 * returns the address in amap of the clump
 */
int
storeClump(Index *ix, ZBlock *zb, u8int *sc, int type, u32int creator, IAddr *ia)
{
	ZBlock *cb;
	Clump cl;
	u64int a;
	u8int bh[VtScoreSize];
	int size, dsize;

	size = zb->len;
	if(size > VtMaxLumpSize){
		setErr(EStrange, "lump too large");
		return 0;
	}
	if(!vtTypeValid(type)){
		setErr(EStrange, "invalid lump type");
		return 0;
	}

	if(1){
		scoreMem(bh, zb->data, size);
		if(!scoreEq(sc, bh)){
			setErr(ECorrupt, "storing clump: corrupted; expected=%V got=%V, size=%d", sc, bh, size);
			return 0;
		}
	}

	cb = allocZBlock(size + ClumpSize, 0);
	if(cb == nil)
		return 0;

	cl.info.type = type;
	cl.info.uncsize = size;
	cl.creator = creator;
	cl.time = now();
	scoreCp(cl.info.score, sc);

	dsize = whackblock(&cb->data[ClumpSize], zb->data, size);
	if(dsize > 0 && dsize < size){
		cl.encoding = ClumpECompress;
	}else{
		cl.encoding = ClumpENone;
		dsize = size;
		memmove(&cb->data[ClumpSize], zb->data, size);
	}
	cl.info.size = dsize;

	a = writeIClump(ix, &cl, cb->data);

	freeZBlock(cb);
	if(a == 0)
		return 0;

	vtLock(stats.lock);
	stats.clumpWrites++;
	stats.clumpBWrites += size;
	stats.clumpBComp += dsize;
	vtUnlock(stats.lock);

	ia->addr = a;
	ia->type = type;
	ia->size = size;
	ia->blocks = (dsize + ClumpSize + (1 << ABlockLog) - 1) >> ABlockLog;

	return 1;
}

u32int
clumpMagic(Arena *arena, u64int aa)
{
	u8int buf[U32Size];

	if(!readArena(arena, aa, buf, U32Size))
		return TWID32;
	return unpackMagic(buf);
}

/*
 * fetch a block based at addr.
 * score is filled in with the block's score.
 * blocks is roughly the length of the clump on disk;
 * if zero, the length is unknown.
 */
ZBlock*
loadClump(Arena *arena, u64int aa, int blocks, Clump *cl, u8int *score, int verify)
{
	Unwhack uw;
	ZBlock *zb, *cb;
	u8int bh[VtScoreSize], *buf;
	u32int n;
	int nunc;

	vtLock(stats.lock);
	stats.clumpReads++;
	vtUnlock(stats.lock);

	if(blocks <= 0)
		blocks = 1;

	cb = allocZBlock(blocks << ABlockLog, 0);
	if(cb == nil)
		return nil;
	n = readArena(arena, aa, cb->data, blocks << ABlockLog);
	if(n < ClumpSize){
		if(n != 0)
			setErr(ECorrupt, "loadClump read less than a header");
		freeZBlock(cb);
		return nil;
	}
	if(!unpackClump(cl, cb->data)){
		freeZBlock(cb);
		return nil;
	}
	n -= ClumpSize;
	if(n < cl->info.size){
		freeZBlock(cb);
		n = cl->info.size;
		cb = allocZBlock(n, 0);
		if(cb == nil)
			return nil;
		if(readArena(arena, aa + ClumpSize, cb->data, n) != n){
			setErr(ECorrupt, "loadClump read too little data");
			freeZBlock(cb);
			return nil;
		}
		buf = cb->data;
	}else
		buf = cb->data + ClumpSize;

	scoreCp(score, cl->info.score);

	zb = allocZBlock(cl->info.uncsize, 0);
	if(zb == nil){
		freeZBlock(cb);
		return nil;
	}
	switch(cl->encoding){
	case ClumpECompress:
		unwhackinit(&uw);
		nunc = unwhack(&uw, zb->data, cl->info.uncsize, buf, cl->info.size);
		if(nunc != cl->info.uncsize){
			if(nunc < 0)
				setErr(ECorrupt, "decompression failed: %s", uw.err);
			else
				setErr(ECorrupt, "decompression gave partial block: %d/%d\n", nunc, cl->info.uncsize);
			freeZBlock(cb);
			freeZBlock(zb);
			return nil;
		}
		break;
	case ClumpENone:
		if(cl->info.size != cl->info.uncsize){
			setErr(ECorrupt, "loading clump: bad uncompressed size for uncompressed block");
			freeZBlock(cb);
			freeZBlock(zb);
			return nil;
		}
		memmove(zb->data, buf, cl->info.uncsize);
		break;
	default:
		setErr(ECorrupt, "unknown encoding in loadLump");
		freeZBlock(cb);
		freeZBlock(zb);
		return nil;
	}
	freeZBlock(cb);

	if(verify){
		scoreMem(bh, zb->data, cl->info.uncsize);
		if(!scoreEq(cl->info.score, bh)){
			setErr(ECorrupt, "loading clump: corrupted; expected=%V got=%V", cl->info.score, bh);
			freeZBlock(zb);
			return nil;
		}
		if(!vtTypeValid(cl->info.type)){
			setErr(ECorrupt, "loading lump: invalid lump type %d", cl->info.type);
			freeZBlock(zb);
			return nil;
		}
	}

	vtLock(stats.lock);
	stats.clumpBReads += cl->info.size;
	stats.clumpBUncomp += cl->info.uncsize;
	vtUnlock(stats.lock);

	return zb;
}
