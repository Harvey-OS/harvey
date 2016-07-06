/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "stdinc.h"
#include "dat.h"
#include "fns.h"

void
usage(void)
{
	fprint(2, "usage: printarena arenafile [offset]\n");
	threadexitsall("usage");
}

static void
rdarena(Arena *arena, uint64_t offset)
{
	uint64_t a, aa, e;
	uint32_t magic;
	Clump cl;
	uint8_t score[VtScoreSize];
	ZBlock *lump;

	printarena(2, arena);

	a = arena->base;
	e = arena->base + arena->size;
	if(offset != ~(uint64_t)0) {
		if(offset >= e-a)
			sysfatal("bad offset %llu >= %llu",
				offset, e-a);
		aa = offset;
	} else
		aa = 0;

	for(; aa < e; aa += ClumpSize+cl.info.size) {
		magic = clumpmagic(arena, aa);
		if(magic == ClumpFreeMagic)
			break;
		if(magic != arena->clumpmagic) {
			fprint(2, "illegal clump magic number %#8.8ux offset %llu\n",
				magic, aa);
			break;
		}
		lump = loadclump(arena, aa, 0, &cl, score, 0);
		if(lump == nil) {
			fprint(2, "clump %llu failed to read: %r\n", aa);
			break;
		}
		if(cl.info.type != VtCorruptType) {
			scoremem(score, lump->data, cl.info.uncsize);
			if(scorecmp(cl.info.score, score) != 0) {
				fprint(2, "clump %llu has mismatched score\n", aa);
				break;
			}
			if(vttypevalid(cl.info.type) < 0) {
				fprint(2, "clump %llu has bad type %d\n", aa, cl.info.type);
				break;
			}
		}
		print("%22llud %V %3d %5d\n", aa, score, cl.info.type, cl.info.uncsize);
		freezblock(lump);
	}
	print("end offset %llu\n", aa);
}

void
threadmain(int argc, char *argv[])
{
	char *file;
	Arena *arena;
	uint64_t offset, aoffset;
	Part *part;
	static unsigned char buf[8192];
	ArenaHead head;

	readonly = 1;	/* for part.c */
	aoffset = 0;
	ARGBEGIN{
	case 'o':
		aoffset = strtoull(EARGF(usage()), 0, 0);
		break;
	default:
		usage();
		break;
	}ARGEND

	offset = ~(uint64_t)0;
	switch(argc) {
	default:
		usage();
	case 2:
		offset = strtoull(argv[1], 0, 0);
		/* fall through */
	case 1:
		file = argv[0];
	}


	ventifmtinstall();
	statsinit();

	part = initpart(file, OREAD|ODIRECT);
	if(part == nil)
		sysfatal("can't open file %s: %r", file);
	if(readpart(part, aoffset, buf, sizeof buf) < 0)
		sysfatal("can't read file %s: %r", file);

	if(unpackarenahead(&head, buf) < 0)
		sysfatal("corrupted arena header: %r");

	print("# arena head version=%d name=%.*s blocksize=%d size=%lld clumpmagic=0x%.8x\n",
		head.version, ANameSize, head.name, head.blocksize,
		head.size, head.clumpmagic);

	if(aoffset+head.size > part->size)
		sysfatal("arena is truncated: want %llu bytes have %llu",
			head.size, part->size);

	partblocksize(part, head.blocksize);
	initdcache(8 * MaxDiskBlock);

	arena = initarena(part, aoffset, head.size, head.blocksize);
	if(arena == nil)
		sysfatal("initarena: %r");

	rdarena(arena, offset);
	threadexitsall(0);
}
