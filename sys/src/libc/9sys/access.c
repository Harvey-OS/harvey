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
access(const char *name, int mode)
{
	int fd;
	Dir *db;
	static char omode[] = {
		0,
		OEXEC,
		OWRITE,
		ORDWR,
		OREAD,
		OEXEC,	/* only approximate */
		ORDWR,
		ORDWR	/* only approximate */
	};

	if(mode == AEXIST){
		db = dirstat(name);
		free(db);
		if(db != nil)
			return 0;
		return -1;
	}
	fd = open(name, omode[mode&7]);
	if(fd >= 0){
		close(fd);
		return 0;
	}
	return -1;
}
