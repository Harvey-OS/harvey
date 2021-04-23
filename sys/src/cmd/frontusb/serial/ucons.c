#include <u.h>
#include <libc.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include "usb.h"
#include "serial.h"

enum {
	Net20DCVid	= 0x0525,	/* Ajays usb debug cable */
	Net20DCDid	= 0x127a,

	HuaweiVid	= 0x12d1,
	HuaweiE220	= 0x1003,
};

Cinfo uconsinfo[] = {
	{ Net20DCVid,	Net20DCDid,	1 },
	{ HuaweiVid,	HuaweiE220,	2 },
	{ 0,		0,		0 },
};

int
uconsprobe(Serial *ser)
{
	Usbdev *ud = ser->dev->usb;
	Cinfo *ip;

	if((ip = matchid(uconsinfo, ud->vid, ud->did)) == nil)
		return -1;
	ser->nifcs = ip->cid;
	return 0;
}
