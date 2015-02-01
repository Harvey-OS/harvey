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
#include <bin.h>
#include <httpd.h>

int
hurlfmt(Fmt *f)
{
	char buf[HMaxWord*2];
	Rune r;
	char *s;
	int t;

	s = va_arg(f->args, char*);
	for(t = 0; t < sizeof(buf) - 8; ){
		s += chartorune(&r, s);
		if(r == 0)
			break;
		if(r <= ' ' || r == '%' || r >= Runeself)
			t += snprint(&buf[t], sizeof(buf)-t, "%%%2.2x", r);
		else
			buf[t++] = r;
	}
	buf[t] = 0;
	return fmtstrcpy(f, buf);
}
