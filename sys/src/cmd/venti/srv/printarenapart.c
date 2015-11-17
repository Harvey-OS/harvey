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

uint8_t buf[64*1024];

void
usage(void)
{
	fprint(2, "usage: printarenapart arenafile [offset]\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	char *file, *p, *name;
	char *table;
	uint64_t offset;
	Part *part;
	ArenaPart ap;
	ArenaHead head;
	Arena tail;
	char ct[40], mt[40];

	readonly = 1;	/* for part.c */
	ARGBEGIN{
	default:
		usage();
		break;
	}ARGEND

	switch(argc) {
	default:
		usage();
	case 1:
		file = argv[0];
	}

	ventifmtinstall();
	statsinit();

	part = initpart(file, OREAD|ODIRECT);
	if(part == nil)
		sysfatal("can't open file %s: %r", file);
	if(readpart(part, PartBlank, buf, sizeof buf) < 0)
		sysfatal("can't read file %s: %r", file);

	if(unpackarenapart(&ap, buf) < 0)
		sysfatal("corrupted arena part header: %r");

	print("# arena part version=%d blocksize=%d arenabase=%d\n",
		ap.version, ap.blocksize, ap.arenabase);
	ap.tabbase = (PartBlank+HeadSize+ap.blocksize-1)&~(ap.blocksize-1);
	ap.tabsize = ap.arenabase - ap.tabbase;

	table = malloc(ap.tabsize+1);
	if(readpart(part, ap.tabbase, (unsigned char*)table, ap.tabsize) < 0)
		sysfatal("read %s: %r", file);
	table[ap.tabsize] = 0;

	partblocksize(part, ap.blocksize);
	initdcache(8 * MaxDiskBlock);

	for(p=table; p && *p; p=strchr(p, '\n')){
		if(*p == '\n')
			p++;
		name = p;
		p = strpbrk(p, " \t");
		if(p == nil){
			fprint(2, "bad line: %s\n", name);
			break;
		}
		offset = strtoull(p, nil, 0);
		if(readpart(part, offset, buf, sizeof buf) < 0){
			fprint(2, "%s: read %s: %r\n", argv0, file);
			continue;
		}
		if(unpackarenahead(&head, buf) < 0){
			fprint(2, "%s: unpackarenahead: %r\n", argv0);
			continue;
		}
		if(readpart(part, offset+head.size-head.blocksize, buf, head.blocksize) < 0){
			fprint(2, "%s: read %s: %r\n", argv0, file);
			continue;
		}
		if(unpackarena(&tail, buf) < 0){
			fprint(2, "%s: unpackarena: %r\n", argv0);
			continue;
		}
		print("arena %s %lld clumps=%,d cclumps=%,d used=%,lld uncsize=%,lld%s\n",
			tail.name, offset,
			tail.diskstats.clumps, tail.diskstats.cclumps,
			tail.diskstats.used, tail.diskstats.uncsize,
			tail.diskstats.sealed ? " sealed" : "");
		strcpy(ct, ctime(tail.ctime));
		ct[28] = 0;
		strcpy(mt, ctime(tail.wtime));
		mt[28] = 0;
		print("\tctime=%s\n\tmtime=%s\n", ct, mt);
	}
	threadexitsall(0);
}
