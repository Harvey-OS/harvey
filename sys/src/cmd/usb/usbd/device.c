#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbproto.h"

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

int
getmaxpkt(Device *d)
{
	int nr;
	byte buf[8];
	DDevice *dd;

	nr = setupreq(d, RD2H|Rstandard|Rdevice, GET_DESCRIPTOR, (DEVICE<<8)|0, 0, buf, sizeof(buf));
	if(nr < sizeof(buf)) {
		fprint(2, "usbd: getmaxpkt: error reading device descriptor for %D, got %d of %d\n", d, nr, sizeof(buf));
		return -1;
	}
	dd = (DDevice*)buf;
	return dd->bMaxPacketSize0;
}

int
setaddress(Device *d, int id)
{
	if(setupreq(d, RH2D, SET_ADDRESS, id, 0, nil, 0) < 0) {
		fprint(2, "usbd: set address %D <- %d failed\n", d, id);
		return -1;
	}
	return 0;
}

int
setconfig(Device *d, int cval)
{
	int i, j, ret;
	Dconf *c;
	Dalt *alt;
	Dinf *intf;
	Endpt *ep;

	c = nil;
	for(i = 0; i < d->nconf; i++) {
		if(d->config[i].cval == cval) {
			c = &d->config[i];
			break;
		}
	}
	if(c == nil) {
		werrstr("setconfig: cval %d not found", cval);
		return -1;
	}
	if(setupreq(d, RH2D, SET_CONFIGURATION, cval, 0, nil, 0) < 0)
		return -1;

	/* BUG: temporary hack; the "class" stuff does NOT belong in the driver! */
	if(d->csp != 0)
		fprint(d->ctl, "class 15 0 0x%lux", d->csp);
	/* ... */

	for(i = 0; i < c->nif; i++) {
		intf = &c->iface[i];

		/* BUG: temporary hack; the "class" stuff does NOT belong in the driver! */
		if((Class(intf->csp) == 1 || Class(intf->csp) == 3) && Subclass(intf->csp) == 1)
			fprint(d->ctl, "class 15 0 0x%lux", intf->csp);
		/* ... */

		alt = intf->alt;
		if(alt == nil)
			continue;

		/* BUG: temporary hack -- first alternate is often a `disabled' state */
		if(alt->next != nil && alt->next->npt > alt->npt)
			alt = alt->next;
		/* ... */

		for(j = 0; j < alt->npt; j++) {
			ep = &alt->ep[j];

			/* BUG: temporary hack; the "class" stuff does NOT belong in the driver! */
			if(fprint(d->ctl, "class 15 %d 0x%lux", ep->addr, intf->csp) < 0)
				fprint(2, "set class addr %d failed: %r\n", ep->addr);
			/* ... */

			if(ep->maxpkt == 0)
				continue;
			ret = -1;
			switch(ep->type) {
			case Econtrol:
				ret = fprint(d->ctl, "ep %d bulk rw %d 1", ep->addr, ep->maxpkt);
				break;
			case Eiso:
				ret = fprint(d->ctl, "ep %d 1 %s 1 %d000", ep->addr, ep->dir == Ein ? "r" : "w", ep->maxpkt);
				break;
			case Ebulk:
				ret = fprint(d->ctl, "ep %d bulk %s %d 1", ep->addr, ep->dir == Ein ? "r" : "w", ep->maxpkt);
				break;
			case Eintr:
				ret = fprint(d->ctl, "ep %d bulk %s %d 1", ep->addr, ep->dir == Ein ? "r" : "w", ep->maxpkt);
				break;
			}
			if(ret < 0)
				fprint(2, "ep %d message failed\n", ep->addr);
		}
	}
	d->state = Configured;
	return 0;
}

/*
int
setalternate(Device *d, int i, int alt)
{
	if(setupreq(d, RH2D, SET_CONFIGURATION, n, 0, nil, 0) < 0)
		return -1;
	return 0;
}
*/
