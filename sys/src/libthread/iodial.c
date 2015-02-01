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
#include <thread.h>
#include "threadimpl.h"

static long
_iodial(va_list *arg)
{
	char *addr, *local, *dir;
	int *cdfp;

	addr = va_arg(*arg, char*);
	local = va_arg(*arg, char*);
	dir = va_arg(*arg, char*);
	cdfp = va_arg(*arg, int*);

	return dial(addr, local, dir, cdfp);
}

int
iodial(Ioproc *io, char *addr, char *local, char *dir, int *cdfp)
{
	return iocall(io, _iodial, addr, local, dir, cdfp);
}
