#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbproto.h"

static int
setupcmd(int fd, uchar *data, int count)
{
	int n;

	n = write(fd, data, count);
	if(n < 0) {
		werrstr("setupcmd: write err: %r");
		return -1;
	}
	if(n != count) {
		werrstr("setupcmd: short write: %d of %d", n, count);
		return -1;
	}
	return n;
}

static int
setupout(int fd, uchar *req, uchar *data, int count)
{
	int n;
	uchar *wp;

	if(count == 0)
		return setupcmd(fd, req, 8);

	wp = emalloc(8+count);
	memmove(wp, req, 8);
	memmove(wp+8, data, count);
	n = setupcmd(fd, wp, 8+count);
	free(wp);
	return n;
}

static int
setupin(int fd, uchar *req, uchar *data, int count)
{
	int n;

	n = setupcmd(fd, req, 8);
	if(n < 0)
		return n;
	n = read(fd, data, count);
	if(n < 0)
		werrstr("setupin: read err: %r");
	else if(n != count)
		werrstr("setupin: short read (%d < %d)", n, count);
	return n;
}

int
setupreq(Device *d, int type, int request, int value, int index, uchar *data, int count)
{
	int fd;
	uchar req[8];

	fd = d->setup;
	req[0] = type;
	req[1] = request;
	PUT2(req+2, value);
	PUT2(req+4, index);
	PUT2(req+6, count);
	if((type&RD2H) != 0)
		return setupin(fd, req, data, count);
	else
		return setupout(fd, req, data, count);
}
