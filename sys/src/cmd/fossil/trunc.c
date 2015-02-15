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

void
main(int argc, char **argv)
{
	Dir d;

	if(argc != 3){
		fprint(2, "usage: trunc file size\n");
		exits("usage");
	}

	nulldir(&d);
	d.length = strtoull(argv[2], 0, 0);
	if(dirwstat(argv[1], &d) < 0)
		sysfatal("dirwstat: %r");
	exits(0);
}
