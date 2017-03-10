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
#include <mp.h>
#include <libsec.h>
#include <authsrv.h>
#include <bio.h>
#include "authcmdlib.h"
#include <auth.h>

static int
netcrypt(void *key, void *chal)
{
	uint8_t buf[8], *p;

	strncpy((char*)buf, chal, 7);
	buf[7] = '\0';
	for(p = buf; *p && *p != '\n'; p++)
		;
	*p = '\0';
	encrypt(key, buf, 8);
	sprint(chal, "%.2x%.2x%.2x%.2x", buf[0], buf[1], buf[2], buf[3]);
	return 1;
}


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
