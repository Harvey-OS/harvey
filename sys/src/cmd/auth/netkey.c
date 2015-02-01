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


void
usage(void)
{
	fprint(2, "usage: netkey\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	char buf[32], pass[32], key[DESKEYLEN];
	char *s;
	int n;

	ARGBEGIN{
	default:
		usage();
	}ARGEND
	if(argc)
		usage();

	s = getenv("service");
	if(s && strcmp(s, "cpu") == 0){
		fprint(2, "netkey must not be run on the cpu server\n");
		exits("boofhead");
	}

	readln("Password: ", pass, sizeof pass, 1);
	passtokey(key, pass);

	for(;;){
		print("challenge: ");
		n = read(0, buf, sizeof buf - 1);
		if(n <= 0)
			exits(0);
		buf[n] = '\0';
		n = strtol(buf, 0, 10);
		snprint(buf, sizeof buf, "%d", n);
		netcrypt(key, buf);
		print("response: %s\n", buf);
	}
}
