#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbfs.h"
#include "serial.h"
#include "ucons.h"

Cinfo uconsinfo[] = {
	{ Net20DCVid,	Net20DCDid },
	{ 0,		0 },
};

int
uconsmatch(char *info)
{
	Cinfo *ip;
	char buf[50];

	for(ip = uconsinfo; ip->vid != 0; ip++){
		snprint(buf, sizeof buf, "vid %#06x did %#06x",
			ip->vid, ip->did);
		dsprint(2, "serial: %s %s\n", buf, info);
		if(strstr(info, buf) != nil)
			return 0;
	}
	return -1;
}

static int
ucseteps(Serial *ser)
{
	ser->maxread = ser->maxwrite = 8;
	devctl(ser->epin,  "maxpkt 8");
	devctl(ser->epout, "maxpkt 8");
	return 0;
}

/* all nops */
Serialops uconsops = {
	.seteps = ucseteps,
};
