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
	/*
	 * Assume that all communication devices are going to
	 * be std. ethernet communication devices. Specific controllers
	 * must have been probed first.
	 * NB: This ignores unions.
	 */
	if(ether->dev->usb->class == Clcomms)
		return getmac(ether);
	return -1;
}
