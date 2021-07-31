#include "stdinc.h"
#include "dat.h"
#include "fns.h"

typedef struct DCache	DCache;

enum
{
	HashLog		= 9,
	HashSize	= 1<<HashLog,
	HashMask	= HashSize - 1,
};

struct DCache
{
	VtLock		*lock;
	VtRendez	*full;
	DBlock		*free;			/* list of available lumps */
	u32int		now;			/* ticks for usage timestamps */
	int		size;			/* max. size of any block; allocated to each block */
	DBlock		**heads;		/* hash table for finding address */
	int		nheap;			/* number of available victims */
	DBlock		**heap;			/* heap for locating victims */
	int		nblocks;		/* number of blocks allocated */
	DBlock		*blocks;		/* array of block descriptors */
	u8int		*mem;			/* memory for all block descriptors */
};

static DCache	dCache;

static int	downHeap(int i, DBlock *b);
static int	upHeap(int i, DBlock *b);
static DBlock	*bumpDBlock(void);
static void	delHeap(DBlock *db);
static void	fixHeap(int i, DBlock *b);

void
initDCache(u32int mem)
{
	DBlock *b, *last;
	u32int nblocks, blockSize;
	int i;

	if(mem < maxBlockSize * 2)
		fatal("need at least %d bytes for the disk cache", maxBlockSize * 2);
	if(maxBlockSize == 0)
		fatal("no max. block size given for disk cache");
	blockSize = maxBlockSize;
	nblocks = mem / blockSize;
	if(0)
		fprint(2, "initialize disk cache with %d blocks of %d bytes\n", nblocks, blockSize);
	dCache.lock = vtLockAlloc();
	dCache.full = vtRendezAlloc(dCache.lock);
	dCache.nblocks = nblocks;
	dCache.size = blockSize;
	dCache.heads = MKNZ(DBlock*, HashSize);
	dCache.heap = MKNZ(DBlock*, nblocks);
	dCache.blocks = MKNZ(DBlock, nblocks);
	dCache.mem = MKNZ(u8int, nblocks * blockSize);

	last = nil;
	for(i = 0; i < nblocks; i++){
		b = &dCache.blocks[i];
		b->data = &dCache.mem[i * blockSize];
		b->heap = TWID32;
		b->lock = vtLockAlloc();
		b->next = last;
		last = b;
	}
	dCache.free = last;
	dCache.nheap = 0;
}

static u32int
pbHash(u64int addr)
{
	u32int h;

#define hashit(c)	((((c) * 0x6b43a9b5) >> (32 - HashLog)) & HashMask)
	h = (addr >> 32) ^ addr;
	return hashit(h);
}

DBlock*
getDBlock(Part *part, u64int addr, int read)
{
	DBlock *b;
	u32int h, size;

	size = part->blockSize;
	if(size > dCache.size){
		setErr(EAdmin, "block size %d too big for cache", size);
		return nil;
	}
	h = pbHash(addr);

	/*
	 * look for the block in the cache
	 */
//checkDCache();
	vtLock(dCache.lock);
again:
	for(b = dCache.heads[h]; b != nil; b = b->next){
		if(b->part == part && b->addr == addr){
			vtLock(stats.lock);
			stats.pcHit++;
			vtUnlock(stats.lock);
			goto found;
		}
	}
	vtLock(stats.lock);
	stats.pcMiss++;
	vtUnlock(stats.lock);

	/*
	 * missed: locate the block with the oldest second to last use.
	 * remove it from the heap, and fix up the heap.
	 */
	b = bumpDBlock();
	if(b == nil){
		logErr(EAdmin, "all disk cache blocks in use");
		vtSleep(dCache.full);
		goto again;
	}

	/*
	 * the new block has no last use, so assume it happens sometime in the middle
ZZZ this is not reasonable
	 */
	b->used = (b->used2 + dCache.now) / 2;

	/*
	 * rechain the block on the correct hash chain
	 */
	b->next = dCache.heads[h];
	dCache.heads[h] = b;
	if(b->next != nil)
		b->next->prev = b;
	b->prev = nil;

	b->addr = addr;
	b->part = part;
	b->size = 0;

found:
	b->ref++;
	b->used2 = b->used;
	b->used = dCache.now++;
	if(b->heap != TWID32)
		fixHeap(b->heap, b);

	vtUnlock(dCache.lock);
//checkDCache();

	vtLock(b->lock);
	if(b->size != size){
		if(b->size < size){
			if(!read)
				memset(&b->data[b->size], 0, size - b->size);
			else if(readPart(part, addr + b->size, &b->data[b->size], size - b->size)){
				vtLock(stats.lock);
				stats.pcReads++;
				stats.pcBReads += size - b->size;
				vtUnlock(stats.lock);
			}else{
				putDBlock(b);
				return nil;
			}
		}
		b->size = size;
	}

	return b;
}

void
putDBlock(DBlock *b)
{
	if(b == nil)
		return;

	vtUnlock(b->lock);
//checkDCache();
	vtLock(dCache.lock);
	if(--b->ref == 0){
		if(b->heap == TWID32)
			upHeap(dCache.nheap++, b);
		vtWakeup(dCache.full);
	}

	vtUnlock(dCache.lock);
//checkDCache();
}

/*
 * remove some block from use and update the free list and counters
 */
static DBlock*
bumpDBlock(void)
{
	DBlock *b;
	ulong h;

	b = dCache.free;
	if(b != nil){
		dCache.free = b->next;
		return b;
	}

	/*
	 * remove blocks until we find one that is unused
	 * referenced blocks are left in the heap even though
	 * they can't be scavenged; this is simple a speed optimization
	 */
	for(;;){
		if(dCache.nheap == 0)
			return nil;
		b = dCache.heap[0];
		delHeap(b);
		if(!b->ref)
			break;
	}

	/*
	 * unchain the block
	 */
	if(b->prev == nil){
		h = pbHash(b->addr);
		if(dCache.heads[h] != b)
			fatal("bad hash chains in disk cache");
		dCache.heads[h] = b->next;
	}else
		b->prev->next = b->next;
	if(b->next != nil)
		b->next->prev = b->prev;

	return b;
}

/*
 * delete an arbitrary block from the heap
 */
static void
delHeap(DBlock *db)
{
	fixHeap(db->heap, dCache.heap[--dCache.nheap]);
	db->heap = TWID32;
}

/*
 * push an element up or down to it's correct new location
 */
static void
fixHeap(int i, DBlock *b)
{
	if(upHeap(i, b) == i)
		downHeap(i, b);
}

static int
upHeap(int i, DBlock *b)
{
	DBlock *bb;
	u32int now;
	int p;

	now = dCache.now;
	for(; i != 0; i = p){
		p = (i - 1) >> 1;
		bb = dCache.heap[p];
		if(b->used2 - now >= bb->used2 - now)
			break;
		dCache.heap[i] = bb;
		bb->heap = i;
	}

	dCache.heap[i] = b;
	b->heap = i;
	return i;
}

static int
downHeap(int i, DBlock *b)
{
	DBlock *bb;
	u32int now;
	int k;

	now = dCache.now;
	for(; ; i = k){
		k = (i << 1) + 1;
		if(k >= dCache.nheap)
			break;
		if(k + 1 < dCache.nheap && dCache.heap[k]->used2 - now > dCache.heap[k + 1]->used2 - now)
			k++;
		bb = dCache.heap[k];
		if(b->used2 - now <= bb->used2 - now)
			break;
		dCache.heap[i] = bb;
		bb->heap = i;
	}

	dCache.heap[i] = b;
	b->heap = i;
	return i;
}

static void
findBlock(DBlock *bb)
{
	DBlock *b, *last;
	int h;

	last = nil;
	h = pbHash(bb->addr);
	for(b = dCache.heads[h]; b != nil; b = b->next){
		if(last != b->prev)
			fatal("bad prev link");
		if(b == bb)
			return;
		last = b;
	}
	fatal("block missing from hash table");
}

void
checkDCache(void)
{
	DBlock *b;
	u32int size, now;
	int i, k, refed, nfree;

	vtLock(dCache.lock);
	size = dCache.size;
	now = dCache.now;
	for(i = 0; i < dCache.nheap; i++){
		if(dCache.heap[i]->heap != i)
			fatal("dc: mis-heaped at %d: %d", i, dCache.heap[i]->heap);
		if(i > 0 && dCache.heap[(i - 1) >> 1]->used2 - now > dCache.heap[i]->used2 - now)
			fatal("dc: bad heap ordering");
		k = (i << 1) + 1;
		if(k < dCache.nheap && dCache.heap[i]->used2 - now > dCache.heap[k]->used2 - now)
			fatal("dc: bad heap ordering");
		k++;
		if(k < dCache.nheap && dCache.heap[i]->used2 - now > dCache.heap[k]->used2 - now)
			fatal("dc: bad heap ordering");
	}

	refed = 0;
	for(i = 0; i < dCache.nblocks; i++){
		b = &dCache.blocks[i];
		if(b->data != &dCache.mem[i * size])
			fatal("dc: mis-blocked at %d", i);
		if(b->ref && b->heap == TWID32)
			refed++;
		if(b->addr)
			findBlock(b);
		if(b->heap != TWID32
		&& dCache.heap[b->heap] != b)
			fatal("dc: spurious heap value");
	}

	nfree = 0;
	for(b = dCache.free; b != nil; b = b->next){
		if(b->addr != 0 || b->heap != TWID32)
			fatal("dc: bad free list");
		nfree++;
	}

	if(dCache.nheap + nfree + refed != dCache.nblocks)
		fatal("dc: missing blocks: %d %d %d", dCache.nheap, refed, dCache.nblocks);
	vtUnlock(dCache.lock);
}
