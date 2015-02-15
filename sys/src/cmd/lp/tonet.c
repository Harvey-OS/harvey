/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* tonet dest - copy stdin to dest, after dialing dest */
#include <u.h>
#include <libc.h>

enum { TIMEOUT = 10*60*1000 };

int
alarmhandler(void *, char *note)
{
	if(strcmp(note, "alarm") == 0) {
		fprint(2, "alarm\n");
		return 1;
	} else
		return 0;
}

void
pass(int in, int out)
{
	int rv;
	static char buf[4096];

	for(;;) {
		alarm(TIMEOUT);		/* to break hanging */
		rv = read(in, buf, sizeof buf);
		if (rv == 0)
			break;
		if(rv < 0)
			sysfatal("read error: %r");
		if(write(out, buf, rv) != rv)
			sysfatal("write error: %r");
	}
	alarm(0);
}

static void
usage(void)
{
	fprint(2, "usage: %s network!destination!service\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int netfd;

	argv0 = argv[0];
	if (argc != 2)
		usage();

	atnotify(alarmhandler, 1);

	netfd = dial(argv[1], "net", 0, 0);
	if (netfd < 0)
		sysfatal("can't dial %s: %r", argv[1]);
	pass(0, netfd);
	exits(0);
}
