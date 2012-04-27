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
