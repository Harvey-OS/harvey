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
#include <thread.h>
#include "usb.h"
#include "hid.h"

typedef struct Parg Parg;

enum
{
	Ndevs = 10,
	Arglen = 80,
	Nargs = 10,
};

static void
usage(void)
{
	fprint(2, "usage: %s [-bdkm] [-a n] [-N nb] [dev...]\n", argv0);
	threadexitsall("usage");
}

void
threadmain(int argc, char **argv)
{
	char args[Arglen];
	char *as, *ae;
	int accel, pena, devid;
	int csps[] = { KbdCSP, PtrCSP, 0 };

	quotefmtinstall();
	pena = 1;
	ae = args+sizeof(args);
	as = seprint(args, ae, "kb");
	ARGBEGIN{
	case 'a':
		accel = strtol(EARGF(usage()), nil, 0);
		as = seprint(as, ae, " -a %d", accel);
		break;
	case 'd':
		usbdebug++;
		as = seprint(as, ae, " -d");
		break;
	case 'k':
		as = seprint(as, ae, " -k");
		pena = 0;
		break;
	case 'm':
		as = seprint(as, ae, " -m");
		pena = 1;
		break;
	case 'N':
		devid = atoi(EARGF(usage()));		/* ignore dev number */
		USED(devid);
		break;
	case 'b':
		as = seprint(as, ae, " -b");
		break;
	default:
		usage();
	}ARGEND;

	rfork(RFNOTEG);
	fmtinstall('U', Ufmt);
	threadsetgrp(threadid());
	if(pena == 0)
		csps[1] = 0;
	startdevs(args, argv, argc, matchdevcsp, csps, kbmain);
	threadexits(nil);
}
