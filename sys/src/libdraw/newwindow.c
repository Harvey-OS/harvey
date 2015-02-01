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
#include <draw.h>

/* Connect us to new window, if possible */
int
newwindow(char *str)
{
	int fd;
	char *wsys;
	char buf[256];

	wsys = getenv("wsys");
	if(wsys == nil)
		return -1;
	fd = open(wsys, ORDWR);
	free(wsys);
	if(fd < 0)
		return -1;
	rfork(RFNAMEG);
	if(str)
		snprint(buf, sizeof buf, "new %s", str);
	else
		strcpy(buf, "new");
	return mount(fd, -1, "/dev", MBEFORE, buf);
}

