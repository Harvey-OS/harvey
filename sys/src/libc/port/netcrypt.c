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

int
netcrypt(void *key, void *chal)
{
	uchar buf[8], *p;

	strncpy((char*)buf, chal, 7);
	buf[7] = '\0';
	for(p = buf; *p && *p != '\n'; p++)
		;
	*p = '\0';
	encrypt(key, buf, 8);
	sprint(chal, "%.2ux%.2ux%.2ux%.2ux", buf[0], buf[1], buf[2], buf[3]);
	return 1;
}
