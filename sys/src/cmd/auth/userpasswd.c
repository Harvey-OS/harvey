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
#include <auth.h>

void
usage(void)
{
	fprint(2, "usage: auth/userpasswd fmt\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	UserPasswd *up;

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();

	up = auth_getuserpasswd(auth_getkey, "proto=pass %s", argv[0]);
	if(up == nil)	/* bug in factotum, fixed but need to reboot servers -rsc, 2/10/2002 */
		up = auth_getuserpasswd(nil, "proto=pass %s", argv[0]);
	if(up == nil)
		sysfatal("getuserpasswd: %r");

	quotefmtinstall();
	print("%s\n%s\n", up->user, up->passwd);
	exits(0);
}
