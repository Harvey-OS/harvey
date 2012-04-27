/* fs-specific configuration */

#include "all.h"

#ifndef	DATE
#define	DATE 1170808167L
#endif

Timet	fs_mktime = DATE;			/* set by mkfile */

Startsb	startsb[] = {
/*	"main",		2,	*/
	"main",		810988,	/* discontinuity before sb @ 696262 */
	0
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
	serve9p1,
	serve9p2,
	nil,
};
