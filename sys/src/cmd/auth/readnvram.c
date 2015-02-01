/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* readnvram */
#include <u.h>
#include <libc.h>
#include <auth.h>
#include <authsrv.h>

void
main(int, char **)
{
	int i;
	Nvrsafe safe;

	quotefmtinstall();

	memset(&safe, 0, sizeof safe);
	/*
	 * readnvram can return -1 meaning nvram wasn't written,
	 * but safe still holds good data.
	 */
	if(readnvram(&safe, 0) < 0 && safe.authid[0] == '\0') 
		sysfatal("readnvram: %r");

	/*
	 *  only use nvram key if it is non-zero
	 */
	for(i = 0; i < DESKEYLEN; i++)
		if(safe.machkey[i] != 0)
			break;
	if(i == DESKEYLEN)
		sysfatal("bad key");

	fmtinstall('H', encodefmt);
	print("key proto=p9sk1 user=%q dom=%q !hex=%.*H !password=______\n", 
		safe.authid, safe.authdom, DESKEYLEN, safe.machkey);
	exits(0);
}
