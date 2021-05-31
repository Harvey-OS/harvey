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
#include <fcall.h>
#include <thread.h>
#include <9p.h>

static void
tforker(void (*fn)(void*), void *arg, int rflag)
{
	procrfork(fn, arg, 32*1024, rflag);
}

void
threadlistensrv(Srv *s, char *addr)
{
	_forker = tforker;
	_listensrv(s, addr);
}

void
threadpostmountsrv(Srv *s, char *name, char *mtpt, int flag)
{
	_forker = tforker;
	_postmountsrv(s, name, mtpt, flag);
}
