#include "stdinc.h"
#include "dat.h"
#include "fns.h"

struct ICache
{
	VtLock	*lock;			/* locks hash table & all associated data */
	IEntry	**heads;		/* heads of all the hash chains */
	int	bits;			/* bits to use for indexing heads */
	u32int	size;			/* number of heads; == 1 << bits, should be < entries */
	IEntry	*base;			/* all allocated hash table entries */
	u32int	entries;		/* elements in base */
	u32int	unused;			/* index of first unused element in base */
	u32int	stolen;			/* last head from which an element was stolen */
};

static ICache icache;

static IEntry	*icacheAlloc(int type, u8int *score);

/*
 * bits is the number of bits in the icache hash table
 * depth is the average depth
 * memory usage is about (1<<bits) * depth * sizeof(IEntry) + (1<<bits) * sizeof(IEntry*)
 */
void
initICache(int bits, int depth)
{
	icache.lock = vtLockAlloc();
	icache.bits = bits;
	icache.size = 1 << bits;
	icache.entries = depth * icache.size;
	icache.base = MKNZ(IEntry, icache.entries);
	icache.heads = MKNZ(IEntry*, icache.size);
}

u32int
hashBits(u8int *sc, int bits)
{
	u32int v;

	v = (sc[0] << 24) | (sc[1] << 16) | (sc[2] << 8) | sc[3];
	if(bits < 32)
		 v >>= (32 - bits);
	return v;
}

/*
ZZZ need to think about evicting the correct IEntry,
and writing back the wtime.
 * look up data score in the index cache
 * if this fails, pull it in from the disk index table, if it exists.
 *
 * must be called with the lump for this score locked
 */
int
lookupScore(u8int *score, int type, IAddr *ia)
{
	IEntry d, *ie, *last;
	u32int h;

	vtLock(stats.lock);
	stats.icLookups++;
	vtUnlock(stats.lock);

	vtLock(icache.lock);
	h = hashBits(score, icache.bits);
	last = nil;
	for(ie = icache.heads[h]; ie != nil; ie = ie->next){
		if(ie->ia.type == type && scoreEq(ie->score, score)){
			if(last != nil)
				last->next = ie->next;
			else
				icache.heads[h] = ie->next;
			vtLock(stats.lock);
			stats.icHits++;
			vtUnlock(stats.lock);
			goto found;
		}
		last = ie;
	}

	vtUnlock(icache.lock);

	if(!loadIEntry(mainIndex, score, type, &d))
		return 0;

	/*
	 * no one else can load an entry for this score,
	 * since we have the overall score lock.
	 */
	vtLock(stats.lock);
	stats.icFills++;
	vtUnlock(stats.lock);

	vtLock(icache.lock);

	ie = icacheAlloc(type, score);

	scoreCp(ie->score, score);
	ie->ia = d.ia;

found:
	ie->next = icache.heads[h];
	icache.heads[h] = ie;

	*ia = ie->ia;

	vtUnlock(icache.lock);

	return 1;
}

/*
 * insert a new element in the hash table.
 */
int
insertScore(u8int *score, IAddr *ia)
{
	IEntry *ie, se;
	u32int h;

	vtLock(stats.lock);
	stats.icInserts++;
	vtUnlock(stats.lock);

	vtLock(icache.lock);
	h = hashBits(score, icache.bits);

	ie = icacheAlloc(ia->type, score);

	scoreCp(ie->score, score);
	ie->ia = *ia;

	ie->next = icache.heads[h];
	icache.heads[h] = ie;

	se = *ie;

	vtUnlock(icache.lock);

	return storeIEntry(mainIndex, &se);
}

/*
 * allocate a index cache entry which hasn't been used in a while.
 * must be called with icache.lock locked
 * if the score is already in the table, update the entry.
 */
static IEntry *
icacheAlloc(int type, u8int *score)
{
	IEntry *ie, *last, *next;
	u32int h;

	h = hashBits(score, icache.bits);
	last = nil;
	for(ie = icache.heads[h]; ie != nil; ie = ie->next){
		if(ie->ia.type == type && scoreEq(ie->score, score)){
			if(last != nil)
				last->next = ie->next;
			else
				icache.heads[h] = ie->next;
			return ie;
		}
		last = ie;
	}

	h = icache.unused;
	if(h < icache.entries){
		ie = &icache.base[h++];
		icache.unused = h;
		return ie;
	}

	h = icache.stolen;
	for(;;){
		h++;
		if(h >= icache.size)
			h = 0;
		ie = icache.heads[h];
		if(ie != nil){
			last = nil;
			for(; next = ie->next; ie = next)
				last = ie;
			if(last != nil)
				last->next = nil;
			else
				icache.heads[h] = nil;
			icache.stolen = h;
			return ie;
		}
	}
}
