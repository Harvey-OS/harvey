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

void
main(void)
{
	Ipifc *ifc, *list;
	Iplifc *lifc;
	int i;

	fmtinstall('I', eipfmt);
	fmtinstall('M', eipfmt);

	list = readipifc("/net", nil, -1);
	for(ifc = list; ifc; ifc = ifc->next){
		print("ipifc %s %d\n", ifc->dev, ifc->mtu);
		for(lifc = ifc->lifc; lifc; lifc = lifc->next)
			print("\t%I %M %I\n", lifc->ip, lifc->mask, lifc->net);
	}
}
