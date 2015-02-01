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
#include <ip.h>

void
usage(void)
{
	fprint(2, "usage: %s [-x netmtpt]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int fd, cfd, n;
	char buf[4096], data[128], devdir[40], net[32];

	setnetmtpt(net, sizeof net, nil);

	ARGBEGIN{
	case 'x':
		setnetmtpt(net, sizeof net, EARGF(usage()));
		break;
	}ARGEND;

	sprint(data, "%s/udp!*!echo", net);
	cfd = announce(data, devdir);
	if(cfd < 0)
		sysfatal("can't announce %s: %r", data);
	if(fprint(cfd, "headers") < 0)
		sysfatal("can't set header mode: %r");

	sprint(data, "%s/data", devdir);
	fd = open(data, ORDWR);
	if(fd < 0)
		sysfatal("open %s: %r", data);
	while ((n = read(fd, buf, sizeof buf)) > 0)
		write(fd, buf, n);
	if (n < 0)
		sysfatal("error reading: %r");
	exits(0);
}
