#include "stdinc.h"
#include "dat.h"
#include "fns.h"

int			queueWrites = 0;

static Packet		*readILump(Lump *u, IAddr *ia, u8int *score);

Packet*
readLump(u8int *score, int type, u32int size)
{
	Lump *u;
	Packet *p;
	IAddr ia;
	u32int n;

	vtLock(stats.lock);
	stats.lumpReads++;
	vtUnlock(stats.lock);
	u = lookupLump(score, type);
	if(u->data != nil){
		n = packetSize(u->data);
		if(n > size){
			setErr(EOk, "read too small: asked for %d need at least %d", size, n);
			putLump(u);

			return nil;
		}
		p = packetDup(u->data, 0, n);
		putLump(u);

		return p;
	}

	if(!lookupScore(score, type, &ia)){
		//ZZZ place to check for someone trying to guess scores
		setErr(EOk, "no block with that score exists");

		putLump(u);
		return nil;
	}
	if(ia.size > size){
		setErr(EOk, "read too small 1: asked for %d need at least %d", size, ia.size);

		putLump(u);
		return nil;
	}

	p = readILump(u, &ia, score);
	putLump(u);

	return p;
}

/*
 * save away a lump, and return it's score.
 * doesn't store duplicates, but checks that the data is really the same.
 */
int
writeLump(Packet *p, u8int *score, int type, u32int creator)
{
	Lump *u;
	int ok;

	vtLock(stats.lock);
	stats.lumpWrites++;
	vtUnlock(stats.lock);

	packetSha1(p, score);

	u = lookupLump(score, type);
	if(u->data != nil){
		ok = 1;
		if(packetCmp(p, u->data) != 0){
			setErr(EStrange, "score collision");
			ok = 0;
		}
		packetFree(p);
		putLump(u);
		return ok;
	}

	if(queueWrites)
		return queueWrite(u, p, creator);

	ok = writeQLump(u, p, creator);

	putLump(u);
	return ok;
}

int
writeQLump(Lump *u, Packet *p, int creator)
{
	ZBlock *flat;
	Packet *old;
	IAddr ia;
	int ok;

	if(lookupScore(u->score, u->type, &ia)){
		/*
		 * if the read fails,
		 * assume it was corrupted data and store the block again
		 */
		old = readILump(u, &ia, u->score);
		if(old != nil){
			ok = 1;
			if(packetCmp(p, old) != 0){
				setErr(EStrange, "score collision");
				ok = 0;
			}
			packetFree(p);
			packetFree(old);

			return ok;
		}
		logErr(EAdmin, "writelump: read %V failed, rewriting: %R\n", u->score);
	}

	flat = packet2ZBlock(p, packetSize(p));
	ok = storeClump(mainIndex, flat, u->score, u->type, creator, &ia);
	freeZBlock(flat);
	if(ok)
		ok = insertScore(u->score, &ia);
	if(ok)
		insertLump(u, p);
	else
		packetFree(p);

	return ok;
}

static Packet*
readILump(Lump *u, IAddr *ia, u8int *score)
{
	Arena *arena;
	ZBlock *zb;
	Packet *p, *pp;
	Clump cl;
	u64int aa;
	u8int sc[VtScoreSize];

	arena = amapItoA(mainIndex, ia->addr, &aa);
	if(arena == nil)
		return nil;

	zb = loadClump(arena, aa, ia->blocks, &cl, sc, paranoid);
	if(zb == nil)
		return nil;

	if(ia->size != cl.info.uncsize){
		setErr(EInconsist, "index and clump size mismatch");
		freeZBlock(zb);
		return nil;
	}
	if(ia->type != cl.info.type){
		setErr(EInconsist, "index and clump type mismatch");
		freeZBlock(zb);
		return nil;
	}
	if(!scoreEq(score, sc)){
		setErr(ECrash, "score mismatch");
		freeZBlock(zb);
		return nil;
	}
	p = zblock2Packet(zb, cl.info.uncsize);
	freeZBlock(zb);
	pp = packetDup(p, 0, packetSize(p));
	insertLump(u, pp);
	return p;
}
