/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * tarcat - concatenate tar archives into a single tar archive
 */

#include <u.h>
#include <libc.h>
#include <ctype.h>
#include "tar.h"

enum {
	Stdin,
	Stdout,
};

static int debug;

/* don't copy zero blocks at end */
static void
catenate(int in, char *inname)
{
	vlong len;
	static Hblock hdr;
	Hblock *hp = &hdr;

	if (debug)
		fprint(2, "%s: reading %s\n", inname, argv0);
	while (getdir(hp, in, &len)) {
		writetar(Stdout, (char *)hp, Tblock);  /* write dir block */
		passtar(hp, in, Stdout, len);
	}
}

void
main(int argc, char **argv)
{
	int errflg = 0;

	ARGBEGIN {
	case 'd':
		++debug;
		break;
	default:
		errflg++;
		break;
	} ARGEND
	if (errflg) {
		fprint(2, "usage: %s [-d] [file]...\n", argv0);
		exits("usage");
	}

	if (argc <= 0)
		catenate(Stdin, "/fd/0");
	else
		for (; argc-- > 0; argv++) {
			int in = open(argv[0], OREAD);

			if (in < 0)
				sysfatal("%s: %r", argv[0]);
			catenate(in, argv[0]);
			close(in);
		}
	closeout(Stdout, "/fd/1", 0);
	exits(0);
}
