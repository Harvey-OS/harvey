#include "stdinc.h"
#include "dat.h"
#include "fns.h"

static int	verbose;

void
usage(void)
{
	fprint(2, "usage: rdarena [-v] arenapart arena\n");
	exits(0);
}

static void
rdArena(Arena *arena)
{
	ZBlock *b;
	u64int a, e;
	u32int bs;

	fprint(2, "copying %s to standard output\n", arena->name);
	printArena(2, arena);

	bs = MaxIoSize;
	if(bs < arena->blockSize)
		bs = arena->blockSize;

	b = allocZBlock(bs, 0);
	e = arena->base + arena->size + arena->blockSize;
	for(a = arena->base - arena->blockSize; a + arena->blockSize <= e; a += bs){
		if(a + bs > e)
			bs = arena->blockSize;
		if(!readPart(arena->part, a, b->data, bs))
			fatal("can't copy %s, read at %lld failed: %r", arena->name, a);
		if(write(1, b->data, bs) != bs)
			fatal("can't copy %s, write at %lld failed: %r", arena->name, a);
	}

	freeZBlock(b);
}

void
main(int argc, char *argv[])
{
	ArenaPart *ap;
	Part *part;
	char *file, *aname;
	int i;

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
	aname = argv[1];

	part = initPart(file, 0);
	if(part == nil)
		fatal("can't open partition %s: %r", file);

	ap = initArenaPart(part);
	if(ap == nil)
		fatal("can't initialize arena partition in %s: %R", file);

	if(verbose)
		printArenaPart(2, ap);

	initDCache(8 * MaxDiskBlock);

	for(i = 0; i < ap->narenas; i++){
		if(strcmp(ap->arenas[i]->name, aname) == 0){
			rdArena(ap->arenas[i]);
			exits(0);
		}
	}

	sysfatal("couldn't find arena %s\n", aname);
}
