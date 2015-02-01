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

void
ding(void*, char *s)
{
	if(strstr(s, "alarm"))
		noted(NCONT);
	else
		noted(NDFLT);
}

void
main(void)
{
	char buf[256];

	alarm(100);
	while(read(0, buf, sizeof(buf)) > 0)
		;
	alarm(0);
	exits(0);
}
