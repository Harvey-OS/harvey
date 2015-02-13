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

/*
 * like fprint but be sure to deliver as a single write.
 * (fprint uses a small write buffer.)
 */
void
xfprint(int fd, int8_t *fmt, ...)
{
	int8_t *s;
	va_list arg;

	va_start(arg, fmt);
	s = vsmprint(fmt, arg);
	va_end(arg);
	if(s == nil)
		sysfatal("smprint: %r");
	write(fd, s, strlen(s));
	free(s);
}

void
main(int argc, int8_t **argv)
{
	int fd;
	int8_t dir[512];

	fd = open("/dev/acme/ctl", OWRITE);
	if(fd < 0)
		exits(0);
	getwd(dir, 512);
	if(dir[0]!=0 && dir[strlen(dir)-1]=='/')
		dir[strlen(dir)-1] = 0;
	xfprint(fd, "name %s/-%s\n",  dir, argc > 1 ? argv[1] : "rc");
	xfprint(fd, "dumpdir %s\n", dir);
	exits(0);
}
