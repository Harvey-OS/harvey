#include	"all.h"
#include	"mem.h"
#include	"io.h"
#include	"ureg.h"
/*
 *	for the console
 *		setenv bootmode d
 *		setenv netaddr 135.104.117.33
 *		bootp(,egl)96280fs
 */


Vmedevice vmedevtab[] =
{
	{
		0, 0, 0xA8, 1, (void*)0x009000,
		"jaguar0", jaginit, jagintr, 0,
	}, /**/
	{
		0, 1, 0xA9, 4, (void*)0x009800,
		"jaguar1", jaginit, jagintr, 0,
	}, /**/
	{
		0, 0, 0x20, 2, (void*)0x004000,
		"eagle", eagleinit, eagleintr, 0,
	}, /**/
	{
		0, 0, 0xD0, 5, (void*)0xF90000,
		"hsvme", hsvmeinit, hsvmeintr, 0,
	}, /**/
/*	{
		1, 0, 0xD2, 5, (void*)0x010000,
		"cyclone", cyclinit, cyclintr, 0,
	}, /**/
	0
};

#ifndef	DATE
#define	DATE	568011600L+4*3600
#endif

long	mktime		= DATE;				/* set by mkfile */
long	startsb		= 0;

void
otherinit(void)
{
	vmeinit();
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
	 * Ethernet i/o processes
	 */
	eaglestart();

	/*
	 * datakit i/o and timer processes
	 */
	dkstart();

	/*
	 * cyclone fiber communications
	cyclstart();
	 */

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
