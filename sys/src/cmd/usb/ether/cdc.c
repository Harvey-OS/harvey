/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
