/*
 * usb/disk - usb mass storage file server
 */
#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include "scsireq.h"
#include "usb.h"
#include "usbfs.h"
#include "ums.h"

enum
{
	Arglen = 80,
};

static void
usage(void)
{
	fprint(2, "usage: %s [-Dd] [-N nb] [-m mnt] [-s srv] [dev...]\n", argv0);
	threadexitsall("usage");
}

static int csps[] = {
	CSP(Clstorage,Subatapi,Protobulk),
	CSP(Clstorage,Sub8070,Protobulk),
	CSP(Clstorage,Subscsi,Protobulk),
	0,
};

void
threadmain(int argc, char **argv)
{
	char args[Arglen];
	char *as, *ae, *srv, *mnt;

	srv = nil;
	mnt = "/n/disk";

	quotefmtinstall();
	ae = args+sizeof(args);
	as = seprint(args, ae, "disk");
	ARGBEGIN{
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
	usbfsinit(srv, mnt, &usbdirfs, MBEFORE);
	startdevs(args, argv, argc, matchdevcsp, csps, diskmain);
	threadexits(nil);
}
