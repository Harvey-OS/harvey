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
#include <ip.h>

#include "boot.h"

void
configrc(Method* m)
{
	void configloopback(void);
	configloopback();
	bind("#S", "/dev", MAFTER);
	char *argv[] = {"rc", "-m", "/boot/rcmain", "-i", 0,};
	print("Step 1. Run an rc. Set things up.\n");
	switch(fork()){
	case -1:
		print("configrc: fork failed: %r\n");
	case 0:
		exec("/boot/rc", argv);
		fatal("can't exec rc");
	default:
		break;
	}
	while(waitpid() != -1)
		;
	print("Step 2. Run an rc. Verify that things are as you want them.\n");
	switch(fork()){
	case -1:
		print("configrc: fork failed: %r\n");
	case 0:
		exec("/boot/rc", argv);
		fatal("can't exec rc");
	default:
		break;
	}
	while(waitpid() != -1)
		;
	print("rc is done, continuing...\n");
}

int
connectrc(void)
{
	int fd;
	char buf[64];

	// Later, make this anything.
	snprint(buf, sizeof buf, "/srv/fossil");
	fd = open("#s/fossil", 2);
	if (fd < 0)
		werrstr("dial %s: %r", buf);
	return fd;
}
