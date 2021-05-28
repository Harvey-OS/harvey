/*
 * Standard usb ethernet communications device.
 */
#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include "usb.h"
#include "usbfs.h"
#include "ether.h"

static int
okclass(Iface *iface)
{
	return Class(iface->csp) == Clcomms && Subclass(iface->csp) == Scether;
}

static int
getmac(Ether *ether)
{
	int i;
	Usbdev *ud;
	uchar *b;
	Desc *dd;
	char *mac;

	ud = ether->dev->usb;

	for(i = 0; i < nelem(ud->ddesc); i++)
		if((dd = ud->ddesc[i]) != nil && okclass(dd->iface)){
			b = (uchar*)&dd->data;
			if(b[1] == Dfunction && b[2] == Fnether){
				mac = loaddevstr(ether->dev, b[3]);
				if(mac != nil && strlen(mac) != 12){
					free(mac);
					mac = nil;
				}
				if(mac != nil){
					parseaddr(ether->addr, mac);
					free(mac);
					return 0;
				}
			}
		}
	return -1;
}

int
cdcreset(Ether *ether)
{
	Usbdev *ud;
	Ep *ep;
	int i;

	/*
	 * Assume that all communication devices are going to
	 * be std. ethernet communication devices. Specific controllers
	 * must have been probed first.
	 */
	ud = ether->dev->usb;
	if(ud->class == Clcomms)
		return getmac(ether);
	if(getmac(ether) == 0){
		for(i = 0; i < Nep; i++){
			ep = ud->ep[i];
			if(ep == nil)
				continue;
			if(ep->iface == nil || !okclass(ep->iface))
				continue;
			if(ep->conf->cval != 1)
				usbcmd(ether->dev, Rh2d|Rstd|Rdev, Rsetconf, ep->conf->cval, 0, nil, 0);
			return 0;
		}
	}
	return -1;
}
