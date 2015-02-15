/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * cat standard input until you get a zero byte
 */

#include <u.h>
#include <libc.h>

void
main(void)
{
	char buf[4096];
	char *p;
	int n;

	while((n = read(0, buf, sizeof(buf))) > 0){
		p = memchr(buf, 0, n);
		if(p != nil)
			n = p-buf;
		if(n > 0)
			write(1, buf, n);
		if(p != nil)
			break;
	}
	exits(0);
}

