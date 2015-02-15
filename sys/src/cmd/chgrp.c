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

int	readgid(char*);
int	uflag;

void
main(int argc, char *argv[])
{
	int i;
	Dir dir;
	char *group;
	char *errs;

	ARGBEGIN {
	default:
	usage:
		fprint(2, "usage: chgrp [ -uo ] group file ....\n");
		exits("usage");
		return;
	case 'u':
	case 'o':
		uflag++;
		break;
	} ARGEND
	if(argc < 1)
		goto usage;

	group = argv[0];
	errs = 0;
	for(i=1; i<argc; i++){
		nulldir(&dir);
		if(uflag)
			dir.uid = group;
		else
			dir.gid = group;
		if(dirwstat(argv[i], &dir) == -1) {
			fprint(2, "chgrp: can't wstat %s: %r\n", argv[i]);
			errs = "can't wstat";
			continue;
		}
	}
	exits(errs);
}
