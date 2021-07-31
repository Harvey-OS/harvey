#include	"all.h"
#include	"mem.h"
#include	"io.h"
#include	"ureg.h"

#ifndef	DATE
#define	DATE	568011600L+4*3600
#endif

long	mktime		= DATE;				/* set by mkfile */
long	startsb		= 0;

void
otherinit(void)
{
	lanceinit(0);
	scsiinit(0);
}

void
touser(void)
{
	int i;

	settime(rtctime());
	boottime = time();

	print("sysinit\n");
	sysinit();

	/*
	 * lance i/o processes
	 */
	lancestart();

	/*
	 * read ahead processes
	 */
	for(i=0; i<conf.nrahead; i++)
		userinit(rahead, 0, "rah");

	/*
	 * server processes
	 */
	for(i=0; i<conf.nserve; i++)
		userinit(serve, 0, "srv");

	/*
	 * worm "dump" copy process
	 */
	userinit(wormcopy, 0, "wcp");

	/*
	 * processes to read the console
	 */
	consserve();

	/*
	 * "sync" copy process
	 */
	u->text = "scp";
	synccopy();
}
