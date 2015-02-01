/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * deliver recipient fromfile mbox - append stdin to mbox with locking & logging
 */
#include "dat.h"
#include "common.h"

void
usage(void)
{
	fprint(2, "usage: %s recipient fromfile mbox\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int bytes, fd, i;
	char now[30];
	char *deliveredto;
	Addr *a;
	Mlock *l;

	ARGBEGIN{
	}ARGEND;

	if(argc != 3)
		usage();

	deliveredto = strrchr(argv[0], '!');
	if(deliveredto == nil)
		deliveredto = argv[0];
	else
		deliveredto++;
	a = readaddrs(argv[1], nil);
	if(a == nil)
		sysfatal("missing from address");

	l = syslock(argv[2]);

	/* append to mbox */
	i = 0;
retry:
	fd = open(argv[2], OWRITE);
	if(fd < 0){
		rerrstr(now, sizeof(now));
		if(strstr(now, "exclusive lock") && i++ < 20){
			sleep(500);	/* wait for lock to go away */
			goto retry;
		}
		sysfatal("opening mailbox: %r");
	}
	seek(fd, 0, 2);
	strncpy(now, ctime(time(0)), sizeof(now));
	now[28] = 0;
	if(fprint(fd, "From %s %s\n", a->val, now) < 0)
		sysfatal("writing mailbox: %r");

	/* copy message handles escapes and any needed new lines */
	bytes = appendfiletombox(0, fd);
	if(bytes < 0)
		sysfatal("writing mailbox: %r");

	close(fd);
	sysunlock(l);

	/* log it */
	syslog(0, "mail", "delivered %s From %s %s (%s) %d", deliveredto,
		a->val, now, argv[0], bytes);
	exits(0);
}
