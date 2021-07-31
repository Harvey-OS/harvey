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
	fprint(2, "usage: %s [-Dd] [-m mnt] [-s srv] [dev...]\n", argv0);
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
	char *as;
	char *ae;
	char *srv;
	char *mnt;

	srv = nil;
	mnt = "/net";

	quotefmtinstall();
	ae = args+sizeof(args);
	as = seprint(args, ae, "ether");
	ARGBEGIN{
	case 'D':
		usbfsdebug++;
		break;
	case 'd':
		usbdebug++;
		as = seprint(as, ae, " -d");
		break;
	case 'm':
		mnt = EARGF(usage());
		break;
	case 's':
		srv = EARGF(usage());
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
