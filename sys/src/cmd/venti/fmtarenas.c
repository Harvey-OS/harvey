#include "stdinc.h"
#include "dat.h"
#include "fns.h"

void
usage(void)
{
	fprint(2, "usage: fmtarenas [-Z] [-b blocksize] [-a arenasize] name file\n");
	exits(0);
}

int
main(int argc, char *argv[])
{
	ArenaPart *ap;
	Part *part;
	Arena *arena;
	u64int addr, limit, asize, apsize;
	char *file, *name, aname[ANameSize];
	int i, n, blockSize, tabSize, zero;

	fmtinstall('V', vtScoreFmt);
	fmtinstall('R', vtErrFmt);
	vtAttach();
	statsInit();

	blockSize = 8 * 1024;
	asize = 512 * 1024 *1024;
	tabSize = 64 * 1024;		/* BUG: should be determine from number of arenas */
	zero = 1;
	ARGBEGIN{
	case 'a':
		asize = unittoull(ARGF());
		if(asize == TWID64)
			usage();
		break;
	case 'b':
		blockSize = unittoull(ARGF());
		if(blockSize == ~0)
			usage();
		if(blockSize > MaxDiskBlock){
			fprint(2, "block size too large, max %d\n", MaxDiskBlock);
			exits("usage");
		}
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

	name = argv[0];
	file = argv[1];

	if(!nameOk(name))
		fatal("illegal name template %s", name);

	part = initPart(file, 1);
	if(part == nil)
		fatal("can't open partition %s: %r", file);

	if(zero)
		zeroPart(part, blockSize);

	ap = newArenaPart(part, blockSize, tabSize);
	if(ap == nil)
		fatal("can't initialize arena: %R");

	n = (ap->size - ap->arenaBase + asize - 1) / asize;
	if(n * asize - (ap->size - ap->arenaBase) < MinArenaSize)
		n--;
	if(n < 0)
		n = 0;

	apsize = asize * n;
	if(apsize > ap->size)
		apsize = ap->size;
	
	fprint(2, "configuring %s with arenas=%d for a total storage of bytes=%lld and directory bytes=%d\n",
		file, n, apsize, ap->tabSize);

	ap->narenas = n;
	ap->map = MKNZ(AMap, n);
	ap->arenas = MKNZ(Arena*, n);

	addr = ap->arenaBase;
	for(i = 0; i < n; i++){
		limit = addr + asize;
		if(limit >= ap->size || ap->size - limit < MinArenaSize){
			limit = ap->size;
			if(limit - addr < MinArenaSize)
				fatal("bad arena set math: runt arena at %lld,%lld %lld\n", addr, limit, ap->size);
		}

		snprint(aname, ANameSize, "%s%d", name, i);

		fprint(2, "adding arena %s at [%lld,%lld)\n", aname, addr, limit);

		arena = newArena(part, aname, addr, limit - addr, blockSize);
		if(!arena)
			fprint(2, "can't make new arena %s: %r", aname);
		freeArena(arena);

		ap->map[i].start = addr;
		ap->map[i].stop = limit;
		nameCp(ap->map[i].name, aname);

		addr = limit;
	}

	if(!wbArenaPart(ap))
		fprint(2, "can't write back arena partition header for %s: %R\n", file);

	exits(0);
	return 0;	/* shut up stupid compiler */
}
