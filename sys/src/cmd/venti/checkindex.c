#include "stdinc.h"
#include "dat.h"
#include "fns.h"

static int
checkBucket(Index *ix, u32int buck, IBucket *ib)
{
	ISect *is;
	DBlock *eb;
	IBucket eib;
	IEntry ie, eie;
	int i, ei, ok, c;

	is = findISect(ix, buck);
	if(is == nil){
		setErr(EAdmin, "bad math in checkBuckets");
		return 0;
	}
	buck -= is->start;
	eb = getDBlock(is->part, is->blockBase + ((u64int)buck << is->blockLog), 1);
	if(eb == nil)
		return 0;
	unpackIBucket(&eib, eb->data);

	ok = 1;
	ei = 0;
	for(i = 0; i < ib->n; i++){
		while(ei < eib.n){
			c = ientryCmp(&ib->data[i * IEntrySize], &eib.data[ei * IEntrySize]);
			if(c == 0){
				unpackIEntry(&ie, &ib->data[i * IEntrySize]);
				unpackIEntry(&eie, &eib.data[ei * IEntrySize]);
				if(!iAddrEq(&ie.ia, &eie.ia)){
					fprint(2, "bad entry in index for score=%V\n", &ib->data[i * IEntrySize]);
					fprint(2, "\taddr=%lld type=%d size=%d blocks=%d\n",
						ie.ia.addr, ie.ia.type, ie.ia.size, ie.ia.blocks);
					fprint(2, "\taddr=%lld type=%d size=%d blocks=%d\n",
						eie.ia.addr, eie.ia.type, eie.ia.size, eie.ia.blocks);
				}
				ei++;
				goto cont;
			}
			if(c < 0)
				break;
if(1)
			fprint(2, "spurious entry in index for score=%V type=%d\n",
				&eib.data[ei * IEntrySize], eib.data[ei * IEntrySize + IEntryTypeOff]);
			ei++;
			ok = 0;
		}
		fprint(2, "missing entry in index for score=%V type=%d\n",
			&ib->data[i * IEntrySize], ib->data[i * IEntrySize + IEntryTypeOff]);
		ok = 0;
	cont:;
	}
	for(; ei < eib.n; ei++){
if(1)		fprint(2, "spurious entry in index for score=%V; found %d entries expected %d\n",
			&eib.data[ei * IEntrySize], eib.n, ib->n);
		ok = 0;
		break;
	}
	putDBlock(eb);
	return ok;
}

int
checkIndex(Index *ix, Part *part, u64int off, u64int clumps, int zero)
{
	IEStream *ies;
	IBucket ib, zib;
	ZBlock *z, *b;
	u32int next, buck;
	int ok, bok;
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
	ib.data = b->data;
	zib.data = z->data;
	zib.n = 0;
	zib.next = 0;
	for(;;){
		buck = buildBucket(ix, ies, &ib);
		found += ib.n;
		if(zero){
			for(; next != buck; next++){
				if(next == ix->buckets){
					if(buck != TWID32)
						fprint(2, "bucket out of range\n");
					goto breakout;
				}
				bok = checkBucket(ix, next, &zib);
				if(!bok){
					fprint(2, "bad bucket=%d found: %R\n", next);
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
		bok = checkBucket(ix, buck, &ib);
		if(!bok){
			fprint(2, "bad bucket found=%lld: %R\n", found);
			ok = 0;
		}
		next = buck + 1;
	}
breakout:;
fprint(2, "found %lld entries in sorted list\n", found);
	freeIEStream(ies);
	freeZBlock(z);
	freeZBlock(b);
	return ok;
}

void
usage(void)
{
	fprint(2, "usage: checkindex [-f] [-B blockcachesize] config tmp\n");
	exits(0);
}

int
main(int argc, char *argv[])
{
	Part *part;
	u64int clumps, base;
	u32int bcmem;
	int fix, skipz;

	vtAttach();

	fix = 0;
	bcmem = 0;
	skipz = 0;
	ARGBEGIN{
	case 'B':
		bcmem = unittoull(ARGF());
		break;
	case 'f':
		fix++;
		break;
	case 'Z':
		skipz = 1;
		break;
	default:
		usage();
		break;
	}ARGEND

	if(!fix)
		readonly = 1;

	if(argc != 2)
		usage();

	if(!initVenti(argv[0]))
		fatal("can't init venti: %R");

	if(bcmem < maxBlockSize * (mainIndex->narenas + mainIndex->nsects * 4 + 16))
		bcmem = maxBlockSize * (mainIndex->narenas + mainIndex->nsects * 4 + 16);
	fprint(2, "initialize %d bytes of disk block cache\n", bcmem);
	initDCache(bcmem);

	part = initPart(argv[1], 1);
	if(part == nil)
		fatal("can't initialize temporary parition: %R");

	clumps = sortRawIEntries(mainIndex, part, &base);
	if(clumps == TWID64)
		fatal("can't build sorted index: %R");
	fprint(2, "found and sorted index entries for clumps=%lld at %lld\n", clumps, base);
	checkIndex(mainIndex, part, base, clumps, !skipz);
	
	exits(0);
	return 0;	/* shut up stupid compiler */
}
