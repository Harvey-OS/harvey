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

int
main(int argc, char *argv[])
{
	int iter = 1000, i;
	Waitmsg *w;

	if(argc > 1)
		iter = strtoul(argv[1], 0, 0);
	for(i = 0; i < iter; i++) {
		int pid;
		pid = fork();
		if(pid < 0) {
			print("FAIL\n");
			exits("FAIL");
		}
		if(pid == 0) {
			// execl won't work yet. varargs hell.
			char *args[] = {"ps", nil};
			exec("/amd64/bin/ps", args);
			fprint(2, "Exec fails: %r\n");
			print("FAIL\n");
			exits("FAIL");
		}
		w = wait();
		if(w == nil) {
			print("FAIL\n");
			exits("FAIL");
		}
	}
	// if we get here, we have survived. That's a miracle.
	print("PASS\n");
	exits("PASS");
	return 0;
}
