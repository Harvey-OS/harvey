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
#include <auth.h>

void
usage(void)
{
	fprint(2, "usage: testboot cmd args...\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int p[2];

	if(argc == 1)
		usage();

	pipe(p);
	switch(rfork(RFPROC|RFFDG|RFNAMEG)){
	case -1:
		sysfatal("fork: %r");

	case 0:
		dup(p[0], 0);
		dup(p[1], 1);
		exec(argv[1], argv+1);
		sysfatal("exec: %r");

	default:
		if(amount(p[0], "/n/kremvax", MREPL, "") < 0)
			sysfatal("amount: %r");
		break;
	}
	exits(nil);
}
