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
#include <ip.h>
#include <plumb.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include <mp.h>
#include <libsec.h>
#include "dat.h"
#include "fns.h"

static int32_t
_iovfprint(va_list *arg)
{
	int fd;
	int8_t *fmt;
	va_list arg2;

	fd = va_arg(*arg, int);
	fmt = va_arg(*arg, int8_t*);
	arg2 = va_arg(*arg, va_list);
	return vfprint(fd, fmt, arg2);
}

int
iovfprint(Ioproc *io, int fd, int8_t *fmt, va_list arg)
{
	return iocall(io, _iovfprint, fd, fmt, arg);
}

int
ioprint(Ioproc *io, int fd, int8_t *fmt, ...)
{
	int n;
	va_list arg;

	va_start(arg, fmt);
	n = iovfprint(io, fd, fmt, arg);
	va_end(arg);
	return n;
}

static int32_t
_iotlsdial(va_list *arg)
{
	int8_t *addr, *local, *dir;
	int *cfdp, fd, tfd, usetls;
	TLSconn conn;

	addr = va_arg(*arg, int8_t*);
	local = va_arg(*arg, int8_t*);
	dir = va_arg(*arg, int8_t*);
	cfdp = va_arg(*arg, int*);
	usetls = va_arg(*arg, int);

	fd = dial(addr, local, dir, cfdp);
	if(fd < 0)
		return -1;
	if(!usetls)
		return fd;

	memset(&conn, 0, sizeof conn);
	/* does no good, so far anyway */
	// conn.chain = readcertchain("/sys/lib/ssl/vsignss.pem");

	tfd = tlsClient(fd, &conn);
	close(fd);
	if(tfd < 0)
		fprint(2, "%s: tlsClient: %r\n", argv0);
	else {
		/* BUG: check cert here? */
		if(conn.cert)
			free(conn.cert);
	}
	return tfd;
}

int
iotlsdial(Ioproc *io, int8_t *addr, int8_t *local, int8_t *dir, int *cfdp,
	  int usetls)
{
	return iocall(io, _iotlsdial, addr, local, dir, cfdp, usetls);
}
