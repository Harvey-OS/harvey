#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbfs.h"
#include "ccid.h"
#include "eprpc.h"
#include "tagrd.h"

Cinfo scr3310info[] = {
	{ Scr3310Vid,	Scr3310Did },
	{ 0,		0 },
};


int
matchscr3310(char *info)
{
	Cinfo *ip;
	char buf[50];

	for(ip = scr3310info; ip->vid != 0; ip++){
		snprint(buf, sizeof buf, "vid %#06x did %#06x",
			ip->vid, ip->did);
		dcprint(2, "ccid: %s %s", buf, info);
		if(strstr(info, buf) != nil)
			return 0;
	}
	return -1;
}


static int
scr3310init(Ccid *)
{
	print("scr3310: INIT\n");

	return 0;
}


Iccops belgops = {
	.init = scr3310init,
};



