#include <stdlib.h>
#include <sys/utsname.h>

int
uname(struct utsname *n)
{
	n->sysname = "Plan9";
	n->nodename = getenv("sysname");
	if(!n->nodename){
		n->nodename = getenv("site");
		if(!n->nodename)
			n->nodename = "?";
	}
	n->release = "1";
	n->version = "0";
	n->machine = getenv("terminal");
	if(!n->machine){
		n->machine = getenv("cputype");
		if(!n->machine)
			n->machine = "?";
	}
	return 0;
}
