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
#include <bio.h>

Biobuf bout;

static void
pie(IEntry *ie)
{
	Bprint(&bout, "%22lld %V %3d %5d\n",
		ie->ia.addr, ie->score, ie->ia.type, ie->ia.size);
}

void
usage(void)
{
	fprint(2, "usage: printarenas [-B blockcachesize] config [arenaname...]\n");
	threadexitsall(0);
}

Config conf;

int
shoulddump(char *name, int argc, char **argv)
{
	int i;

	if(argc == 0)
		return 1;
	for(i=0; i<argc; i++)
		if(strcmp(name, argv[i]) == 0)
			return 1;
	return 0;
}

enum
{
	ClumpChunks = 32*1024,
};

void
dumparena(Arena *arena, u64int a)
{
	IEntry ie;
	ClumpInfo *ci, *cis;
	u32int clump;
	int i, n, nskip;

	cis = MKN(ClumpInfo, ClumpChunks);
	nskip = 0;
	memset(&ie, 0, sizeof(IEntry));
	for(clump = 0; clump < arena->memstats.clumps; clump += n){
		n = ClumpChunks;
		if(n > arena->memstats.clumps - clump)
			n = arena->memstats.clumps - clump;
		if(readclumpinfos(arena, clump, cis, n) != n){
			fprint(2, "arena directory read failed: %r\n");
			break;
		}

		for(i = 0; i < n; i++){
			ci = &cis[i];
			ie.ia.type = ci->type;
			ie.ia.size = ci->uncsize;
			ie.ia.addr = a;
			a += ci->size + ClumpSize;
			ie.ia.blocks = (ci->size + ClumpSize + (1 << ABlockLog) - 1) >> ABlockLog;
			scorecp(ie.score, ci->score);
			pie(&ie);
		}
	}
	free(cis);
}

void
threadmain(int argc, char *argv[])
{
	int i;
	Index *ix;
	u32int bcmem;

	bcmem = 0;
	ARGBEGIN{
	case 'B':
		bcmem = unittoull(ARGF());
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc < 1)
		usage();

	ventifmtinstall();

	if(initventi(argv[0], &conf) < 0)
		sysfatal("can't init venti: %r");

	if(bcmem < maxblocksize * (mainindex->narenas + mainindex->nsects * 4 + 16))
		bcmem = maxblocksize * (mainindex->narenas + mainindex->nsects * 4 + 16);
	if(0) fprint(2, "initialize %d bytes of disk block cache\n", bcmem);
	initdcache(bcmem);

	Binit(&bout, 1, OWRITE);
	ix = mainindex;
	for(i=0; i<ix->narenas; i++)
		if(shoulddump(ix->arenas[i]->name, argc-1, argv+1))
			dumparena(ix->arenas[i], ix->amap[i].start);
	Bterm(&bout);
	threadexitsall(0);
}
