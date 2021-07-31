/*
 * usb/print - usb printer
 */
#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"

enum
{
	Arglen = 80,
};

static void
usage(void)
{
	fprint(2, "usage: %s [-d] [dev...]\n", argv0);
	threadexitsall("usage");
}

static int csps[] = { 0x020107, 0 };

extern int printmain(Dev*, int, char**);

void
threadmain(int argc, char **argv)
{
	char args[Arglen];
	char *ae;

	quotefmtinstall();
	ae = args+sizeof(args);
	seprint(args, ae, "print");
	ARGBEGIN{
	case 'd':
		usbdebug++;
		break;
	default:
		usage();
	}ARGEND

	rfork(RFNOTEG);
	threadsetgrp(threadid());
	fmtinstall('U', Ufmt);
	startdevs(args, argv, argc, matchdevcsp, csps, printmain);
	threadexits(nil);
}
