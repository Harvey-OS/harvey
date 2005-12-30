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
