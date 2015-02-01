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
#include <ip.h>
#include <plumb.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include "dat.h"
#include "fns.h"

char *cookiefile;
char *mtpt = "/mnt/web";
char *service;

Ctl globalctl = 
{
	1,	/* accept cookies */
	1,	/* send cookies */
	10,	/* redirect limit */
	"webfs/2.0 (plan 9)"	/* user agent */
};

void
usage(void)
{
	fprint(2, "usage: webfs [-c cookies] [-m mtpt] [-s service]\n");
	threadexitsall("usage");
}

#include <pool.h>
void
threadmain(int argc, char **argv)
{
	rfork(RFNOTEG);
	ARGBEGIN{
	case 'd':
		mainmem->flags |= POOL_PARANOIA|POOL_ANTAGONISM;
		break;
	case 'D':
		chatty9p++;
		break;
	case 'c':
		cookiefile = EARGF(usage());
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	case 's':
		service = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	quotefmtinstall();
	if(argc != 0)
		usage();

	plumbinit();
	globalctl.useragent = estrdup(globalctl.useragent);
	initcookies(cookiefile);
	initurl();
	initfs();
	threadpostmountsrv(&fs, service, mtpt, MREPL);
	threadexits(nil);
}
