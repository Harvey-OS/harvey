/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* generic old-cw configuration */

#include "all.h"

#ifndef	DATE
#define	DATE 1170808167L
#endif

Timet	fs_mktime = DATE;			/* set by mkfile */

Startsb	startsb[] = {
	"main",		2,
	nil,
};

void
localconfinit(void)
{
	conf.nfile = 40000;
	conf.nodump = 0;
//	conf.nodump = 1;		/* jukebox is r/o */
	conf.firstsb = 13219302;
	conf.recovsb = 0;
	conf.nlgmsg = 100;
	conf.nsmmsg = 500;
}

int (*fsprotocol[])(Msgbuf*) = {
	serve9p1,
	serve9p2,
	nil,
};
