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
#include "authcmdlib.h"

void	install(char*, char*, int);
void	usage(void);

void
main(int argc, char *argv[])
{
	char *key;
	char *u;
	char keybuf[DESKEYLEN];

	argv0 = "printnetkey";
	fmtinstall('K', keyfmt);

	ARGBEGIN{
	default:
		usage();
	}ARGEND
	if(argc != 1)
		usage();

	u = argv[0];
	fmtinstall('K', keyfmt);
	
	if(memchr(u, '\0', ANAMELEN) == 0)
		error("bad user name");
	key = findkey(NETKEYDB, u, keybuf);
	if(!key)
		error("%s has no netkey\n", u);
	print("user %s: net key %K\n", u, key);
	exits(0);
}

void
usage(void)
{
	fprint(2, "usage: printnetkey user\n");
	exits("usage");
}
