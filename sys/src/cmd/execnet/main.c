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
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "dat.h"

void
usage(void)
{
	fprint(2, "usage: execnet [-n exec] [/net]\n");
	exits("usage");
}

void
threadmain(int argc, char **argv)
{
	char *net;

//extern long _threaddebuglevel;
//_threaddebuglevel = 1<<20;	/* DBGNOTE */

	rfork(RFNOTEG);
	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 'n':
		setexecname(EARGF(usage()));
		break;
	}ARGEND

	switch(argc){
	default:
		usage();
	case 0:
		net = "/net";
		break;
	case 1:
		net = argv[0];
		break;
	}

	quotefmtinstall();

	initfs();
	threadpostmountsrv(&fs, nil, net, MBEFORE);
	threadexits(nil);
}


