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
#include <bio.h>

#include "modem.h"

static Modem modems[1];

static void
usage(void)
{
	fprint(2, "%s: usage: %s [-v] number pages\n", argv0, argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int fd, cfd, r;
	Modem *m;
	char *addr;

	m = &modems[0];

	ARGBEGIN{
	case 'v':
		vflag = 1;
		break;
	default:
		usage();
		break;

	}ARGEND

	if(argc <= 1)
		usage();
	verbose("send: %s %s...", argv[0], argv[1]);

	addr = netmkaddr(*argv, "telco", "fax!9600");
	fd = dial(addr, 0, 0, &cfd);
	if(fd < 0){
		fprint(2, "faxsend: can't dial %s: %r\n", addr);
		exits("Retry, can't dial");
	}
	initmodem(m, fd, cfd, 0, 0);
	argc--; argv++;
	r = faxsend(m, argc, argv);
	if(r != Eok){
		fprint(2, "faxsend: %s\n", m->error);
		syslog(0, "fax", "failed %s %s: %s", argv[0], argv[1], m->error); 
		exits(m->error);
	}
	syslog(0, "fax", "success %s %s", argv[0], argv[1]); 
	exits(0);
}
