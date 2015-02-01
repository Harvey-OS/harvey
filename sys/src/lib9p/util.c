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
#include <fcall.h>
#include <thread.h>
#include "9p.h"

void
readbuf(Req *r, void *s, long n)
{
	r->ofcall.count = r->ifcall.count;
	if(r->ifcall.offset >= n){
		r->ofcall.count = 0;
		return;
	}
	if(r->ifcall.offset+r->ofcall.count > n)
		r->ofcall.count = n - r->ifcall.offset;
	memmove(r->ofcall.data, (char*)s+r->ifcall.offset, r->ofcall.count);
}

void
readstr(Req *r, char *s)
{
	readbuf(r, s, strlen(s));
}
