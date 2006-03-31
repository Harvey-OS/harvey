#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

int
uname(struct utsname *n)
{
	n->sysname = getenv("osname");
	if(!n->sysname)
		n->sysname = "Plan9";
	n->nodename = getenv("sysname");
	if(!n->nodename){
		n->nodename = getenv("site");
		if(!n->nodename)
			n->nodename = "?";
	}
	n->release = "4";			/* edition */
	n->version = "0";
	n->machine = getenv("cputype");
	if(!n->machine)
		n->machine = "?";
	if(strcmp(n->machine, "386") == 0)
		n->machine = "i386";		/* for gnu configure */
	return 0;
}
