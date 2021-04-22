/*
 * usb/print - usb printer file server
 * BUG: Assumes the printer will be always connected and
 * not hot-plugged. (Otherwise should stay running and
 * listen to errors to keep the device there as long as it has
 * not failed). Also, this is untested and done ad-hoc to
 * replace the print script.
 */
#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"

enum
{
	Qdir = 0,
	Qctl,
	Qraw,
	Qdata,
	Qmax,
};

int
findendpoints(Dev *dev, int devid)
{
	Ep *ep;
	Dev *d;
	Usbdev *ud;
	int i, epout;

	epout = -1;
	ud = dev->usb;
	for(i = 0; i < nelem(ud->ep); i++){
		if((ep = ud->ep[i]) == nil)
			break;
		if(ep->iface->csp != 0x020107)
			continue;
		if(ep->type == Ebulk && (ep->dir == Eboth || ep->dir == Eout))
			if(epout == -1)
				epout = ep->id;
	}
	dprint(2, "print: ep ids: out %d\n", epout);
	if(epout == -1)
		return -1;
	d = openep(dev, epout);
	if(d == nil){
		fprint(2, "print: openep %d: %r\n", epout);
		return -1;
	}
	opendevdata(d, OWRITE);
	if(d->dfd < 0){
		fprint(2, "print: open i/o ep data: %r\n");
		closedev(d);
		return -1;
	}
	dprint(2, "print: ep out %s\n", d->dir);
	if(usbdebug > 1)
		devctl(d, "debug 1");
	devctl(d, "name lp%d", devid);
	return 0;
}

static int
usage(void)
{
	werrstr("usage: usb/print [-N id]");
	return -1;
}

int
printmain(Dev *dev, int argc, char **argv)
{
	int devid;

	devid = dev->id;
	ARGBEGIN{
	case 'N':
		devid = atoi(EARGF(usage()));
		break;
	default:
		return usage();
	}ARGEND
	if(argc != 0)
		return usage();

	if(findendpoints(dev, devid) < 0){
		werrstr("print: endpoints not found");
		return -1;
	}
	return 0;
}
