#include "stdinc.h"
#include "dat.h"
#include "fns.h"

static int	writeClumpHead(Arena *arena, u64int aa, Clump *cl);
static int	writeClumpMagic(Arena *arena, u64int aa, u32int magic);

int
clumpInfoEq(ClumpInfo *c, ClumpInfo *d)
{
	return c->type == d->type
		&& c->size == d->size
		&& c->uncsize == d->uncsize
		&& scoreEq(c->score, d->score);
}

/*
 * synchronize the clump info directory with
 * with the clumps actually stored in the arena.
 * the directory should be at least as up to date
 * as the arena's trailer.
 *
 * checks/updates at most n clumps.
 *
 * returns 1 if ok, -1 if an error occured, 0 if blocks were updated
 */
int
syncArena(Arena *arena, u32int n, int zok, int fix)
{
	ZBlock *lump;
	Clump cl;
	ClumpInfo ci;
	static ClumpInfo zci;
	u8int score[VtScoreSize];
	u64int uncsize, used, aa;
	u32int clump, clumps, cclumps, magic;
	int err, flush, broken;

	used = arena->used;
	clumps = arena->clumps;
	cclumps = arena->cclumps;
	uncsize = arena->uncsize;

	flush = 0;
	err = 0;
	for(; n; n--){
		aa = arena->used;
		clump = arena->clumps;
		magic = clumpMagic(arena, aa);
		if(magic == ClumpFreeMagic)
			break;
		if(magic != ClumpMagic){
			fprint(2, "illegal clump magic number=%#8.8ux at clump=%d\n", magic, clump);
			err |= SyncDataErr;
//ZZZ write a zero here?
			if(0 && fix && !writeClumpMagic(arena, aa, ClumpFreeMagic)){
				fprint(2, "can't write corrected clump free magic: %R");
				err |= SyncFixErr;
			}
			break;
		}
		arena->clumps++;

		broken = 0;
		lump = loadClump(arena, aa, 0, &cl, score, 0);
		if(lump == nil){
			fprint(2, "clump=%d failed to read correctly: %R\n", clump);
			err |= SyncDataErr;
		}else if(cl.info.type != VtTypeCorrupt){
			scoreMem(score, lump->data, cl.info.uncsize);
			if(!scoreEq(cl.info.score, score)){
				fprint(2, "clump=%d has mismatched score\n", clump);
				err = SyncDataErr;
				broken = 1;
			}else if(!vtTypeValid(cl.info.type)){
				fprint(2, "clump=%d has invalid type %d", clump, cl.info.type);
				err = SyncDataErr;
				broken = 1;
			}
			if(broken && fix){
				cl.info.type = VtTypeCorrupt;
				if(!writeClumpHead(arena, aa, &cl)){
					fprint(2, "can't write corrected clump header: %R");
					err |= SyncFixErr;
				}
			}
		}
		freeZBlock(lump);
		arena->used += ClumpSize + cl.info.size;

		if(!broken && !readClumpInfo(arena, clump, &ci)){
			fprint(2, "arena directory read failed\n");
			broken = 1;
		}else if(!broken && !clumpInfoEq(&ci, &cl.info)){
			if(clumpInfoEq(&ci, &zci)){
				err |= SyncCIZero;
				if(!zok)
					fprint(2, "unwritten clump info for clump=%d\n", clump);
			}else{
				err |= SyncCIErr;
				fprint(2, "bad clump info for clump=%d\n", clump);
				fprint(2, "\texpected score=%V type=%d size=%d uncsize=%d\n",
					cl.info.score, cl.info.type, cl.info.size, cl.info.uncsize);
				fprint(2, "\tfound score=%V type=%d size=%d uncsize=%d\n",
					ci.score, ci.type, ci.size, ci.uncsize);
			}
			broken = 1;
		}
		if(broken && fix){
			flush = 1;
			ci = cl.info;
			if(!writeClumpInfo(arena, clump, &ci)){
				fprint(2, "can't write correct clump directory: %R\n");
				err |= SyncFixErr;
			}
		}

		arena->uncsize += cl.info.uncsize;
		if(cl.info.size < cl.info.uncsize)
			arena->cclumps++;
	}

	if(flush){
		arena->wtime = now();
		if(arena->ctime == 0 && arena->clumps)
			arena->ctime = arena->wtime;
		if(!flushCIBlocks(arena)){
			fprint(2, "can't flush arena directory cache: %R");
			err |= SyncFixErr;
		}
	}

	if(used != arena->used
	|| clumps != arena->clumps
	|| cclumps != arena->cclumps
	|| uncsize != arena->uncsize)
		err |= SyncHeader;

	return err;
}

static int
writeClumpHead(Arena *arena, u64int aa, Clump *cl)
{
	ZBlock *zb;
	int ok;

	zb = allocZBlock(ClumpSize, 0);
	if(zb == nil)
		return 0;
	ok = packClump(cl, zb->data)
		&& writeArena(arena, aa, zb->data, ClumpSize) == ClumpSize;
	freeZBlock(zb);
	return ok;
}

static int
writeClumpMagic(Arena *arena, u64int aa, u32int magic)
{
	u8int buf[U32Size];

	packMagic(magic, buf);
	return writeArena(arena, aa, buf, U32Size) == U32Size;
}
