#include "stdinc.h"
#include "dat.h"
#include "fns.h"

static int
writeBucket(Index *ix, u32int buck, IBucket *ib, ZBlock *b)
{
	ISect *is;

	is = findISect(ix, buck);
	if(is == nil){
		setErr(EAdmin, "bad math in writeBucket");
		return 0;
	}
	if(buck < is->start || buck >= is->stop)
		setErr(EAdmin, "index write out of bounds: %d not in [%d,%d)\n",
				buck, is->start, is->stop);
	buck -= is->start;
	vtLock(stats.lock);
	stats.indexWrites++;
	vtUnlock(stats.lock);
	packIBucket(ib, b->data);
	return writePart(is->part, is->blockBase + ((u64int)buck << is->blockLog), b->data, is->blockSize);
}

static int
buildIndex(Index *ix, Part *part, u64int off, u64int clumps, int zero)
{
	IEStream *ies;
	IBucket ib, zib;
	ZBlock *z, *b;
	u32int next, buck;
	int ok;
	u64int found = 0;

//ZZZ make buffer size configurable
	b = allocZBlock(ix->blockSize, 0);
	z = allocZBlock(ix->blockSize, 1);
	ies = initIEStream(part, off, clumps, 64*1024);
	if(b == nil || z == nil || ies == nil){
		ok = 0;
		goto breakout;
		return 0;
	}
	ok = 1;
	next = 0;
	ib.data = b->data + IBucketSize;
	zib.data = z->data + IBucketSize;
	zib.n = 0;
	zib.next = 0;
	for(;;){
		buck = buildBucket(ix, ies, &ib);
		found += ib.n;
		if(zero){
			for(; next != buck; next++){
				if(next == ix->buckets){
					if(buck != TWID32){
						fprint(2, "bucket out of range\n");
						ok = 0;
					}
					goto breakout;
				}
				if(!writeBucket(ix, next, &zib, z)){
					fprint(2, "can't write zero bucket to buck=%d: %R", next);
					ok = 0;
				}
			}
		}
		if(buck >= ix->buckets){
			if(buck == TWID32)
				break;
			fprint(2, "bucket out of range\n");
			ok = 0;
			goto breakout;
		}
		if(!writeBucket(ix, buck, &ib, b)){
			fprint(2, "bad bucket found=%lld: %R\n", found);
			ok = 0;
		}
		next = buck + 1;
	}
breakout:;
	fprint(2, "constructed index with %lld entries\n", found);
	freeIEStream(ies);
	freeZBlock(z);
	freeZBlock(b);
	return ok;
}

void
usage(void)
{
	fprint(2, "usage: buildindex [-Z] [-B blockcachesize] config tmppart\n");
	exits(0);
}

int
main(int argc, char *argv[])
{
	Part *part;
	u64int clumps, base;
	u32int bcmem;
	int zero;

	vtAttach();

	zero = 1;
	bcmem = 0;
	ARGBEGIN{
	case 'B':
		bcmem = unittoull(ARGF());
		break;
	case 'Z':
		zero = 0;
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc != 2)
		usage();

	if(!initVenti(argv[0]))
		fatal("can't init venti: %R");

	if(bcmem < maxBlockSize * (mainIndex->narenas + mainIndex->nsects * 4 + 16))
		bcmem = maxBlockSize * (mainIndex->narenas + mainIndex->nsects * 4 + 16);
	fprint(2, "initialize %d bytes of disk block cache\n", bcmem);
	initDCache(bcmem);

	fprint(2, "building a new index %s using %s for temporary storage\n", mainIndex->name, argv[1]);

	part = initPart(argv[1], 1);
	if(part == nil)
		fatal("can't initialize temporary parition: %R");

	clumps = sortRawIEntries(mainIndex, part, &base);
	if(clumps == TWID64)
		fatal("can't build sorted index: %R");
	fprint(2, "found and sorted index entries for clumps=%lld at %lld\n", clumps, base);

	if(!buildIndex(mainIndex, part, base, clumps, zero))
		fatal("can't build new index: %R");
	
	exits(0);
	return 0;	/* shut up stupid compiler */
}
