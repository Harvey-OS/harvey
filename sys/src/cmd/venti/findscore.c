#include "stdinc.h"
#include "dat.h"
#include "fns.h"

enum
{
	ClumpChunks	= 32*1024
};

static int	verbose;

static int	writeClumpHead(Arena *arena, u64int aa, Clump *cl);
static int	writeClumpMagic(Arena *arena, u64int aa, u32int magic);

int
clumpInfoEq(ClumpInfo *c, ClumpInfo *d)
{
	return c->type == d->type
		&& c->size == d->size
		&& c->uncsize == d->uncsize
		&& scoreEq(c->score, d->score);
}

/*
 * synchronize the clump info directory with
 * with the clumps actually stored in the arena.
 * the directory should be at least as up to date
 * as the arena's trailer.
 *
 * checks/updates at most n clumps.
 *
 * returns 1 if ok, -1 if an error occured, 0 if blocks were updated
 */
int
findscore(Arena *arena, uchar *score)
{
	IEntry ie;
	ClumpInfo *ci, *cis;
	u64int a;
	u32int clump;
	int i, n, found;

//ZZZ remove fprint?
	if(arena->clumps)
		fprint(2, "reading directory for arena=%s with %d entries\n", arena->name, arena->clumps);

	cis = MKN(ClumpInfo, ClumpChunks);
	found = 0;
	memset(&ie, 0, sizeof(IEntry));
	for(clump = 0; clump < arena->clumps; clump += n){
		n = ClumpChunks;
		if(n > arena->clumps - clump)
			n = arena->clumps - clump;
		if(readClumpInfos(arena, clump, cis, n) != n){
			setErr(EOk, "arena directory read failed: %R");
			break;
		}

		for(i = 0; i < n; i++){
			ci = &cis[i];
			if(scoreEq(score, ci->score)){
				fprint(2, "found at clump=%d with type=%d size=%d csize=%d position=%lld\n",
					clump + i, ci->type, ci->uncsize, ci->size, a);
				found++;
			}
			a += ci->size + ClumpSize;
		}
	}
	free(cis);
	return found;
}

void
usage(void)
{
	fprint(2, "usage: findscore [-v] arenafile score\n");
	exits(0);
}

int
main(int argc, char *argv[])
{
	ArenaPart *ap;
	Part *part;
	char *file;
	u8int score[VtScoreSize];
	int i, found;

	fmtinstall('V', vtScoreFmt);
	fmtinstall('R', vtErrFmt);
	vtAttach();
	statsInit();

	ARGBEGIN{
	case 'v':
		verbose++;
		break;
	default:
		usage();
		break;
	}ARGEND

	readonly = 1;

	if(argc != 2)
		usage();

	file = argv[0];
	if(!strScore(argv[1], score))
		fatal("bad score %s\n", argv[1]);

	part = initPart(file, 0);
	if(part == nil)
		fatal("can't open partition %s: %r", file);

	ap = initArenaPart(part);
	if(ap == nil)
		fatal("can't initialize arena partition in %s: %R", file);

	if(verbose > 1){
		printArenaPart(2, ap);
		fprint(2, "\n");
	}

	initDCache(8 * MaxDiskBlock);

	found = 0;
	for(i = 0; i < ap->narenas; i++)
		found += findscore(ap->arenas[i], score);

	print("found %d occurances of %V\n", found, score);

	if(verbose > 1)
		printStats();
	exits(0);
	return 0;	/* shut up stupid compiler */
}
