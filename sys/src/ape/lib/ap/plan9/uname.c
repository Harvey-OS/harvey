/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
