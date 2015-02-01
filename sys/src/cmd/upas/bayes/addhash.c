/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <regexp.h>
#include "hash.h"

Hash hash;

void
usage(void)
{
	fprint(2, "addhash [-o out] file scale [file scale]...\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i, fd, n;
	char err[ERRMAX], *out;
	Biobuf *b, bout;

	out = nil;
	ARGBEGIN{
	case 'o':
		out = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;

	if(argc==0 || argc%2)
		usage();

	while(argc > 0){
		if((b = Bopenlock(argv[0], OREAD)) == nil)
			sysfatal("open %s: %r", argv[0]);
		n = atoi(argv[1]);
		if(n == 0)
			sysfatal("0 scale given");
		Breadhash(b, &hash, n);
		Bterm(b);
		argv += 2;
		argc -= 2;
	}

	fd = 1;
	if(out){
		for(i=0; i<120; i++){
			if((fd = create(out, OWRITE, 0666|DMEXCL)) >= 0)
				break;
			rerrstr(err, sizeof err);
			if(strstr(err, "file is locked")==nil && strstr(err, "exclusive lock")==nil)
				break;
			sleep(1000);
		}
		if(fd < 0)
			sysfatal("could not open %s: %r", out);
	}
		
	Binit(&bout, fd, OWRITE);
	Bwritehash(&bout, &hash);
	Bterm(&bout);
	exits(0);
}

