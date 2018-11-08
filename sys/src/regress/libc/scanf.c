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
#include <stdio.h>

void
main()
{
	// Basic test
	{
		char *str = "123 456 789\n";
		int a = -1, b = -1, c = -1;
		sscanf(str, "%d %d %d\n", &a, &b, &c);

		if (a != 123 || b != 456 || c != 789) {
			print("FAIL - expected %s, found %d %d %d\n", str, a, b, c);
			exits("FAIL");
		}
	}

	// Hex parsing
	{
		char *str = "0x0 0x1d";
		int a = -1, b = -1;
		sscanf(str, "%x %x", &a, &b);

		if (a != 0x0 || b != 0x1d) {
			print("FAIL - expected %s, found %x %x\n", str, a, b);
			exits("FAIL");
		}
	}

	print("PASS\n");
	exits(nil);
}
