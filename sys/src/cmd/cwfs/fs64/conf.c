/* fs64-specific configuration */

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
	conf.nodump = 0;
	conf.dumpreread = 1;
	conf.firstsb = 0;	/* time- & jukebox-dependent optimisation */
	conf.recovsb = 0;
	conf.nlgmsg = 1100;	/* @8576 bytes, for packets */
	conf.nsmmsg = 500;	/* @128 bytes */
}

int (*fsprotocol[])(Msgbuf*) = {
	/* 64-bit file servers can't serve 9P1 correctly: NAMELEN is too big */
	serve9p2,
	nil,
};
