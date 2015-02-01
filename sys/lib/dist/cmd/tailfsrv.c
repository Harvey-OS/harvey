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

void
main(void)
{
	int fd, p[2];
	char buf[8192], n;

	pipe(p);
	fd = create("/srv/log", OWRITE, 0666);
	fprint(fd, "%d", p[0]);
	close(fd);
	close(p[0]);
	while((n = read(p[1], buf, sizeof buf)) >= 0)
		write(1, buf, n);
}
