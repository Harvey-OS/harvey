#include "stdinc.h"
#include "dat.h"
#include "fns.h"

static int	verbose;

static void
checkArena(Arena *arena, int scan, int fix)
{
	Arena old;
	int err, e;

	if(verbose && arena->clumps)
		printArena(2, arena);

	old = *arena;

	if(scan){
		arena->used = 0;
		arena->clumps = 0;
		arena->cclumps = 0;
		arena->uncsize = 0;
	}

	err = 0;
	for(;;){
		e = syncArena(arena, 1000, 0, fix);
		err |= e;
		if(!(e & SyncHeader))
			break;
		if(verbose && arena->clumps)
			fprint(2, ".");
	}
	if(verbose && arena->clumps)
		fprint(2, "\n");

	err &= ~SyncHeader;
	if(arena->used != old.used
	|| arena->clumps != old.clumps
	|| arena->cclumps != old.cclumps
	|| arena->uncsize != old.uncsize){
		fprint(2, "incorrect arena header fields\n");
		printArena(2, arena);
		err |= SyncHeader;
	}

	if(!err || !fix)
		return;

	fprint(2, "writing fixed arena header fields\n");
	if(!wbArena(arena))
		fprint(2, "arena header write failed: %r\n");
}

void
usage(void)
{
	fprint(2, "usage: checkarenas [-afv] file\n");
	exits(0);
}

int
main(int argc, char *argv[])
{
	ArenaPart *ap;
	Part *part;
	char *file;
	int i, fix, scan;

	fmtinstall('V', vtScoreFmt);
	fmtinstall('R', vtErrFmt);
	vtAttach();
	statsInit();

	fix = 0;
	scan = 0;
	ARGBEGIN{
	case 'f':
		fix++;
		break;
	case 'a':
		scan = 1;
		break;
	case 'v':
		verbose++;
		break;
	default:
		usage();
		break;
	}ARGEND

	if(!fix)
		readonly = 1;

	if(argc != 1)
		usage();

	file = argv[0];

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

	for(i = 0; i < ap->narenas; i++)
		checkArena(ap->arenas[i], scan, fix);

	if(verbose > 1)
		printStats();
	exits(0);
	return 0;	/* shut up stupid compiler */
}
