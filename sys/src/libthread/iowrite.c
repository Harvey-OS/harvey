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

static int32_t
_iowrite(va_list *arg)
{
	int fd;
	void *a;
	int32_t n;

	fd = va_arg(*arg, int);
	a = va_arg(*arg, void *);
	n = va_arg(*arg, int32_t);
	return write(fd, a, n);
}

int32_t
iowrite(Ioproc *io, int fd, void *a, int32_t n)
{
	return iocall(io, _iowrite, fd, a, n);
}
