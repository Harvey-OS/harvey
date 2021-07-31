#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"

#include "dat.h"
#include "fns.h"

void
devspeed(Device *d, int ls)
{
	if(fprint(d->ctl, "speed %d", !ls) < 0)
		sysfatal("devspeed: write error: %r");
	if(debug)
		fprint(2, "usbd: %D: set speed %s\n", d, ls?"low":"high");
}

void
setup0(Device *d, int type, int req, int value, int index, int count)
{
	if(setupreq(d->ep[0], type, req, value, index, count) < 0)
		sysfatal("usbd: %D: transaction error", d);
}

void
setconfig(Device *d, int n)
{
	setup0(d, RH2D, SET_CONFIGURATION, n, 0, 0);
	d->state = Configured;
}

int
getmaxpkt(Device *d)
{
	DDevice *dd;
	byte buf[8];
	int nr = -1;

	if (setupreq(d->ep[0], RD2H|Rstandard|Rdevice, GET_DESCRIPTOR, (DEVICE<<8)|0, 0, sizeof(buf)) < 0 ||
	   (nr = setupreply(d->ep[0], buf, sizeof(buf))) < sizeof(buf)) {
		fprint(2, "usbd: getmaxpkt: error reading device descriptor for %D, got %d of %d\n", d, nr, sizeof(buf));
		return -1;
	}
	dd = (DDevice*)buf;
	return dd->bMaxPacketSize0;
}

int
setaddress(Device *d, int id)
{
	if (setupreq(d->ep[0], RH2D, SET_ADDRESS, id, 0, 0) < 0) {
		fprint(2, "usbd: set address %D <- %d failed\n", d, id);
		return -1;
	}
	return 0;
}
