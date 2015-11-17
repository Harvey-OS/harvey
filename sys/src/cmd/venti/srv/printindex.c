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
	fprint(2, "usage: printindex [-B blockcachesize] config [isectname...]\n");
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

void
dumpisect(ISect *is)
{
	int j;
	uint8_t *buf;
	uint32_t i;
	uint64_t off;
	IBucket ib;
	IEntry ie;

	buf = emalloc(is->blocksize);
	for(i=0; i<is->blocks; i++){
		off = is->blockbase+(uint64_t)is->blocksize*i;
		if(readpart(is->part, off, buf, is->blocksize) < 0)
			fprint(2, "read %s at 0x%llux: %r\n", is->part->name, off);
		else{
			unpackibucket(&ib, buf, is->bucketmagic);
			for(j=0; j<ib.n; j++){
				unpackientry(&ie, &ib.data[j*IEntrySize]);
				pie(&ie);
			}
		}
	}
}

void
threadmain(int argc, char *argv[])
{
	int i;
	Index *ix;
	uint32_t bcmem;

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

	fmtinstall('H', encodefmt);

	if(initventi(argv[0], &conf) < 0)
		sysfatal("can't init venti: %r");

	if(bcmem < maxblocksize * (mainindex->narenas + mainindex->nsects * 4 + 16))
		bcmem = maxblocksize * (mainindex->narenas + mainindex->nsects * 4 + 16);
	if(0) fprint(2, "initialize %d bytes of disk block cache\n", bcmem);
	initdcache(bcmem);

	ix = mainindex;
	Binit(&bout, 1, OWRITE);
	for(i=0; i<ix->nsects; i++)
		if(shoulddump(ix->sects[i]->name, argc-1, argv+1))
			dumpisect(ix->sects[i]);
	Bterm(&bout);
	threadexitsall(0);
}
