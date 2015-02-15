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
#include <authsrv.h>

static char *pbmsg = "AS protocol botch";

int
_asgetticket(int fd, char *trbuf, char *tbuf)
{
	if(write(fd, trbuf, TICKREQLEN) < 0){
		close(fd);
		werrstr(pbmsg);
		return -1;
	}
	return _asrdresp(fd, tbuf, 2*TICKETLEN);
}
