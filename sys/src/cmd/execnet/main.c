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


