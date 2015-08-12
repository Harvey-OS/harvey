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
#include <authsrv.h>
#include <bio.h>
#include <ndb.h>

int
authdial(char *netroot, char *dom)
{
	char server[Ndbvlen];
	Ndbtuple *nt;

	if(dom != nil) {
		/* look up an auth server in an authentication domain */
		nt = csgetval(netroot, "authdom", dom, "auth", server);

		/* if that didn't work, just try the IP domain */
		if(nt == nil)
			nt = csgetval(netroot, "dom", dom, "auth", server);
		if(nt == nil) {
			werrstr("no auth server found for %s", dom);
			return -1;
		}
		ndbfree(nt);
		return dial(netmkaddr(server, netroot, "ticket"), 0, 0, 0);
	} else {
		/* look for one relative to my machine */
		return dial(netmkaddr("$auth", netroot, "ticket"), 0, 0, 0);
	}
}
