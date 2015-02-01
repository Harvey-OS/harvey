/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

NbGlobals nbglobals;

NbName nbnameany = { '*' };

int
nbinit(void)
{
	Ipifc *ipifc;
	int i;
	fmtinstall('I', eipfmt);
	fmtinstall('B', nbnamefmt);
	ipifc = readipifc("/net", nil, 0);
	if (ipifc == nil || ipifc->lifc == nil) {
		print("no network interface");
		return -1;
	}
	ipmove(nbglobals.myipaddr, ipifc->lifc->ip);
	ipmove(nbglobals.bcastaddr, ipifc->lifc->ip);
	nbmknamefromstring(nbglobals.myname, sysname());
	for (i = 0; i < IPaddrlen; i++)
		nbglobals.bcastaddr[i] |= ~ipifc->lifc->mask[i];
	return 0;
}
