#include "stdinc.h"
#include "dat.h"
#include "fns.h"

void
printIndex(int fd, Index *ix)
{
	int i;

	fprint(fd, "index=%s version=%d blockSize=%d tabSize=%d\n",
		ix->name, ix->version, ix->blockSize, ix->tabSize);
	fprint(fd, "\tbuckets=%d div=%d\n", ix->buckets, ix->div);
	for(i = 0; i < ix->nsects; i++)
		fprint(fd, "\tsect=%s for buckets [%lld,%lld)\n", ix->smap[i].name, ix->smap[i].start, ix->smap[i].stop);
	for(i = 0; i < ix->narenas; i++)
		fprint(fd, "\tarena=%s at [%lld,%lld)\n", ix->amap[i].name, ix->amap[i].start, ix->amap[i].stop);
}

void
printArenaPart(int fd, ArenaPart *ap)
{
	int i;

	fprint(fd, "arena parition=%s\n\tversion=%d blockSize=%d arenas=%d\n\tsetBase=%d setSize=%d\n",
		ap->part->name, ap->version, ap->blockSize, ap->narenas, ap->tabBase, ap->tabSize);
	for(i = 0; i < ap->narenas; i++)
		fprint(fd, "\tarena=%s at [%lld,%lld)\n", ap->map[i].name, ap->map[i].start, ap->map[i].stop);
}

void
printArena(int fd, Arena *arena)
{
	fprint(fd, "arena='%s' [%lld,%lld)\n\tversion=%d created=%d modified=%d",
		arena->name, arena->base, arena->base + arena->size + 2 * arena->blockSize,
		arena->version, arena->ctime, arena->wtime);
	if(arena->sealed)
		fprint(2, " sealed\n");
	else
		fprint(2, "\n");
	if(!scoreEq(zeroScore, arena->score))
		fprint(2, "\tscore=%V\n", arena->score);

	fprint(fd, "\tclumps=%,d compressed clumps=%,d data=%,lld compressed data=%,lld disk storage=%,lld\n",
		arena->clumps, arena->cclumps, arena->uncsize,
		arena->used - arena->clumps * ClumpSize,
		arena->used + arena->clumps * ClumpInfoSize);
}
