#include "stdinc.h"
#include "dat.h"
#include "fns.h"

typedef struct ASum ASum;

struct ASum
{
	Arena	*arena;
	ASum	*next;
};

static void	sealArena(Arena *arena);
static int	okArena(Arena *arena);
static int	loadArena(Arena *arena);
static CIBlock	*getCIB(Arena *arena, int clump, int writing, CIBlock *rock);
static void	putCIB(Arena *arena, CIBlock *cib);
static int	flushCIEntry(Arena *arena, int i);
static void	doASum(void *);

static VtLock	*sumLock;
static VtRendez	*sumWait;
static ASum	*sumq;

int
initArenaSum(void)
{
	sumLock = vtLockAlloc();
	sumWait = vtRendezAlloc(sumLock);
	if(vtThread(doASum, nil) < 0){
		setErr(EOk, "can't start arena checksum slave: %R");
		return 0;
	}
	return 1;
}

/*
 * make an Arena, and initialize it based upon the disk header and trailer.
 */
Arena*
initArena(Part *part, u64int base, u64int size, u32int blockSize)
{
	Arena *arena;

	arena = MKZ(Arena);
	arena->part = part;
	arena->blockSize = blockSize;
	arena->clumpMax = arena->blockSize / ClumpInfoSize;
	arena->base = base + blockSize;
	arena->size = size - 2 * blockSize;
	arena->lock = vtLockAlloc();

	if(!loadArena(arena)){
		setErr(ECorrupt, "arena header or trailer corrupted");
		freeArena(arena);
		return nil;
	}
	if(!okArena(arena)){
		freeArena(arena);
		return nil;
	}

	if(arena->sealed && scoreEq(zeroScore, arena->score))
		backSumArena(arena);

	return arena;
}

void
freeArena(Arena *arena)
{
	if(arena == nil)
		return;
	if(arena->cib.data != nil){
		putDBlock(arena->cib.data);
		arena->cib.data = nil;
	}
	vtLockFree(arena->lock);
	free(arena);
}

Arena*
newArena(Part *part, char *name, u64int base, u64int size, u32int blockSize)
{
	Arena *arena;

	if(!nameOk(name)){
		setErr(EOk, "illegal arena name", name);
		return nil;
	}
	arena = MKZ(Arena);
	arena->part = part;
	arena->version = ArenaVersion;
	arena->blockSize = blockSize;
	arena->clumpMax = arena->blockSize / ClumpInfoSize;
	arena->base = base + blockSize;
	arena->size = size - 2 * blockSize;
	arena->lock = vtLockAlloc();

	nameCp(arena->name, name);

	if(!wbArena(arena) || !wbArenaHead(arena)){
		freeArena(arena);
		return nil;
	}

	return arena;
}

int
readClumpInfo(Arena *arena, int clump, ClumpInfo *ci)
{
	CIBlock *cib, r;

	cib = getCIB(arena, clump, 0, &r);
	if(cib == nil)
		return 0;
	unpackClumpInfo(ci, &cib->data->data[cib->offset]);
	putCIB(arena, cib);
	return 1;
}

int
readClumpInfos(Arena *arena, int clump, ClumpInfo *cis, int n)
{
	CIBlock *cib, r;
	int i;

	for(i = 0; i < n; i++){
		cib = getCIB(arena, clump + i, 0, &r);
		if(cib == nil)
			break;
		unpackClumpInfo(&cis[i], &cib->data->data[cib->offset]);
		putCIB(arena, cib);
	}
	return i;
}

/*
 * write directory information for one clump
 * must be called the arena locked
 */
int
writeClumpInfo(Arena *arena, int clump, ClumpInfo *ci)
{
	CIBlock *cib, r;

	cib = getCIB(arena, clump, 1, &r);
	if(cib == nil)
		return 0;
	packClumpInfo(ci, &cib->data->data[cib->offset]);
	putCIB(arena, cib);
	return 1;
}

u64int
arenaDirSize(Arena *arena, u32int clumps)
{
	return ((clumps / arena->clumpMax) + 1) * arena->blockSize;
}

/*
 * read a clump of data
 * n is a hint of the size of the data, not including the header
 * make sure it won't run off the end, then return the number of bytes actually read
 */
u32int
readArena(Arena *arena, u64int aa, u8int *buf, long n)
{
	DBlock *b;
	u64int a;
	u32int blockSize, off, m;
	long nn;

	if(n == 0)
		return 0;

	vtLock(arena->lock);
	a = arena->size - arenaDirSize(arena, arena->clumps);
	vtUnlock(arena->lock);
	if(aa >= a){
		setErr(EOk, "reading beyond arena clump storage: clumps=%d aa=%lld a=%lld -1 clumps=%lld\n", arena->clumps, aa, a, arena->size - arenaDirSize(arena, arena->clumps - 1));
		return 0;
	}
	if(aa + n > a)
		n = a - aa;

	blockSize = arena->blockSize;
	a = arena->base + aa;
	off = a & (blockSize - 1);
	a -= off;
	nn = 0;
	for(;;){
		b = getDBlock(arena->part, a, 1);
		if(b == nil)
			return 0;
		m = blockSize - off;
		if(m > n - nn)
			m = n - nn;
		memmove(&buf[nn], &b->data[off], m);
		putDBlock(b);
		nn += m;
		if(nn == n)
			break;
		off = 0;
		a += blockSize;
	}
	return n;
}

/*
 * write some data to the clump section at a given offset
 * used to fix up corrupted arenas.
 */
u32int
writeArena(Arena *arena, u64int aa, u8int *clbuf, u32int n)
{
	DBlock *b;
	u64int a;
	u32int blockSize, off, m;
	long nn;
	int ok;

	if(n == 0)
		return 0;

	vtLock(arena->lock);
	a = arena->size - arenaDirSize(arena, arena->clumps);
	if(aa >= a || aa + n > a){
		vtUnlock(arena->lock);
		setErr(EOk, "writing beyond arena clump storage");
		return 0;
	}

	blockSize = arena->blockSize;
	a = arena->base + aa;
	off = a & (blockSize - 1);
	a -= off;
	nn = 0;
	for(;;){
		b = getDBlock(arena->part, a, off != 0 || off + n < blockSize);
		if(b == nil){
			vtUnlock(arena->lock);
			return 0;
		}
		m = blockSize - off;
		if(m > n - nn)
			m = n - nn;
		memmove(&b->data[off], &clbuf[nn], m);
		ok = writePart(arena->part, a, b->data, blockSize);
		putDBlock(b);
		if(!ok){
			vtUnlock(arena->lock);
			return 0;
		}
		nn += m;
		if(nn == n)
			break;
		off = 0;
		a += blockSize;
	}
	vtUnlock(arena->lock);
	return n;
}

/*
 * allocate space for the clump and write it,
 * updating the arena directory
ZZZ question: should this distinguish between an arena
filling up and real errors writing the clump?
 */
u64int
writeAClump(Arena *arena, Clump *c, u8int *clbuf)
{
	DBlock *b;
	u64int a, aa;
	u32int clump, n, nn, m, off, blockSize;
	int ok;

	n = c->info.size + ClumpSize;
	vtLock(arena->lock);
	aa = arena->used;
	if(arena->sealed
	|| aa + n + U32Size + arenaDirSize(arena, arena->clumps + 1) > arena->size){
		if(!arena->sealed)
			sealArena(arena);
		vtUnlock(arena->lock);
		return TWID64;
	}
	if(!packClump(c, &clbuf[0])){
		vtUnlock(arena->lock);
		return TWID64;
	}

	/*
	 * write the data out one block at a time
	 */
	blockSize = arena->blockSize;
	a = arena->base + aa;
	off = a & (blockSize - 1);
	a -= off;
	nn = 0;
	for(;;){
		b = getDBlock(arena->part, a, off != 0);
		if(b == nil){
			vtUnlock(arena->lock);
			return TWID64;
		}
		m = blockSize - off;
		if(m > n - nn)
			m = n - nn;
		memmove(&b->data[off], &clbuf[nn], m);
		ok = writePart(arena->part, a, b->data, blockSize);
		putDBlock(b);
		if(!ok){
			vtUnlock(arena->lock);
			return TWID64;
		}
		nn += m;
		if(nn == n)
			break;
		off = 0;
		a += blockSize;
	}

	arena->used += c->info.size + ClumpSize;
	arena->uncsize += c->info.uncsize;
	if(c->info.size < c->info.uncsize)
		arena->cclumps++;

	clump = arena->clumps++;
	if(arena->clumps == 0)
		fatal("clumps wrapped\n");
	arena->wtime = now();
	if(arena->ctime == 0)
		arena->ctime = arena->wtime;

	writeClumpInfo(arena, clump, &c->info);
//ZZZ make this an enum param
	if((clump & 0x1ff) == 0x1ff){
		flushCIBlocks(arena);
		wbArena(arena);
	}

	vtUnlock(arena->lock);
	return aa;
}

/*
 * once sealed, an arena never has any data added to it.
 * it should only be changed to fix errors.
 * this also syncs the clump directory.
 */
static void
sealArena(Arena *arena)
{
	flushCIBlocks(arena);
	arena->sealed = 1;
	wbArena(arena);
	backSumArena(arena);
}

void
backSumArena(Arena *arena)
{
	ASum *as;

	if(sumLock == nil)
		return;
	as = MK(ASum);
	if(as == nil)
		return;
	vtLock(sumLock);
	as->arena = arena;
	as->next = sumq;
	sumq = as;
	vtWakeup(sumWait);
	vtUnlock(sumLock);
}

static void
doASum(void *unused)
{
	ASum *as;
	Arena *arena;

	if(unused){;}

	for(;;){
		vtLock(sumLock);
		while(sumq == nil)
			vtSleep(sumWait);
		as = sumq;
		sumq = as->next;
		vtUnlock(sumLock);
		arena = as->arena;
		free(as);

		sumArena(arena);
	}
}

void
sumArena(Arena *arena)
{
	ZBlock *b;
	VtSha1 *s;
	u64int a, e;
	u32int bs;
	u8int score[VtScoreSize];

	bs = MaxIoSize;
	if(bs < arena->blockSize)
		bs = arena->blockSize;

	s = vtSha1Alloc();
	if(s == nil){
		logErr(EOk, "sumArena can't initialize sha1 state");
		return;
	}

	/*
	 * read & sum all blocks except the last one
	 */
	vtSha1Init(s);
	b = allocZBlock(bs, 0);
	e = arena->base + arena->size;
	for(a = arena->base - arena->blockSize; a + arena->blockSize <= e; a += bs){
		if(a + bs > e)
			bs = arena->blockSize;
		if(!readPart(arena->part, a, b->data, bs))
			goto ReadErr;
		vtSha1Update(s, b->data, bs);
	}

	/*
	 * the last one is special, since it may already have the checksum included
	 */
	bs = arena->blockSize;
	if(!readPart(arena->part, e, b->data, bs)){
ReadErr:
		logErr(EOk, "sumArena can't sum %s, read at %lld failed: %r", arena->name, a);
		freeZBlock(b);
		vtSha1Free(s);
		return;
	}

	vtSha1Update(s, b->data, bs - VtScoreSize);
	vtSha1Update(s, zeroScore, VtScoreSize);
	vtSha1Final(s, score);
	vtSha1Free(s);

	/*
	 * check for no checksum or the same
	 */
	if(!scoreEq(score, &b->data[bs - VtScoreSize])){
		if(!scoreEq(zeroScore, &b->data[bs - VtScoreSize]))
			logErr(EOk, "overwriting mismatched checksums for arena=%s, found=%V calculated=%V",
				arena->name, &b->data[bs - VtScoreSize], score);
		scoreCp(&b->data[bs - VtScoreSize], score);
		if(!writePart(arena->part, e, b->data, bs))
			logErr(EOk, "sumArena can't write sum for %s: %r", arena->name);
	}
	freeZBlock(b);

	vtLock(arena->lock);
	scoreCp(arena->score, score);
	vtUnlock(arena->lock);
}

/*
 * write the arena trailer block to the partition
 */
int
wbArena(Arena *arena)
{
	ZBlock *b;
	int ok;

	b = allocZBlock(arena->blockSize, 1);
	if(b == nil){
		logErr(EAdmin, "can't write arena trailer: %R");
///ZZZ add error message?
		return 0;
	}
	ok = okArena(arena) && packArena(arena, b->data)
		&& writePart(arena->part, arena->base + arena->size, b->data, arena->blockSize);
	freeZBlock(b);
	return ok;
}

int
wbArenaHead(Arena *arena)
{
	ZBlock *b;
	ArenaHead head;
	int ok;

	nameCp(head.name, arena->name);
	head.version = arena->version;
	head.size = arena->size + 2 * arena->blockSize;
	head.blockSize = arena->blockSize;
	b = allocZBlock(arena->blockSize, 1);
	if(b == nil){
		logErr(EAdmin, "can't write arena header: %R");
///ZZZ add error message?
		return 0;
	}
	ok = packArenaHead(&head, b->data)
		&& writePart(arena->part, arena->base - arena->blockSize, b->data, arena->blockSize);
	freeZBlock(b);
	return ok;
}

/*
 * read the arena header and trailer blocks from disk
 */
static int
loadArena(Arena *arena)
{
	ArenaHead head;
	ZBlock *b;

	b = allocZBlock(arena->blockSize, 0);
	if(b == nil)
		return 0;
	if(!readPart(arena->part, arena->base + arena->size, b->data, arena->blockSize)){
		freeZBlock(b);
		return 0;
	}
	if(!unpackArena(arena, b->data)){
		freeZBlock(b);
		return 0;
	}
	if(arena->version != ArenaVersion){
		setErr(EAdmin, "unknown arena version %d", arena->version);
		freeZBlock(b);
		return 0;
	}
	scoreCp(arena->score, &b->data[arena->blockSize - VtScoreSize]);

	if(!readPart(arena->part, arena->base - arena->blockSize, b->data, arena->blockSize)){
		logErr(EAdmin, "can't read arena header: %R");
		freeZBlock(b);
		return 1;
	}
	if(!unpackArenaHead(&head, b->data))
		logErr(ECorrupt, "corrupted arena header: %R");
	else if(!nameEq(arena->name, head.name)
	     || arena->version != head.version
	     || arena->blockSize != head.blockSize
	     || arena->size + 2 * arena->blockSize != head.size)
		logErr(ECorrupt, "arena header inconsistent with arena data");
	freeZBlock(b);

	return 1;
}

static int
okArena(Arena *arena)
{
	u64int dsize;
	int ok;

	ok = 1;
	dsize = arenaDirSize(arena, arena->clumps);
	if(arena->used + dsize > arena->size){
		setErr(ECorrupt, "arena used > size");
		ok = 0;
	}

	if(arena->cclumps > arena->clumps)
		logErr(ECorrupt, "arena has more compressed clumps than total clumps");

	if(arena->uncsize + arena->clumps * ClumpSize + arena->blockSize < arena->used)
		logErr(ECorrupt, "arena uncompressed size inconsistent with used space %lld %d %lld", arena->uncsize, arena->clumps, arena->used);

	if(arena->ctime > arena->wtime)
		logErr(ECorrupt, "arena creation time after last write time");

	return ok;
}

static CIBlock*
getCIB(Arena *arena, int clump, int writing, CIBlock *rock)
{
	CIBlock *cib;
	u32int block, off;

	if(clump >= arena->clumps){
		setErr(EOk, "clump directory access out of range");
		return nil;
	}
	block = clump / arena->clumpMax;
	off = (clump - block * arena->clumpMax) * ClumpInfoSize;

	if(arena->cib.block == block
	&& arena->cib.data != nil){
		arena->cib.offset = off;
		return &arena->cib;
	}

	if(writing){
		flushCIBlocks(arena);
		cib = &arena->cib;
	}else
		cib = rock;

	vtLock(stats.lock);
	stats.ciReads++;
	vtUnlock(stats.lock);

	cib->block = block;
	cib->offset = off;
	cib->data = getDBlock(arena->part, arena->base + arena->size - (block + 1) * arena->blockSize, arena->blockSize);
	if(cib->data == nil)
		return nil;
	return cib;
}

static void
putCIB(Arena *arena, CIBlock *cib)
{
	if(cib != &arena->cib){
		putDBlock(cib->data);
		cib->data = nil;
	}
}

/*
 * must be called with arena locked
 */
int
flushCIBlocks(Arena *arena)
{
	int ok;

	if(arena->cib.data == nil)
		return 1;
	vtLock(stats.lock);
	stats.ciWrites++;
	vtUnlock(stats.lock);
	ok = writePart(arena->part, arena->base + arena->size - (arena->cib.block + 1) * arena->blockSize, arena->cib.data->data, arena->blockSize);

	if(!ok)
		setErr(EAdmin, "failed writing arena directory block");
	putDBlock(arena->cib.data);
	arena->cib.data = nil;
	return ok;
}
