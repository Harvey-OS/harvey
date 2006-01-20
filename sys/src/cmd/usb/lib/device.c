#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"

static int
readnum(int fd)
{
	char buf[20];
	int n;

	for(;;){
		n = read(fd, buf, sizeof buf);
		if (n < 0){
			rerrstr(buf, sizeof buf);
			if (strcmp(buf, "interrupted") != 0)
				break;
		} else
			break;
	}
	buf[sizeof(buf)-1] = 0;
	return n <= 0? -1: strtol(buf, nil, 0);
}

Device*
opendev(int ctlrno, int id)
{
	int isnew;
	Device *d;
	char name[100], *p;

	d = emallocz(sizeof(Device), 1);
	incref(d);

	isnew = 0;
	if(id == -1) {
		sprint(name, "/dev/usb%d/new", ctlrno);
		if((d->ctl = open(name, ORDWR)) < 0){
		Error0:
			close(d->ctl);
			werrstr("open %s: %r", name);
			free(d);
			/* return nil; */
			sysfatal("%r");
		}
		id = readnum(d->ctl);
		isnew = 1;
	}
	sprint(name, "/dev/usb%d/%d/", ctlrno, id);
	p = name+strlen(name);

	if(!isnew) {
		strcpy(p, "ctl");
		if((d->ctl = open(name, ORDWR)) < 0)
			goto Error0;
	}

	strcpy(p, "setup");
	if((d->setup = open(name, ORDWR)) < 0){
	Error1:
		close(d->setup);
		goto Error0;
	}

	strcpy(p, "status");
	if((d->status = open(name, OREAD)) < 0)
		goto Error1;

	d->ctlrno = ctlrno;
	d->id = id;
	d->ep[0] = newendpt(d, 0, 0);
	return d;
}

void
closedev(Device *d)
{
	int i;

	if(d==nil)
		return;
	if(decref(d) != 0)
		return;
	close(d->ctl);
	close(d->setup);
	close(d->status);

	for(i=0; i<nelem(d->ep); i++)
		free(d->ep[i]);
	free(d);
}

void
setdevclass(Device *d, int n)
{
	Endpt *e;

	if (e = d->ep[n]) {
		if (verbose) fprint(2, "class %d %d %#6.6lux %#4.4ux %#4.4ux\n",
			d->nif, n, e->csp, d->vid, d->did);
		fprint(d->ctl, "class %d %d %#6.6lux %#4.4ux %#4.4ux",
			d->nif, n, e->csp, d->vid, d->did);
	}
}

int
describedevice(Device *d)
{
	DDevice *dd;
	byte buf[1023];
	int nr = -1;

	if (setupreq(d->ep[0], RD2H|Rstandard|Rdevice, GET_DESCRIPTOR, (DEVICE<<8)|0, 0, sizeof(buf)) < 0 ||
	   (nr = setupreply(d->ep[0], buf, sizeof(buf))) < DDEVLEN) {
		fprint(2, "usb: error reading device descriptor, got %d of %d\n",
			nr, DDEVLEN);
		return -1;
	}
	/* extract gubbins */
	pdesc(d, -1, -1, buf, nr);
	dd = (DDevice*)buf;
	d->csp = CSP(dd->bDeviceClass, dd->bDeviceSubClass, dd->bDeviceProtocol);
	d->ep[0]->maxpkt = dd->bMaxPacketSize0;
	if (dd->bDeviceClass == 9)
		d->class = Hubclass;
	else
		d->class = Otherclass;
	d->nconf = dd->bNumConfigurations;
	d->vid = GET2(dd->idVendor);
	d->did = GET2(dd->idProduct);
	return 0;
}

int
loadconfig(Device *d, int n)
{
	byte buf[1023];
	int nr, len;

	if (setupreq(d->ep[0], RD2H|Rstandard|Rdevice, GET_DESCRIPTOR, (CONFIGURATION<<8)|n, 0, sizeof(buf)) < 0 ||
	   (nr = setupreply(d->ep[0], buf, sizeof(buf))) < 1) {
		fprint(2, "usb: error reading configuration descriptor\n");
		return -1;
	}
	if (buf[1] == CONFIGURATION) {
		len = GET2(((DConfig*)buf)->wTotalLength);
		if (len < nr)
			nr = len;
	}
	/* extract gubbins */
	pdesc(d, n, -1, buf, nr);
	return 0;
}

Endpt *
newendpt(Device *d, int id, ulong csp)
{
	Endpt *e;

	e = mallocz(sizeof(*e), 1);
	e->id = id;
	e->dev = d;
	e->csp = csp;
	e->maxpkt = 32;
	return e;
}
