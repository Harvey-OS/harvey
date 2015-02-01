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
#include <bio.h>
#include "authcmdlib.h"

/*
 * print a key in des standard form
 */
int
keyfmt(Fmt *f)
{
	uchar key[8];
	char buf[32];
	uchar *k;
	int i;

	k = va_arg(f->args, uchar*);
	key[0] = 0;
	for(i = 0; i < 7; i++){
		key[i] |= k[i] >> i;
		key[i] &= ~1;
		key[i+1] = k[i] << (7 - i);
	}
	key[7] &= ~1;
	snprint(buf, sizeof buf,
		"%.3uo %.3uo %.3uo %.3uo %.3uo %.3uo %.3uo %.3uo",
		key[0], key[1], key[2], key[3], key[4], key[5], key[6], key[7]);
	fmtstrcpy(f, buf);
	return 0;
}
