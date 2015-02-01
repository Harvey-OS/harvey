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

static	int	verbose;
void
usage(void)
{
	fprint(2, "usage: syncindex [-v] [-B blockcachesize] config\n");
	threadexitsall("usage");
}

Config conf;

void
threadmain(int argc, char *argv[])
{
	u32int bcmem, icmem;

	bcmem = 0;
	icmem = 0;
	ARGBEGIN{
	case 'B':
		bcmem = unittoull(EARGF(usage()));
		break;
	case 'I':
		icmem = unittoull(EARGF(usage()));
		break;
	case 'v':
		verbose++;
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc != 1)
		usage();

	ventifmtinstall();
	if(initventi(argv[0], &conf) < 0)
		sysfatal("can't init venti: %r");
	if(mainindex->bloom && loadbloom(mainindex->bloom) < 0)
		sysfatal("can't load bloom filter: %r");

	if(bcmem < maxblocksize * (mainindex->narenas + mainindex->nsects * 4 + 16))
		bcmem = maxblocksize * (mainindex->narenas + mainindex->nsects * 4 + 16);
	if(0) fprint(2, "initialize %d bytes of disk block cache\n", bcmem);
	initdcache(bcmem);
	initlumpcache(1*1024*1024, 1024/8);
	initicache(icmem);
	initicachewrite();
	if(mainindex->bloom)
		startbloomproc(mainindex->bloom);

	if(verbose)
		printindex(2, mainindex);
	if(syncindex(mainindex) < 0)
		sysfatal("failed to sync index=%s: %r", mainindex->name);
	flushicache();
	flushdcache();

	threadexitsall(0);
}
