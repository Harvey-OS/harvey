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
        void *seg = segattach(0, "shared", nil, 1024);
        if (seg == (void*)-1) {
		print("FAIL segattach\n");
		exits("FAIL");
                return;
        }

        int rcode = segdetach(seg);
        if (rcode != 0) {
		print("FAIL segdetach\n");
		exits("FAIL");
        }

	print("PASS\n");
	exits(nil);
}
