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
#include <fcall.h>
#include <thread.h>
#include <9p.h>

/*
 * simplistic permission checking.  assume that
 * each user is the leader of her own group.
 */
int
hasperm(File *f, char *uid, int p)
{
	int m;

	m = f->mode & 7;	/* other */
	if((p & m) == p)
		return 1;

	if(strcmp(f->uid, uid) == 0) {
		m |= (f->mode>>6) & 7;
		if((p & m) == p)
			return 1;
	}

	if(strcmp(f->gid, uid) == 0) {
		m |= (f->mode>>3) & 7;
		if((p & m) == p)
			return 1;
	}

	return 0;
}
