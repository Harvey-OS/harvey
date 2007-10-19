#include <u.h>
#include <libc.h>
#include <authsrv.h>
#include <bio.h>
#include <ndb.h>

int
authdial(char *netroot, char *dom)
{
	char *p;
	int rv;

	if(dom == nil)
		/* look for one relative to my machine */
		return dial(netmkaddr("$auth", netroot, "ticket"), 0, 0, 0);

	/* look up an auth server in an authentication domain */
	p = csgetvalue(netroot, "authdom", dom, "auth", nil);

	/* if that didn't work, just try the IP domain */
	if(p == nil)
		p = csgetvalue(netroot, "dom", dom, "auth", nil);
	/*
	 * if that didn't work, try p9auth.$dom.  this is very helpful if
	 * you can't edit /lib/ndb.
	 */
	if(p == nil)
		p = smprint("p9auth.%s", dom);
	if(p == nil){			/* should no longer ever happen */
		werrstr("no auth server found for %s", dom);
		return -1;
	}
	rv = dial(netmkaddr(p, netroot, "ticket"), 0, 0, 0);
	free(p);
	return rv;
}
