/* choline-specific configuration */

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
	conf.nfile = 60000;
	conf.nodump = 0;
	conf.firstsb = 12565379;
	conf.recovsb = 0;
	conf.nlgmsg = 100;
	conf.nsmmsg = 500;
}

int (*fsprotocol[])(Msgbuf*) = {
	serve9p1,
	serve9p2,
	nil,
};
