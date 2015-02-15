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

/*
 * Format:
  3 r  M    4 (0000000000457def 11 00)   8192      512 /rc/lib/rcmain
 */

int
iounit(int fd)
{
	int i, cfd;
	char buf[128], *args[10];

	snprint(buf, sizeof buf, "#d/%dctl", fd);
	cfd = open(buf, OREAD);
	if(cfd < 0)
		return 0;
	i = read(cfd, buf, sizeof buf-1);
	close(cfd);
	if(i <= 0)
		return 0;
	buf[i] = '\0';
	if(tokenize(buf, args, nelem(args)) != nelem(args))
		return 0;
	return atoi(args[7]);
}
