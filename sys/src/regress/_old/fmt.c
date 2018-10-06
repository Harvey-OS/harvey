
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
main(void)
{
	static char buf[1024];
	unsigned long x = 0xfeefd000deadbeef;
	seprint(buf, buf+sizeof(buf), "%.2X", x);
	if (strcmp("DEADBEEF", buf)) {
		print("FAIL");
		exits("FAIL");
	}
	print("PASS\n");
	exits("PASS");
}
