/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/limits.h>

char *
getlogin_r(char *buf, int len)
{
	int f, n;

	f = open("/dev/user", O_RDONLY);
	if(f < 0)
		return 0;
	n = read(f, buf, len);
	buf[len-1] = 0;
	close(f);
	return (n>=0)? buf : 0;
}

char *
getlogin(void)
{
	static char buf[NAME_MAX+1];

	return getlogin_r(buf, sizeof buf);
}
