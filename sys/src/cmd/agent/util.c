#include "agent.h"

void
memrandom(void *p, int n)
{
	ulong *lp;
	uchar *cp;

	for(lp = p; n >= sizeof(ulong); n -= sizeof(ulong))
		*lp++ = fastrand();
	for(cp = (uchar*)lp; n > 0; n--)
		*cp++ = fastrand();
}

char*
safecpy(char *to, char *from, int n)
{
	memset(to, 0, n);
	if(n == 1)
		return to;
	strncpy(to, from, n-1);
	return to;
}
