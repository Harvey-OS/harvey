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
#include <auth.h>

/*
 *  become the authenticated user
 */
int
auth_chuid(AuthInfo *ai, char *ns)
{
	int rv, fd;

	if(ai == nil || ai->cap == nil || *ai->cap == 0){
		werrstr("no capability");
		return -1;
	}

	/* change uid */
	fd = open("#¤/capuse", OWRITE);
	if(fd < 0){
		werrstr("opening #¤/capuse: %r");
		return -1;
	}
	rv = write(fd, ai->cap, strlen(ai->cap));
	close(fd);
	if(rv < 0){
		werrstr("writing %s to #¤/capuse: %r", ai->cap);
		return -1;
	}

	/* get a link to factotum as new user */
	fd = open("/srv/factotum", ORDWR);
	if(fd >= 0)
		mount(fd, -1, "/mnt", MREPL, "", 'M');

	/* set up new namespace */
	return newns(ai->cuid, ns);
}
