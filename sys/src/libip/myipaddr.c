/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <ip.h>

static uchar loopbacknet[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0xff, 0xff,
	127, 0, 0, 0
};
static uchar loopbackmask[IPaddrlen] = {
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0, 0, 0
};

// find first ip addr that isn't the friggin loopback address
// unless there are no others
int
myipaddr(uchar *ip, char *net)
{
	Ipifc *nifc;
	Iplifc *lifc;
	static Ipifc *ifc;
	uchar mynet[IPaddrlen];

	ifc = readipifc(net, ifc, -1);
	for(nifc = ifc; nifc; nifc = nifc->next)
		for(lifc = nifc->lifc; lifc; lifc = lifc->next){
			maskip(lifc->ip, loopbackmask, mynet);
			if(ipcmp(mynet, loopbacknet) == 0){
				continue;
			}
			if(ipcmp(lifc->ip, IPnoaddr) != 0){
				ipmove(ip, lifc->ip);
				return 0;
			}
		}
	ipmove(ip, IPnoaddr);
	return -1;
}
