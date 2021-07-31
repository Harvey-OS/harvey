#include "stdinc.h"
#include "dat.h"
#include "fns.h"

int	count[VtMaxLumpSize][VtMaxType];

enum
{
	ClumpChunks	= 32*1024
};

static
readArenaInfo(Arena *arena)
{
	ClumpInfo *ci, *cis;
	u32int clump;
	int i, n, ok;

	if(arena->clumps)
		fprint(2, "reading directory for arena=%s with %d entries\n", arena->name, arena->clumps);

	cis = MKN(ClumpInfo, ClumpChunks);
	ok = 1;
	for(clump = 0; clump < arena->clumps; clump += n){
		n = ClumpChunks;

		if(n > arena->clumps - clump)
			n = arena->clumps - clump;

		if(readClumpInfos(arena, clump, cis, n) != n){
			setErr(EOk, "arena directory read failed: %R");
			ok = 0;
			break;
		}

		for(i = 0; i < n; i++){
			ci = &cis[i];
			if(ci->type >= VtMaxType || ci->uncsize >= VtMaxLumpSize) {
				fprint(2, "bad clump: %d: type = %d: size = %d\n", clump+i, ci->type, ci->uncsize);
				continue;
			}
if(ci->uncsize == 422)
print("%s: %d: %V\n", arena->name, clump+i, ci->score);
			count[ci->uncsize][ci->type]++;
		}
	}
	free(cis);
	if(!ok)
		return TWID32;
	return clump;
}

static void
clumpStats(Index *ix)
{
	int ok;
	ulong clumps, n;
	int i, j, t;

	ok = 1;
	clumps = 0;
	for(i = 0; i < ix->narenas; i++){
		n = readArenaInfo(ix->arenas[i]);
		if(n == TWID32){
			ok = 0;
			break;
		}
		clumps += n;
	}

	if(!ok)
		return;

	print("clumps = %ld\n", clumps);
	for(i=0; i<VtMaxLumpSize; i++) {
		t = 0;
		for(j=0; j<VtMaxType; j++)
			t += count[i][j];
		if(t == 0)
			continue;
		print("%d\t%d", i, t);
		for(j=0; j<VtMaxType; j++)
			print("\t%d", count[i][j]);
		print("\n");
	}
}


void
usage(void)
{
	fprint(2, "usage: clumpstats [-B blockcachesize] config\n");
	exits(0);
}

int
main(int argc, char *argv[])
{
	u32int bcmem;

	vtAttach();

	bcmem = 0;

	ARGBEGIN{
	case 'B':
		bcmem = unittoull(ARGF());
		break;
	default:
		usage();
		break;
	}ARGEND

	readonly = 1;

	if(argc != 1)
		usage();

	if(!initVenti(argv[0]))
		fatal("can't init venti: %R");

	if(bcmem < maxBlockSize * (mainIndex->narenas + mainIndex->nsects * 4 + 16))
		bcmem = maxBlockSize * (mainIndex->narenas + mainIndex->nsects * 4 + 16);
	fprint(2, "initialize %d bytes of disk block cache\n", bcmem);
	initDCache(bcmem);

	clumpStats(mainIndex);
	
	exits(0);
	return 0;	/* shut up stupid compiler */
}
