#include "stdinc.h"
#include "dat.h"
#include "fns.h"

Stats stats;

void
statsInit(void)
{
	stats.lock = vtLockAlloc();
}

static int
percent(long v, long total)
{
	if(total == 0)
		total = 1;
	if(v < 1000*1000)
		return (v * 100) / total;
	total /= 100;
	if(total == 0)
		total = 1;
	return v / total;
}

void
printStats(void)
{
	fprint(2, "lump writes=%,ld\n", stats.lumpWrites);
	fprint(2, "lump reads=%,ld\n", stats.lumpReads);
	fprint(2, "lump cache read hits=%,ld\n", stats.lumpHit);
	fprint(2, "lump cache read misses=%,ld\n", stats.lumpMiss);

	fprint(2, "clump disk writes=%,ld\n", stats.clumpWrites);
	fprint(2, "clump disk bytes written=%,lld\n", stats.clumpBWrites);
	fprint(2, "clump disk bytes compressed=%,lld\n", stats.clumpBComp);
	fprint(2, "clump disk reads=%,ld\n", stats.clumpReads);
	fprint(2, "clump disk bytes read=%,lld\n", stats.clumpBReads);
	fprint(2, "clump disk bytes uncompressed=%,lld\n", stats.clumpBUncomp);

	fprint(2, "clump directory disk writes=%,ld\n", stats.ciWrites);
	fprint(2, "clump directory disk reads=%,ld\n", stats.ciReads);

	fprint(2, "index disk writes=%,ld\n", stats.indexWrites);
	fprint(2, "index disk reads=%,ld\n", stats.indexReads);
	fprint(2, "index disk reads for modify=%,ld\n", stats.indexWReads);
	fprint(2, "index disk reads for allocation=%,ld\n", stats.indexAReads);

	fprint(2, "index cache lookups=%,ld\n", stats.icLookups);
	fprint(2, "index cache hits=%,ld %d%%\n", stats.icHits,
		percent(stats.icHits, stats.icLookups));
	fprint(2, "index cache fills=%,ld %d%%\n", stats.icFills,
		percent(stats.icFills, stats.icLookups));
	fprint(2, "index cache inserts=%,ld\n", stats.icInserts);

	fprint(2, "disk cache hits=%,ld\n", stats.pcHit);
	fprint(2, "disk cache misses=%,ld\n", stats.pcMiss);
	fprint(2, "disk cache reads=%,ld\n", stats.pcReads);
	fprint(2, "disk cache bytes read=%,lld\n", stats.pcBReads);

	fprint(2, "disk writes=%,ld\n", stats.diskWrites);
	fprint(2, "disk bytes written=%,lld\n", stats.diskBWrites);
	fprint(2, "disk reads=%,ld\n", stats.diskReads);
	fprint(2, "disk bytes read=%,lld\n", stats.diskBReads);
}
