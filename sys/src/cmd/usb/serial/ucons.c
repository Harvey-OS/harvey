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
		dsprint(2, "serial: %s %s", buf, info);
		if(strstr(info, buf) != nil)
			return 0;
	}
	return -1;
}


Serialops uconsops = {
	.init =		serialnop,
	.getparam =	serialnop,
	.setparam =	serialnop,
	.clearpipes =	serialnop,
	.sendlines =	serialnop,
	.modemctl =	serialnopctl,
	.setbreak =	serialnopctl,
};
