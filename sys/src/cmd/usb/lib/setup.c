#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"

int
setupcmd(Endpt *e, int type, int req, int value, int index, byte *data, int count)
{
	byte *wp;
	int n, i, fd;

	if (e == nil)
		abort();
	fd = e->dev->setup;
	if(fd < 0)
		sysfatal("RSC: this used to use the global usbsetup0\n");
	wp = malloc(8+count);
	if (wp == nil) sysfatal("setupcmd: malloc");
	wp[0] = type;
	wp[1] = req;
	PUT2(wp+2, value);
	PUT2(wp+4, index);
	PUT2(wp+6, count);
	memmove(wp+8, data, count);
	if (debugdebug) {
		fprint(2, "out\t%d\t[%d]", fd, 8+count);
		for(i=0; i<8+count; i++)
			fprint(2, " %.2ux", wp[i]);
		fprint(2, "\n");
	}
	n = write(fd, wp, 8+count);
	if (n < 0) {
		fprint(2, "setupreq: write err: %r\n");
		return -1;
	}
	if (n != 8+count) {
		fprint(2, "setupcmd: short write: %d\n", n);
		return -1;
	}
	return n;
}

int
setupreq(Endpt *e, int type, int req, int value, int index, int count)
{
	byte *wp, buf[8];
	int n, i, fd;

	if (e == nil)
		abort();
	fd = e->dev->setup;
	if(fd < 0)
		sysfatal("RSC: this used to use the global usbsetup0\n");
	wp = buf;
	wp[0] = type;
	wp[1] = req;
	PUT2(wp+2, value);
	PUT2(wp+4, index);
	PUT2(wp+6, count);
	if (debugdebug) {
		fprint(2, "out\t%d\t[8]", fd);
		for(i=0; i<8; i++)
			fprint(2, " %.2ux", buf[i]);
		fprint(2, "\n");
	}
	n = write(fd, buf, 8);
	if (n < 0) {
		fprint(2, "setupreq: write err: %r\n");
		return -1;
	}
	if (n != 8) {
		fprint(2, "setupreq: short write: %d\n", n);
		return -1;
	}
	return n;
}

int
setupreply(Endpt *e, void *buf, int nb)
{
	uchar *p;
	int i, fd;
	char err[32];

	fd = e->dev->setup;
	if(fd < 0)
		sysfatal("RSC: this used to use the global usbsetup0\n");
	for(;;){
		nb = read(fd, buf, nb);
		if (nb >= 0)
			break;
		rerrstr(err, sizeof err);
		if (strcmp(err, "interrupted") != 0)
			break;
	}
	p = buf;
	if (debugdebug) {
		fprint(2, "in\t%d\t[%d]", fd, nb);
		for(i=0; i<nb; i++)
			fprint(2, " %.2ux", p[i]);
		fprint(2, "\n");
	}
	return nb;
}
