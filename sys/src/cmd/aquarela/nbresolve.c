#include "headers.h"
#include <bio.h>
#include <ndb.h>

int
nbnameresolve(NbName nbname, uchar *ipaddr)
{
	ulong r, ttl;
	char name[NETPATHLEN];
	NbName copy;
	Ndbtuple *t;

	/* for now, just use dns */
	if (nbremotenametablefind(nbname, ipaddr)) {
//print("%B found in cache\n", nbname);
		return 1;
	}
	if (nbnsfindname(nil, nbname, ipaddr, &ttl) == 0) {
		nbremotenametableadd(nbname, ipaddr, ttl);
		return 1;
	}
	nbnamecpy(copy, nbname);
	copy[NbNameLen - 1] = 0;
	nbmkstringfromname(name, sizeof(name), copy);
	t = dnsquery("/net", name, "ip");
	if (t == nil)
		return 0;
	r = parseip(ipaddr, t->line->val);
	ndbfree(t);
	return r != -1;
}
