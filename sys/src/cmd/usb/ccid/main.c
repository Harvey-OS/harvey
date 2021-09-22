#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbfs.h"
#include "ccid.h"
#include "atr.h"
#include "eprpc.h"
#include "tagrd.h"

typedef struct Parg Parg;

enum {
	Arglen = 80,
};

Cinfo cinfo[] = {
	{ TagrdVid,	TagrdDid },
	{ 0,		0 },
};

static void
usage(void)
{
	fprint(2, "usage: %s [-d] [-a n] [dev...]\n", argv0);
	threadexitsall("usage");
}

static int
matchccid(char *info, void*)
{
	if(matchtagrd(info) == 0)
		return 0;
	if(matchscr3310(info) == 0)
		return 0;
	return -1;
}
int	mainstacksize = 64*1024;
void
threadmain(int argc, char **argv)
{
	char *mnt, *srv, *as, *ae;
	char args[Arglen];
	int csps[] = { CcidCSP,  CcidCSP2, 0 };

	mnt = "/dev";
	srv = nil;

	quotefmtinstall();
	ae = args + sizeof args;
	as = seprint(args, ae, "ccid");
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
		break;
	}ARGEND;

	rfork(RFNOTEG);
	fmtinstall('U', Ufmt);
	threadsetgrp(threadid());

	usbfsinit(srv, mnt, &usbdirfs, MAFTER|MCREATE);
	startdevs(args, argv, argc, matchccid, csps, ccidmain);
	threadexits(nil);
}
