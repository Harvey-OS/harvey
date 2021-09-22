#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbfs.h"
#include "ibuddy.h"

typedef struct Parg Parg;

enum
{
	Arglen = 80,
};

static void
usage(void)
{
	fprint(2, "usage: %s [-d] [-a n] [dev...]\n", argv0);
	threadexitsall("usage");
}

/* returns 0  if it matches with the dev */
static int
matchibuddy(char *info, void*)
{
	char buf[50];

	snprint(buf, sizeof(buf), "vid %#06x did %#06x", Ibuddyvid, Ibuddydid);
	dsprint(2, "ibuddy: %s %s\n", buf, info);
	if(strstr(info, buf) != nil){
			return 0;
	}
	return -1;
}

void
threadmain(int argc, char **argv)
{
	char args[Arglen];
	char *mnt;
	char *srv;
	char *as;
	char *ae;

	mnt = "/n/ibuddy";
	srv = nil;

	quotefmtinstall();
	ae = args+sizeof(args);
	as = seprint(args, ae, "ibuddy");
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
		break;
	default:
		usage();
	}ARGEND;

	rfork(RFNOTEG);
	fmtinstall('U', Ufmt);
	threadsetgrp(threadid());
	usbfsinit(srv, mnt, &usbdirfs, MAFTER|MCREATE);
	startdevs(args, argv, argc, matchibuddy, nil, ibuddymain);
	threadexits(nil);
}
