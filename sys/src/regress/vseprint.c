
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
testvseprint(char *buf, int n, char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf+n, fmt, arg);
	va_end(arg);
}

main()
{
	char buf[1024];
	testvseprint(buf, sizeof(buf), "This is a really long message %d %d %d %d\n", 1, 2, 3, 4);
	if (strcmp(buf, "This is a really long message 1 2 3 4\n")) {
		print("FAIL\n");
		exits("FAIL");
	}

	print("PASS\n");
	exits("PASS");
}
