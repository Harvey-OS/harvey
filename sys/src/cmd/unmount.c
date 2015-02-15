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
main(int argc, char *argv[])
{
	int r;
	char *mnted, *mtpt;

	argv0 = argv[0];
	switch (argc) {
	case 2:
		mnted = nil;
		mtpt = argv[1];
		break;
	case 3:
		mnted = argv[1];
		mtpt = argv[2];
		break;
	default:
		SET(mnted, mtpt);
		fprint(2, "usage: unmount mountpoint\n");
		fprint(2, "       unmount mounted mountpoint\n");
		exits("usage");
	}

	/* unmount takes arguments in the same order as mount */
	r = unmount(mnted, mtpt);
	if(r < 0)
		sysfatal("%s: %r", mtpt);
	exits(0);
}
