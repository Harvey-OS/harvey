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
_ioopen(va_list *arg)
{
	char *path;
	int mode;

	path = va_arg(*arg, char*);
	mode = va_arg(*arg, int);
	return open(path, mode);
}

int
ioopen(Ioproc *io, char *path, int mode)
{
	return iocall(io, _ioopen, path, mode);
}
