/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * IMPORTANT!  DO NOT ADD LIBRARY CALLS TO THIS FILE.
 * The entire text image must fit on one page
 * (and there's no data segment, so any read/write data must be on the stack).
 */

#include <u.h>
#include <libc.h>

int8_t cons[] = "#c/cons";
int8_t boot[] = "/boot/boot";
int8_t dev[] = "/dev";
int8_t c[] = "#c";
int8_t e[] = "#e";
int8_t ec[] = "#ec";
int8_t s[] = "#s";
int8_t srv[] = "/srv";
int8_t env[] = "/env";

void
startboot(int8_t *argv0, int8_t **argv)
{
	int8_t buf[200];

	USED(argv0);
	/*
	 * open the console here so that /boot/boot,
	 * which could be a shell script, can inherit the open fds.
	 */
	open(cons, OREAD);
	open(cons, OWRITE);
	open(cons, OWRITE);
	bind(c, dev, MAFTER);
	bind(ec, env, MAFTER);
	bind(e, env, MCREATE|MAFTER);
	bind(s, srv, MREPL|MCREATE);
	exec(boot, argv);

	rerrstr(buf, sizeof buf);
	buf[sizeof buf - 1] = '\0';
	_exits(buf);
}
