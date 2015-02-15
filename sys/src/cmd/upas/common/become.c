/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "common.h"
#include <auth.h>
#include <ndb.h>

/*
 *  become powerless user
 */
int
become(char **cmd, char *who)
{
	int fd;

	USED(cmd);
	if(strcmp(who, "none") == 0) {
		fd = open("#c/user", OWRITE);
		if(fd < 0 || write(fd, "none", strlen("none")) < 0) {
			werrstr("can't become none");
			return -1;
		}
		close(fd);
		if(newns("none", 0)) {
			werrstr("can't set new namespace");
			return -1;
		}
	}
	return 0;
}

