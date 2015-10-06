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
	static int c;
	int pid;
	pid = rfork(RFMEM|RFPROC);
	if (pid < 0) {
		print("FAIL: rfork\n");
		exits("FAIL");
	}
	if (pid > 0) {
		c++;
	}
	if (pid == 0) {
		print("PASS\n");
		exits(nil);
	}

	if (c > 1) {
		print("FAIL\n");
		exits("FAIL");
	}
	exits(nil);
}
