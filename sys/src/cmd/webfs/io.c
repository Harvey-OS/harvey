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

static long
_iovfprint(va_list *arg)
{
	int fd;
	char *fmt;
	va_list arg2;

	fd = va_arg(*arg, int);
	fmt = va_arg(*arg, char*);
	arg2 = va_arg(*arg, va_list);
	return vfprint(fd, fmt, arg2);
}

int
iovfprint(Ioproc *io, int fd, char *fmt, va_list arg)
{
	return iocall(io, _iovfprint, fd, fmt, arg);
}

int
ioprint(Ioproc *io, int fd, char *fmt, ...)
{
	int n;
	va_list arg;

	va_start(arg, fmt);
	n = iovfprint(io, fd, fmt, arg);
	va_end(arg);
	return n;
}

static long
_iotlsdial(va_list *arg)
{
	char *addr, *local, *dir;
	int *cfdp, fd, tfd, usetls;
	TLSconn conn;

	addr = va_arg(*arg, char*);
	local = va_arg(*arg, char*);
	dir = va_arg(*arg, char*);
	cfdp = va_arg(*arg, int*);
	usetls = va_arg(*arg, int);

	fd = dial(addr, local, dir, cfdp);
	if(fd < 0)
		return -1;
	if(!usetls)
		return fd;

	memset(&conn, 0, sizeof conn);
	tfd = tlsClient(fd, &conn);
	if(tfd < 0){
		print("tls %r\n");
		close(fd);
		return -1;
	}
	/* BUG: check cert here? */
	if(conn.cert)
		free(conn.cert);
	close(fd);
	return tfd;
}

int
iotlsdial(Ioproc *io, char *addr, char *local, char *dir, int *cfdp, int usetls)
{
	return iocall(io, _iotlsdial, addr, local, dir, cfdp, usetls);
}
