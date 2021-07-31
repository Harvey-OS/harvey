/*
 * Disk cache.
 * 
 * Caches raw disk blocks.  Getdblock() gets a block, putdblock puts it back.
 * Getdblock has a mode parameter that determines i/o and access to a block:
 * if mode is OREAD or ORDWR, it is read from disk if not already in memory.
 * If mode is ORDWR or OWRITE, it is locked for exclusive use before being returned.
 * It is *not* marked dirty -- once changes have been made, they should be noted
 * by using dirtydblock() before putdblock().  
 *
 * There is a global cache lock as well as a lock on each block. 
 * Within a thread, the cache lock can be acquired while holding a block lock,
 * but not vice versa; and a block cannot be locked if you already hold the lock
 * on another block.
 * 
 * The flush proc writes out dirty blocks in batches, one batch per dirty tag.
 * For example, the DirtyArena blocks are all written to disk before any of the
 * DirtyArenaCib blocks.
 *
 * This code used to be in charge of flushing the dirty index blocks out to 
 * disk, but updating the index turned out to benefit from extra care.
 * Now cached index blocks are never marked dirty.  The index.c code takes
 * care of updating them behind our back, and uses _getdblock to update any
 * cached copies of the blocks as it changes them on disk.
 */

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
	QLock		lock;
	RWLock		dirtylock;		/* must be held to inspect or set b->dirty */
	Rendez		full;
	Round		round;
	DBlock		*free;			/* list of available lumps */
	u32int		now;			/* ticks for usage timestamps */
	int		size;			/* max. size of any block; allocated to each block */
	DBlock		**heads;		/* hash table for finding address */
	int		nheap;			/* number of available victims */
	DBlock		**heap;			/* heap for locating victims */
	int		nblocks;		/* number of blocks allocated */
	DBlock		*blocks;		/* array of block descriptors */
	DBlock		**write;		/* array of block pointers to be written */
	u8int		*mem;			/* memory for all block descriptors */
	int		ndirty;			/* number of dirty blocks */
	int		maxdirty;		/* max. number of dirty blocks */
	Channel	*ra;
	u8int		*rabuf;
	u32int		ramax;
	u32int		rasize;
	u64int		raaddr;
	Part		*rapart;

	AState	diskstate;
	AState	state;
};

typedef struct Ra Ra;
struct Ra
{
	Part *part;
	u64int addr;
};

static DCache	dcache;

static int	downheap(int i, DBlock *b);
static int	upheap(int i, DBlock *b);
static DBlock	*bumpdblock(void);
static void	delheap(DBlock *db);
static void	fixheap(int i, DBlock *b);
static void	flushproc(void*);
static void	writeproc(void*);
static void raproc(void*);

void
initdcache(u32int mem)
{
	DBlock *b, *last;
	u32int nblocks, blocksize;
	int i;
	u8int *p;

	if(mem < maxblocksize * 2)
		sysfatal("need at least %d bytes for the disk cache", maxblocksize * 2);
	if(maxblocksize == 0)
		sysfatal("no max. block size given for disk cache");
	blocksize = maxblocksize;
	nblocks = mem / blocksize;
	dcache.full.l = &dcache.lock;
	dcache.nblocks = nblocks;
	dcache.maxdirty = (nblocks * 2) / 3;
	trace(TraceProc, "initialize disk cache with %d blocks of %d bytes, maximum %d dirty blocks\n",
			nblocks, blocksize, dcache.maxdirty);
	dcache.size = blocksize;
	dcache.heads = MKNZ(DBlock*, HashSize);
	dcache.heap = MKNZ(DBlock*, nblocks);
	dcache.blocks = MKNZ(DBlock, nblocks);
	dcache.write = MKNZ(DBlock*, nblocks);
	dcache.mem = MKNZ(u8int, (nblocks+1+128) * blocksize);
	dcache.ra = chancreate(sizeof(Ra), 0);

	last = nil;
	p = (u8int*)(((ulong)dcache.mem+blocksize-1)&~(ulong)(blocksize-1));
	for(i = 0; i < nblocks; i++){
		b = &dcache.blocks[i];
		b->data = &p[i * blocksize];
		b->heap = TWID32;
		b->writedonechan = chancreate(sizeof(void*), 1);
		b->next = last;
		last = b;
	}
	dcache.rabuf = &p[i*blocksize];
	dcache.ramax = 128*blocksize;
	dcache.raaddr = 0;
	dcache.rapart = nil;

	dcache.free = last;
	dcache.nheap = 0;
	setstat(StatDcacheSize, nblocks);
	initround(&dcache.round, "dcache", 120*1000);

	vtproc(flushproc, nil);
	vtproc(delaykickroundproc, &dcache.round);
	vtproc(raproc, nil);
}

void
setdcachestate(AState *a)
{
	trace(TraceBlock, "setdcachestate %s 0x%llux clumps %d", a->arena ? a->arena->name : nil, a->aa, a->stats.clumps);
	qlock(&dcache.lock);
	dcache.state = *a;
	qunlock(&dcache.lock);
}

AState
diskstate(void)
{
	AState a;

	qlock(&dcache.lock);
	a = dcache.diskstate;
	qunlock(&dcache.lock);
	return a;
}

static void
raproc(void *v)
{
	Ra ra;
	DBlock *b;

	USED(v);
	while(recv(dcache.ra, &ra) == 1){
		if(ra.part->size <= ra.addr)
			continue;
		b = _getdblock(ra.part, ra.addr, OREAD, 2);
		putdblock(b);
	}
}

/*
 * We do readahead a whole arena at a time now,
 * so dreadahead is a no-op.  The original implementation
 * is in unused_dreadahead below.
 */
void
dreadahead(Part *part, u64int addr, int miss)
{
	USED(part);
	USED(addr);
	USED(miss);
}

void
unused_dreadahead(Part *part, u64int addr, int miss)
{
	Ra ra;
	static struct {
		Part *part;
		u64int addr;
	} lastmiss;
	static struct {
		Part *part;
		u64int addr;
		int dir;
	} lastra;

	if(miss){
		if(lastmiss.part==part && lastmiss.addr==addr-dcache.size){
		XRa:
			lastra.part = part;
			lastra.dir = addr-lastmiss.addr;
			lastra.addr = addr+lastra.dir;
			ra.part = part;
			ra.addr = lastra.addr;
			nbsend(dcache.ra, &ra);
		}else if(lastmiss.part==part && lastmiss.addr==addr+dcache.size){
			addr -= dcache.size;
			goto XRa;
		}
	}else{
		if(lastra.part==part && lastra.addr==addr){
			lastra.addr += lastra.dir;
			ra.part = part;
			ra.addr = lastra.addr;
			nbsend(dcache.ra, &ra);
		}
	}

	if(miss){
		lastmiss.part = part;
		lastmiss.addr = addr;
	}
}

int
rareadpart(Part *part, u64int addr, u8int *buf, uint n, int load)
{
	uint nn;
	static RWLock ralock;

	rlock(&ralock);
	if(dcache.rapart==part && dcache.raaddr <= addr && addr+n <= dcache.raaddr+dcache.rasize){
		memmove(buf, dcache.rabuf+(addr-dcache.raaddr), n);
		runlock(&ralock);
		return 0;
	}
	if(load != 2 || addr >= part->size){	/* addr >= part->size: let readpart do the error */	
		runlock(&ralock);
		diskaccess(0);
		return readpart(part, addr, buf, n);
	}

	runlock(&ralock);
	wlock(&ralock);
fprint(2, "raread %s %llx\n", part->name, addr);
	nn = dcache.ramax;
	if(addr+nn > part->size)
		nn = part->size - addr;
	diskaccess(0);
	if(readpart(part, addr, dcache.rabuf, nn) < 0){
		wunlock(&ralock);
		return -1;
	}
	memmove(buf, dcache.rabuf, n);	
	dcache.rapart = part;
	dcache.rasize = nn;
	dcache.raaddr = addr;
	wunlock(&ralock);

	addstat(StatApartReadBytes, nn-n);
	return 0;
}

static u32int
pbhash(u64int addr)
{
	u32int h;

#define hashit(c)	((((c) * 0x6b43a9b5) >> (32 - HashLog)) & HashMask)
	h = (addr >> 32) ^ addr;
	return hashit(h);
}

DBlock*
getdblock(Part *part, u64int addr, int mode)
{
	DBlock *b;
	uint ms;
	
	ms = msec();
	b = _getdblock(part, addr, mode, 1);
	if(mode == OREAD || mode == ORDWR)
		addstat(StatDcacheRead, 1);
	if(mode == OWRITE || mode == ORDWR)
		addstat(StatDcacheWrite, 1);
	ms = msec() - ms;
	addstat2(StatDcacheLookup, 1, StatDcacheLookupTime, ms);
	return b;
}

DBlock*
_getdblock(Part *part, u64int addr, int mode, int load)
{
	DBlock *b;
	u32int h, size;

	trace(TraceBlock, "getdblock enter %s 0x%llux", part->name, addr);
	size = part->blocksize;
	if(size > dcache.size){
		seterr(EAdmin, "block size %d too big for cache with size %d", size, dcache.size);
		return nil;
	}
	h = pbhash(addr);

	/*
	 * look for the block in the cache
	 */
	qlock(&dcache.lock);
again:
	for(b = dcache.heads[h]; b != nil; b = b->next){
		if(b->part == part && b->addr == addr){
			/*
			qlock(&stats.lock);
			stats.pchit++;
			qunlock(&stats.lock);
			*/
			if(load){
				addstat(StatDcacheHit, 1);
				if(load != 2 && mode != OWRITE)
					dreadahead(part, b->addr, 0);
			}
			goto found;
		}
	}

	/*
	 * missed: locate the block with the oldest second to last use.
	 * remove it from the heap, and fix up the heap.
	 */
	if(!load){
		qunlock(&dcache.lock);
		return nil;
	}

	addstat(StatDcacheMiss, 1);

	b = bumpdblock();
	if(b == nil){
		trace(TraceBlock, "all disk cache blocks in use");
		addstat(StatDcacheStall, 1);
		rsleep(&dcache.full);
		addstat(StatDcacheStall, -1);
		goto again;
	}

	assert(!b->dirty);

	/*
	 * the new block has no last use, so assume it happens sometime in the middle
ZZZ this is not reasonable
	 */
	b->used = (b->used2 + dcache.now) / 2;

	/*
	 * rechain the block on the correct hash chain
	 */
	b->next = dcache.heads[h];
	dcache.heads[h] = b;
	if(b->next != nil)
		b->next->prev = b;
	b->prev = nil;

	b->addr = addr;
	b->part = part;
	b->size = 0;
	if(load != 2 && mode != OWRITE)
		dreadahead(part, b->addr, 1);

found:
	b->ref++;
	b->used2 = b->used;
	b->used = dcache.now++;
	if(b->heap != TWID32)
		fixheap(b->heap, b);

	qunlock(&dcache.lock);

	trace(TraceBlock, "getdblock lock");
	addstat(StatDblockStall, 1);
	if(mode == OREAD)
		rlock(&b->lock);
	else
		wlock(&b->lock);
	addstat(StatDblockStall, -1);
	trace(TraceBlock, "getdblock locked");

	if(b->size != size){
		if(mode == OREAD){
			addstat(StatDblockStall, 1);
			runlock(&b->lock);
			wlock(&b->lock);
			addstat(StatDblockStall, -1);
		}
		if(b->size < size){
			if(mode == OWRITE)
				memset(&b->data[b->size], 0, size - b->size);
			else{
				trace(TraceBlock, "getdblock readpart %s 0x%llux", part->name, addr);
				if(rareadpart(part, addr + b->size, &b->data[b->size], size - b->size, load) < 0){
					b->mode = ORDWR;	/* so putdblock wunlocks */
					putdblock(b);
					return nil;
				}
				trace(TraceBlock, "getdblock readpartdone");
				addstat(StatApartRead, 1);
				addstat(StatApartReadBytes, size-b->size);
			}
		}
		b->size = size;
		if(mode == OREAD){
			addstat(StatDblockStall, 1);
			wunlock(&b->lock);
			rlock(&b->lock);
			addstat(StatDblockStall, -1);
		}
	}

	b->mode = mode;
	trace(TraceBlock, "getdblock exit");
	return b;
}

void
putdblock(DBlock *b)
{
	if(b == nil)
		return;

	trace(TraceBlock, "putdblock %s 0x%llux", b->part->name, b->addr);

	if(b->mode == OREAD)
		runlock(&b->lock);
	else
		wunlock(&b->lock);

	qlock(&dcache.lock);
	if(--b->ref == 0 && !b->dirty){
		if(b->heap == TWID32)
			upheap(dcache.nheap++, b);
		rwakeupall(&dcache.full);
	}
	qunlock(&dcache.lock);
}

void
dirtydblock(DBlock *b, int dirty)
{
	int odirty;
	Part *p;


	trace(TraceBlock, "dirtydblock enter %s 0x%llux %d from 0x%lux", b->part->name, b->addr, dirty, getcallerpc(&b));
	assert(b->ref != 0);
	assert(b->mode==ORDWR || b->mode==OWRITE);

	odirty = b->dirty;
	if(b->dirty)
		assert(b->dirty == dirty);
	else
		b->dirty = dirty;

	p = b->part;
	if(p->writechan == nil){
		trace(TraceBlock, "dirtydblock allocwriteproc %s", p->name);
		/* XXX hope this doesn't fail! */
		p->writechan = chancreate(sizeof(DBlock*), dcache.nblocks);
		vtproc(writeproc, p);
	}
	qlock(&dcache.lock);
	if(!odirty){
		dcache.ndirty++;
		setstat(StatDcacheDirty, dcache.ndirty);
		if(dcache.ndirty >= dcache.maxdirty)
			kickround(&dcache.round, 0);
		else
			delaykickround(&dcache.round);
	}
	qunlock(&dcache.lock);
}

static void
unchain(DBlock *b)
{
	ulong h;
	
	/*
	 * unchain the block
	 */
	if(b->prev == nil){
		h = pbhash(b->addr);
		if(dcache.heads[h] != b)
			sysfatal("bad hash chains in disk cache");
		dcache.heads[h] = b->next;
	}else
		b->prev->next = b->next;
	if(b->next != nil)
		b->next->prev = b->prev;
}

/*
 * remove some block from use and update the free list and counters
 */
static DBlock*
bumpdblock(void)
{
	DBlock *b;

	trace(TraceBlock, "bumpdblock enter");
	b = dcache.free;
	if(b != nil){
		dcache.free = b->next;
		return b;
	}

	if(dcache.ndirty >= dcache.maxdirty)
		kickdcache();

	/*
	 * remove blocks until we find one that is unused
	 * referenced blocks are left in the heap even though
	 * they can't be scavenged; this is simple a speed optimization
	 */
	for(;;){
		if(dcache.nheap == 0){
			kickdcache();
			trace(TraceBlock, "bumpdblock gotnothing");
			return nil;
		}
		b = dcache.heap[0];
		delheap(b);
		if(!b->ref && !b->dirty)
			break;
	}

	trace(TraceBlock, "bumpdblock bumping %s 0x%llux", b->part->name, b->addr);

	unchain(b);
	return b;
}

void
emptydcache(void)
{
	DBlock *b;
	
	qlock(&dcache.lock);
	while(dcache.nheap > 0){
		b = dcache.heap[0];
		delheap(b);
		if(!b->ref && !b->dirty){
			unchain(b);
			b->next = dcache.free;
			dcache.free = b;
		}
	}
	qunlock(&dcache.lock);
}

/*
 * delete an arbitrary block from the heap
 */
static void
delheap(DBlock *db)
{
	if(db->heap == TWID32)
		return;
	fixheap(db->heap, dcache.heap[--dcache.nheap]);
	db->heap = TWID32;
}

/*
 * push an element up or down to it's correct new location
 */
static void
fixheap(int i, DBlock *b)
{
	if(upheap(i, b) == i)
		downheap(i, b);
}

static int
upheap(int i, DBlock *b)
{
	DBlock *bb;
	u32int now;
	int p;

	now = dcache.now;
	for(; i != 0; i = p){
		p = (i - 1) >> 1;
		bb = dcache.heap[p];
		if(b->used2 - now >= bb->used2 - now)
			break;
		dcache.heap[i] = bb;
		bb->heap = i;
	}

	dcache.heap[i] = b;
	b->heap = i;
	return i;
}

static int
downheap(int i, DBlock *b)
{
	DBlock *bb;
	u32int now;
	int k;

	now = dcache.now;
	for(; ; i = k){
		k = (i << 1) + 1;
		if(k >= dcache.nheap)
			break;
		if(k + 1 < dcache.nheap && dcache.heap[k]->used2 - now > dcache.heap[k + 1]->used2 - now)
			k++;
		bb = dcache.heap[k];
		if(b->used2 - now <= bb->used2 - now)
			break;
		dcache.heap[i] = bb;
		bb->heap = i;
	}

	dcache.heap[i] = b;
	b->heap = i;
	return i;
}

static void
findblock(DBlock *bb)
{
	DBlock *b, *last;
	int h;

	last = nil;
	h = pbhash(bb->addr);
	for(b = dcache.heads[h]; b != nil; b = b->next){
		if(last != b->prev)
			sysfatal("bad prev link");
		if(b == bb)
			return;
		last = b;
	}
	sysfatal("block missing from hash table");
}

void
checkdcache(void)
{
	DBlock *b;
	u32int size, now;
	int i, k, refed, nfree;

	qlock(&dcache.lock);
	size = dcache.size;
	now = dcache.now;
	for(i = 0; i < dcache.nheap; i++){
		if(dcache.heap[i]->heap != i)
			sysfatal("dc: mis-heaped at %d: %d", i, dcache.heap[i]->heap);
		if(i > 0 && dcache.heap[(i - 1) >> 1]->used2 - now > dcache.heap[i]->used2 - now)
			sysfatal("dc: bad heap ordering");
		k = (i << 1) + 1;
		if(k < dcache.nheap && dcache.heap[i]->used2 - now > dcache.heap[k]->used2 - now)
			sysfatal("dc: bad heap ordering");
		k++;
		if(k < dcache.nheap && dcache.heap[i]->used2 - now > dcache.heap[k]->used2 - now)
			sysfatal("dc: bad heap ordering");
	}

	refed = 0;
	for(i = 0; i < dcache.nblocks; i++){
		b = &dcache.blocks[i];
		if(b->data != &dcache.mem[i * size])
			sysfatal("dc: mis-blocked at %d", i);
		if(b->ref && b->heap == TWID32)
			refed++;
		if(b->addr)
			findblock(b);
		if(b->heap != TWID32
		&& dcache.heap[b->heap] != b)
			sysfatal("dc: spurious heap value");
	}

	nfree = 0;
	for(b = dcache.free; b != nil; b = b->next){
		if(b->addr != 0 || b->heap != TWID32)
			sysfatal("dc: bad free list");
		nfree++;
	}

	if(dcache.nheap + nfree + refed != dcache.nblocks)
		sysfatal("dc: missing blocks: %d %d %d", dcache.nheap, refed, dcache.nblocks);
	qunlock(&dcache.lock);
}

void
flushdcache(void)
{
	trace(TraceProc, "flushdcache enter");
	kickround(&dcache.round, 1);
	trace(TraceProc, "flushdcache exit");
}

void
kickdcache(void)
{
	kickround(&dcache.round, 0);
}

static int
parallelwrites(DBlock **b, DBlock **eb, int dirty)
{
	DBlock **p, **q;
	Part *part;

	for(p=b; p<eb && (*p)->dirty == dirty; p++){
		assert(b<=p && p<eb);
		sendp((*p)->part->writechan, *p);
	}
	q = p;
	for(p=b; p<q; p++){
		assert(b<=p && p<eb);
		recvp((*p)->writedonechan);
	}
	
	/*
	 * Flush the partitions that have been written to.
	 */
	part = nil;
	for(p=b; p<q; p++){
		if(part != (*p)->part){
			part = (*p)->part;
			flushpart(part);	/* what if it fails? */
		}
	}

	return p-b;
}

/*
 * Sort first by dirty flag, then by partition, then by address in partition.
 */
static int
writeblockcmp(const void *va, const void *vb)
{
	DBlock *a, *b;

	a = *(DBlock**)va;
	b = *(DBlock**)vb;

	if(a->dirty != b->dirty)
		return a->dirty - b->dirty;
	if(a->part != b->part){
		if(a->part < b->part)
			return -1;
		if(a->part > b->part)
			return 1;
	}
	if(a->addr < b->addr)
		return -1;
	return 1;
}

static void
flushproc(void *v)
{
	int i, j, n;
	ulong t0;
	DBlock *b, **write;
	AState as;

	USED(v);
	threadsetname("flushproc");
	for(;;){
		waitforkick(&dcache.round);

		trace(TraceWork, "start");
		qlock(&dcache.lock);
		as = dcache.state;
		qunlock(&dcache.lock);

		t0 = nsec()/1000;

		trace(TraceProc, "build t=%lud", (ulong)(nsec()/1000)-t0);
		write = dcache.write;
		n = 0;
		for(i=0; i<dcache.nblocks; i++){
			b = &dcache.blocks[i];
			if(b->dirty)
				write[n++] = b;
		}

		qsort(write, n, sizeof(write[0]), writeblockcmp);

		/* Write each stage of blocks out. */
		trace(TraceProc, "writeblocks t=%lud", (ulong)(nsec()/1000)-t0);
		i = 0;
		for(j=1; j<DirtyMax; j++){
			trace(TraceProc, "writeblocks.%d t=%lud", j, (ulong)(nsec()/1000)-t0);
			i += parallelwrites(write+i, write+n, j);
		}
		if(i != n){
			fprint(2, "in flushproc i=%d n=%d\n", i, n);
			for(i=0; i<n; i++)
				fprint(2, "\tblock %d: dirty=%d\n", i, write[i]->dirty);
			abort();
		}

/* XXX
* the locking here is suspect.  what if a block is redirtied
* after the write happens?  we'll still decrement dcache.ndirty here.
*/
		trace(TraceProc, "undirty.%d t=%lud", j, (ulong)(nsec()/1000)-t0);
		qlock(&dcache.lock);
		dcache.diskstate = as;
		for(i=0; i<n; i++){
			b = write[i];
			--dcache.ndirty;
			if(b->ref == 0 && b->heap == TWID32){
				upheap(dcache.nheap++, b);
				rwakeupall(&dcache.full);
			}
		}
		setstat(StatDcacheDirty, dcache.ndirty);
		qunlock(&dcache.lock);
		addstat(StatDcacheFlush, 1);
		trace(TraceWork, "finish");
	}
}

static void
writeproc(void *v)
{
	DBlock *b;
	Part *p;

	p = v;

	threadsetname("writeproc:%s", p->name);
	for(;;){
		b = recvp(p->writechan);
		trace(TraceWork, "start");
		assert(b->part == p);
		trace(TraceProc, "wlock %s 0x%llux", p->name, b->addr);
		wlock(&b->lock);
		trace(TraceProc, "writepart %s 0x%llux", p->name, b->addr);
		diskaccess(0);
		if(writepart(p, b->addr, b->data, b->size) < 0)
			fprint(2, "write error: %r\n"); /* XXX details! */
		addstat(StatApartWrite, 1);
		addstat(StatApartWriteBytes, b->size);
		b->dirty = 0;
		wunlock(&b->lock);
		trace(TraceProc, "finish %s 0x%llux", p->name, b->addr);
		trace(TraceWork, "finish");
		sendp(b->writedonechan, b);
	}
}
