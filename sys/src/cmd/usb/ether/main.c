/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * usb/ether - usb ethernet adapter.
 * BUG: This should use /dev/etherfile to
 * use the kernel ether device code.
 */
#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include "usb.h"
#include "usbfs.h"
#include "ether.h"

enum
{
	Arglen = 80,
};

static void
usage(void)
{
	fprint(2, "usage: %s [-a addr] [-Dd] [-N nb] [-m mnt] [-s srv] [dev...]\n", argv0);
	threadexitsall("usage");
}

/*
 * Ether devices may be weird.
 * Be optimistic and try to use any communication
 * device or one of the `vendor specific class' devices
 * that we know are ethernets.
 */
static int
matchether(char *info, void*)
{
	Cinfo *ip;
	char buf[50];

	/*
	 * I have an ether reporting comms.0.0
	 */
	if(strstr(info, "comms") != nil)
		return 0;
	for(ip = cinfo; ip->vid != 0; ip++){
		snprint(buf, sizeof(buf), "vid %#06x did %#06x", ip->vid, ip->did);
		if(strstr(info, buf) != nil)
			return 0;
	}
	return -1;
}

void
threadmain(int argc, char **argv)
{
	char args[Arglen];
	char *as, *ae, *srv, *mnt;

	srv = nil;
	mnt = "/net";

	quotefmtinstall();
	ae = args+sizeof(args);
	as = seprint(args, ae, "ether");
	ARGBEGIN{
	case 'a':
		as = seprint(as, ae, " -a %s", EARGF(usage()));
		break;
	case 'D':
		usbfsdebug++;
		break;
	case 'd':
		usbdebug++;
		as = seprint(as, ae, " -d");
		break;
	case 'N':
		as = seprint(as, ae, " -N %s", EARGF(usage()));
		break;
	case 'm':
		mnt = EARGF(usage());
		break;
	case 's':
		srv = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	rfork(RFNOTEG);
	threadsetgrp(threadid());
	fmtinstall('U', Ufmt);
	usbfsinit(srv, mnt, &usbdirfs, MAFTER|MCREATE);
	startdevs(args, argv, argc, matchether, nil, ethermain);
	threadexits(nil);
}
