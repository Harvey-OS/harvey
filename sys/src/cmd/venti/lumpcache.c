#include "stdinc.h"
#include "dat.h"
#include "fns.h"

typedef struct LumpCache	LumpCache;

enum
{
	HashLog		= 9,
	HashSize	= 1<<HashLog,
	HashMask	= HashSize - 1,
};

struct LumpCache
{
	VtLock		*lock;
	VtRendez	*full;
	Lump		*free;			/* list of available lumps */
	u32int		allowed;		/* total allowable space for packets */
	u32int		avail;			/* remaining space for packets */
	u32int		now;			/* ticks for usage timestamps */
	Lump		**heads;		/* hash table for finding address */
	int		nheap;			/* number of available victims */
	Lump		**heap;			/* heap for locating victims */
	int		nblocks;		/* number of blocks allocated */
	Lump		*blocks;		/* array of block descriptors */
};

static LumpCache	lumpCache;

static void	delHeap(Lump *db);
static int	downHeap(int i, Lump *b);
static void	fixHeap(int i, Lump *b);
static int	upHeap(int i, Lump *b);
static Lump	*bumpLump(void);

void
initLumpCache(u32int size, u32int nblocks)
{
	Lump *last, *b;
	int i;

	lumpCache.lock = vtLockAlloc();
	lumpCache.full = vtRendezAlloc(lumpCache.lock);
	lumpCache.nblocks = nblocks;
	lumpCache.allowed = size;
	lumpCache.avail = size;
	lumpCache.heads = MKNZ(Lump*, HashSize);
	lumpCache.heap = MKNZ(Lump*, nblocks);
	lumpCache.blocks = MKNZ(Lump, nblocks);

	last = nil;
	for(i = 0; i < nblocks; i++){
		b = &lumpCache.blocks[i];
		b->type = TWID8;
		b->heap = TWID32;
		b->lock = vtLockAlloc();
		b->next = last;
		last = b;
	}
	lumpCache.free = last;
	lumpCache.nheap = 0;
}

Lump*
lookupLump(u8int *score, int type)
{
	Lump *b;
	u32int h;

	h = hashBits(score, HashLog);

	/*
	 * look for the block in the cache
	 */
//checkLumpCache();
	vtLock(lumpCache.lock);
again:
	for(b = lumpCache.heads[h]; b != nil; b = b->next){
		if(scoreEq(score, b->score) && type == b->type){
			vtLock(stats.lock);
			stats.lumpHit++;
			vtUnlock(stats.lock);
			goto found;
		}
	}

	/*
	 * missed: locate the block with the oldest second to last use.
	 * remove it from the heap, and fix up the heap.
	 */
	while(lumpCache.free == nil){
		if(bumpLump() == nil){
			logErr(EAdmin, "all lump cache blocks in use");
			vtSleep(lumpCache.full);
			goto again;
		}
	}
	vtLock(stats.lock);
	stats.lumpMiss++;
	vtUnlock(stats.lock);

	b = lumpCache.free;
	lumpCache.free = b->next;

	/*
	 * the new block has no last use, so assume it happens sometime in the middle
ZZZ this is not reasonable
	 */
	b->used = (b->used2 + lumpCache.now) / 2;

	/*
	 * rechain the block on the correct hash chain
	 */
	b->next = lumpCache.heads[h];
	lumpCache.heads[h] = b;
	if(b->next != nil)
		b->next->prev = b;
	b->prev = nil;

	scoreCp(b->score, score);
	b->type = type;
	b->size = 0;
	b->data = nil;

found:
	b->ref++;
	b->used2 = b->used;
	b->used = lumpCache.now++;
	if(b->heap != TWID32)
		fixHeap(b->heap, b);
	vtUnlock(lumpCache.lock);

//checkLumpCache();

	vtLock(b->lock);

	return b;
}

void
insertLump(Lump *b, Packet *p)
{
	u32int size;

	/*
	 * look for the block in the cache
	 */
//checkLumpCache();
	vtLock(lumpCache.lock);
again:

	/*
	 * missed: locate the block with the oldest second to last use.
	 * remove it from the heap, and fix up the heap.
	 */
	size = packetAllocatedSize(p);
//ZZZ
	while(lumpCache.avail < size){
		if(bumpLump() == nil){
			logErr(EAdmin, "all lump cache blocks in use");
			vtSleep(lumpCache.full);
			goto again;
		}
	}
	b->data = p;
	b->size = size;
	lumpCache.avail -= size;

	vtUnlock(lumpCache.lock);
//checkLumpCache();
}

void
putLump(Lump *b)
{
	if(b == nil)
		return;

	vtUnlock(b->lock);
//checkLumpCache();
	vtLock(lumpCache.lock);
	if(--b->ref == 0){
		if(b->heap == TWID32)
			upHeap(lumpCache.nheap++, b);
		vtWakeup(lumpCache.full);
	}

	vtUnlock(lumpCache.lock);
//checkLumpCache();
}

/*
 * remove some lump from use and update the free list and counters
 */
static Lump*
bumpLump(void)
{
	Lump *b;
	u32int h;

	/*
	 * remove blocks until we find one that is unused
	 * referenced blocks are left in the heap even though
	 * they can't be scavenged; this is simple a speed optimization
	 */
	for(;;){
		if(lumpCache.nheap == 0)
			return nil;
		b = lumpCache.heap[0];
		delHeap(b);
		if(!b->ref){
			vtWakeup(lumpCache.full);
			break;
		}
	}

	/*
	 * unchain the block
	 */
	if(b->prev == nil){
		h = hashBits(b->score, HashLog);
		if(lumpCache.heads[h] != b)
			fatal("bad hash chains in lump cache");
		lumpCache.heads[h] = b->next;
	}else
		b->prev->next = b->next;
	if(b->next != nil)
		b->next->prev = b->prev;

	if(b->data != nil){
		packetFree(b->data);
		b->data = nil;
		lumpCache.avail += b->size;
		b->size = 0;
	}
	b->type = TWID8;

	b->next = lumpCache.free;
	lumpCache.free = b;

	return b;
}

/*
 * delete an arbitrary block from the heap
 */
static void
delHeap(Lump *db)
{
	fixHeap(db->heap, lumpCache.heap[--lumpCache.nheap]);
	db->heap = TWID32;
}

/*
 * push an element up or down to it's correct new location
 */
static void
fixHeap(int i, Lump *b)
{
	if(upHeap(i, b) == i)
		downHeap(i, b);
}

static int
upHeap(int i, Lump *b)
{
	Lump *bb;
	u32int now;
	int p;

	now = lumpCache.now;
	for(; i != 0; i = p){
		p = (i - 1) >> 1;
		bb = lumpCache.heap[p];
		if(b->used2 - now >= bb->used2 - now)
			break;
		lumpCache.heap[i] = bb;
		bb->heap = i;
	}

	lumpCache.heap[i] = b;
	b->heap = i;
	return i;
}

static int
downHeap(int i, Lump *b)
{
	Lump *bb;
	u32int now;
	int k;

	now = lumpCache.now;
	for(; ; i = k){
		k = (i << 1) + 1;
		if(k >= lumpCache.nheap)
			break;
		if(k + 1 < lumpCache.nheap && lumpCache.heap[k]->used2 - now > lumpCache.heap[k + 1]->used2 - now)
			k++;
		bb = lumpCache.heap[k];
		if(b->used2 - now <= bb->used2 - now)
			break;
		lumpCache.heap[i] = bb;
		bb->heap = i;
	}

	lumpCache.heap[i] = b;
	b->heap = i;
	return i;
}

static void
findBlock(Lump *bb)
{
	Lump *b, *last;
	int h;

	last = nil;
	h = hashBits(bb->score, HashLog);
	for(b = lumpCache.heads[h]; b != nil; b = b->next){
		if(last != b->prev)
			fatal("bad prev link");
		if(b == bb)
			return;
		last = b;
	}
	fatal("block score=%V type=%#x missing from hash table", bb->score, bb->type);
}

void
checkLumpCache(void)
{
	Lump *b;
	u32int size, now, nfree;
	int i, k, refed;

	vtLock(lumpCache.lock);
	now = lumpCache.now;
	for(i = 0; i < lumpCache.nheap; i++){
		if(lumpCache.heap[i]->heap != i)
			fatal("lc: mis-heaped at %d: %d", i, lumpCache.heap[i]->heap);
		if(i > 0 && lumpCache.heap[(i - 1) >> 1]->used2 - now > lumpCache.heap[i]->used2 - now)
			fatal("lc: bad heap ordering");
		k = (i << 1) + 1;
		if(k < lumpCache.nheap && lumpCache.heap[i]->used2 - now > lumpCache.heap[k]->used2 - now)
			fatal("lc: bad heap ordering");
		k++;
		if(k < lumpCache.nheap && lumpCache.heap[i]->used2 - now > lumpCache.heap[k]->used2 - now)
			fatal("lc: bad heap ordering");
	}

	refed = 0;
	size = 0;
	for(i = 0; i < lumpCache.nblocks; i++){
		b = &lumpCache.blocks[i];
		if(b->data == nil && b->size != 0)
			fatal("bad size: %d data=%p", b->size, b->data);
		if(b->ref && b->heap == TWID32)
			refed++;
		if(b->type != TWID8){
			findBlock(b);
			size += b->size;
		}
		if(b->heap != TWID32
		&& lumpCache.heap[b->heap] != b)
			fatal("lc: spurious heap value");
	}
	if(lumpCache.avail != lumpCache.allowed - size)
		fatal("mismatched available=%d and allowed=%d - used=%d space", lumpCache.avail, lumpCache.allowed, size);

	nfree = 0;
	for(b = lumpCache.free; b != nil; b = b->next){
		if(b->type != TWID8 || b->heap != TWID32)
			fatal("lc: bad free list");
		nfree++;
	}

	if(lumpCache.nheap + nfree + refed != lumpCache.nblocks)
		fatal("lc: missing blocks: %d %d %d %d", lumpCache.nheap, refed, nfree, lumpCache.nblocks);
	vtUnlock(lumpCache.lock);
}
