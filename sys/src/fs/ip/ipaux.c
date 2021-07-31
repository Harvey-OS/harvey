#include "all.h"

#include "../ip/ip.h"

int
chartoea(uchar *ea, char *cp)
{
	int i, h, c;

	h = 0;
	for(i=0; i<Easize*2; i++) {
		c = *cp++;
		if(c >= '0' && c <= '9')
			c = c - '0';
		else
		if(c >= 'a' && c <= 'f')
			c = c - 'a' + 10;
		else
		if(c >= 'A' && c <= 'F')
			c = c - 'A' + 10;
		else
			return 1;
		h = (h*16) + c;
		if(i & 1) {
			*ea++ = h;
			h = 0;
		}
	}
	if(*cp != 0)
		return 1;
	return 0;
}

int
chartoip(uchar *pa, char *cp)
{
	int i, c, h;

	for(i=0;;) {
		h = 0;
		for(;;) {
			c = *cp++;
			if(c < '0' || c > '9')
				break;
			h = (h*10) + (c-'0');
		}
		*pa++ = h;
		i++;
		if(i == Pasize) {
			if(c != 0)
				return 1;
			return 0;
		}
		if(c != '.')
			return 1;
	}
}

void
getipa(Ifc *ifc, int a)
{

	memmove(ifc->ipa, ipaddr[a].sysip, Pasize);
	memmove(ifc->netgate, ipaddr[a].defgwip, Pasize);
	ifc->ipaddr = nhgetl(ifc->ipa);
	ifc->mask = nhgetl(ipaddr[a].defmask);
	ifc->cmask = ipclassmask(ifc->ipa);
}

int
isvalidip(uchar ip[Pasize])
{
	if(ip[0] || ip[1] || ip[2] || ip[3])
		return 1;
	return 0;
}
