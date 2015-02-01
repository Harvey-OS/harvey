/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "stdinc.h"
#include "vac.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

void
threadmain(int argc, char **argv)
{
	Biobuf b;
	char *p;

	ARGBEGIN{
	default:
		goto usage;
	}ARGEND
	
	if(argc != 1){
	usage:
		fprint(2, "usage: testinc includefile\n");
		threadexitsall("usage");
	}
	
	loadexcludefile(argv[0]);
	Binit(&b, 0, OREAD);
	while((p = Brdline(&b, '\n')) != nil){
		p[Blinelen(&b)-1] = 0;
		print("%d %s\n", includefile(p), p);
	}
	threadexitsall(0);
}
