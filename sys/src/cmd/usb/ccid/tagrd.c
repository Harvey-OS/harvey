#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbfs.h"
#include "ccid.h"
#include "eprpc.h"
#include "tagrd.h"

Cinfo taginfo[] = {
	{ TagrdVid,	TagrdDid },
	{ 0,		0 },
};


int
matchtagrd(char *info)
{
	Cinfo *ip;
	char buf[50];

	for(ip = taginfo; ip->vid != 0; ip++){
		snprint(buf, sizeof buf, "vid %#06x did %#06x",
			ip->vid, ip->did);
		dcprint(2, "ccid: %s %s", buf, info);
		if(strstr(info, buf) != nil)
			return 0;
	}
	return -1;
}



enum{
	Maxfwarelen = 32
};


uchar unlockser[4] = {0xff, 0x10, 0x96, 0x79};

static int
tagrdinit(Ccid *ccid)
{
	Cmsg *c;
	char *s;

	s = nil;
	dcprint(2, "tagrd: special ttag init\n");
	
	/* BUG, strange unlock sequence, I don't really know what it does */
	c = iccmsgrpc(ccid, unlockser, 4, XfrBlockTyp, 0);
	if( c == nil )
		goto Error;
	free(c);

	return 0;
Error:
	free(c);
	free(s);
	fprint(2, "tagrd: init, rpc error\n");
	return -1;
}


Iccops tagops = {
	.init = tagrdinit,
};


