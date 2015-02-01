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
/*
 * Print working (current) directory
 */

void
main(int argc, char *argv[])
{
	char pathname[512];

	USED(argc, argv);
	if(getwd(pathname, sizeof(pathname)) == 0) {
		fprint(2, "pwd: %r\n");
		exits("getwd");
	}
	print("%s\n", pathname);
	exits(0);
}
