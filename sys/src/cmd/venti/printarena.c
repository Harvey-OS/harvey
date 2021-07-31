#include "stdinc.h"
#include "dat.h"
#include "fns.h"

int wanttype;
int readonly = 1;	/* for part.c */

void
usage(void)
{
	fprint(2, "usage: printarena arenafile [offset]\n");
	exits("usage");
}

static void
rdArena(Arena *arena, u64int offset)
{
	u64int a, aa, e;
	u32int magic;
	Clump cl;
	uchar score[VtScoreSize];
	ZBlock *lump;

	printArena(2, arena);

	a = arena->base;
	e = arena->base + arena->size;
	if(offset != ~(u64int)0) {
		if(offset >= e-a)
			vtFatal("bad offset %llud >= %llud\n",
				offset, e-a);
		aa = offset;
	} else
		aa = 0;

	for(; aa < e; aa += ClumpSize+cl.info.size) {
		magic = clumpMagic(arena, aa);
		if(magic == ClumpFreeMagic)
			break;
		if(magic != ClumpMagic) {
			fprint(2, "illegal clump magic number %#8.8ux offset %llud\n",
				magic, aa);
			break;
		}
		lump = loadClump(arena, aa, 0, &cl, score, 0);
		if(lump == nil) {
			fprint(2, "clump %llud failed to read: %R\n", aa);
			break;
		}
		if(cl.info.type != VtTypeCorrupt) {
			scoreMem(score, lump->data, cl.info.uncsize);
			if(!scoreEq(cl.info.score, score)) {
				fprint(2, "clump %llud has mismatched score\n", aa);
				break;
			}
			if(!vtTypeValid(cl.info.type)) {
				fprint(2, "clump %llud has bad type %d\n", aa, cl.info.type);
				break;
			}
		}
		if(wanttype == 0 || cl.info.type == wanttype)
			print("%V %d\n", score, cl.info.type);
		freeZBlock(lump);
	}
	print("end offset %llud\n", aa);
}

int
main(int argc, char *argv[])
{
	char *file;
	Arena *arena;
	u64int offset, aoffset;
	Part *part;
	Dir *d;
	uchar buf[8192];
	ArenaHead head;

	aoffset = 0;
	ARGBEGIN{
	case 't':
		wanttype = atoi(EARGF(usage()));
		break;
	case 'o':
		aoffset = strtoull(EARGF(usage()), 0, 0);
		break;
	default:
		usage();
		break;
	}ARGEND

	offset = ~(u64int)0;
	switch(argc) {
	default:
		usage();
	case 2:
		offset = strtoull(argv[1], 0, 0);
		/* fall through */
	case 1:
		file = argv[0];
	}

	vtAttach();

	fmtinstall('V', vtScoreFmt);
	fmtinstall('R', vtErrFmt);

	statsInit();

	if((d = dirstat(file)) == nil)
		vtFatal("can't stat file %s: %r", file);

	part = initPart(file, 0);
	if(part == nil)
		vtFatal("can't open file %s: %R", file);
	if(!readPart(part, aoffset, buf, sizeof buf))
		vtFatal("can't read file %s: %R", file);

	if(!unpackArenaHead(&head, buf))
		vtFatal("corrupted arena header: %R");

	if(aoffset+head.size > d->length)
		vtFatal("arena is truncated: want %llud bytes have %llud\n",
			head.size, d->length);

	partBlockSize(part, head.blockSize);
	initDCache(8 * MaxDiskBlock);

	arena = initArena(part, aoffset, head.size, head.blockSize);
	if(arena == nil)
		vtFatal("initArena: %R");

	rdArena(arena, offset);
	vtDetach();
	exits(0);
	return 0;
}
