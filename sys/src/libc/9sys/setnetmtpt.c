#include <u.h>
#include <libc.h>

void
setnetmtpt(char *net, int n, char *x)
{
	if(x == nil)
		x = "/net";

	if(*x == '/'){
		strncpy(net, x, n);
		net[n-1] = 0;
	} else {
		snprint(net, n, "/net%s", x);
	}
}
