#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbproto.h"

static int	loadconfig(Device *d, int n);
static int	checkep(Dalt*, Endpt*);
static void	adddesc(Ddesc*, uchar*);
static void	dumplangids(Device*);

int
describedevice(Device *d)
{
	int i, nr;
	DDevice *dd;
	uchar buf[1023];

	nr = setupreq(d, RD2H|Rstandard|Rdevice, GET_DESCRIPTOR, (DEVICE<<8)|0, 0, buf, sizeof(buf));
	if(nr < 0) {
		werrstr("describedevice: %r");
		return -1;
	}
	if(nr < DDEVLEN) {
		werrstr("describedevice: short device descriptor (%d < %d)", nr, DDEVLEN);
		return -1;
	}
	dd = (DDevice*)buf;
	if(dd->bDescriptorType != DEVICE || dd->bLength < DDEVLEN) {
		werrstr("describedevice: bogus device descriptor");
		return -1;
	}
	d->vers = GET2(dd->bcdUSB);
	d->csp = CSP(dd->bDeviceClass, dd->bDeviceSubClass, dd->bDeviceProtocol);
	d->max0 = dd->bMaxPacketSize0;
	d->vid = GET2(dd->idVendor);
	d->did = GET2(dd->idProduct);
	d->release = GET2(dd->bcdDevice);
	d->manufacturer = dd->iManufacturer;
	d->product = dd->iProduct;
	d->serial = dd->iSerialNumber;
	d->nconf = dd->bNumConfigurations;

	/* read all configurations */
	if(d->config != nil)
		free(d->config);
	d->config = emallocz(d->nconf*sizeof(Dconf), 1);
	for(i = 0; i < d->nconf; i++)
		if(loadconfig(d, i) < 0)
			return -1;
	return 0;
}

static uchar type[] =
{
	[0]	Econtrol,
	[1]	Eiso,
	[2]	Ebulk,
	[3]	Eintr,
};

static uchar isotype[] =
{
	[0]	Eunknown,
	[1]	Easync,
	[2]	Eadapt,
	[3]	Esync,
};

static int
loadconfig(Device *d, int n)
{
	int i, nr;
	Dconf *c;
	Dinf *intf;
	Endpt *ep;
	DConfig *dc;
	Ddesc *desc;
	DInterface *di;
	Dalt *alt, **alttail;
	DEndpoint *dep;
	uchar buf[1023], *p, *ebuf;

	nr = setupreq(d, RD2H|Rstandard|Rdevice, GET_DESCRIPTOR, (CONFIGURATION<<8)|n, 0, buf, sizeof(buf));
	if(nr < 0) {
		werrstr("loadconfig: %r");
		return -1;
	}
	if(nr < DCONFLEN) {
		werrstr("loadconfig: short configuration descriptor (%d < %d)", nr, 2);
		return -1;
	}
	dc = (DConfig*)buf;
	if(dc->bDescriptorType != CONFIGURATION || dc->bLength < DCONFLEN) {
		werrstr("describedevice: bogus configuration descriptor");
		return -1;
	}
	if(GET2(dc->wTotalLength) != nr) {
		werrstr("describedevice: configuration descriptor length mismatch");
		return -1;
	}
	c = &d->config[n];
	c->d = d;
	c->x = n;
	c->nif = dc->bNumInterfaces;
	c->cval = dc->bConfigurationValue;
	c->config = dc->iConfiguration;
	c->attrib = dc->bmAttributes;
	c->milliamps = dc->MaxPower*2;		/* 2mA units */

	c->iface = emallocz(c->nif*sizeof(Dinf), 1);
	for(i = 0; i < c->nif; i++) {
		intf = &c->iface[i];
		intf->d = d;
		intf->conf = c;
		intf->x = i;
	}
	alttail = emallocz(c->nif*sizeof(Dalt*), 1);
	alt = nil;
	ep = nil;

	p = buf+dc->bLength;
	ebuf = buf+nr;
	while(p < ebuf) {
		if(p+p[0] > ebuf) {
			werrstr("truncated descriptor in loadconfig");
			goto error;
		}
		switch(p[1]) {
		case DEVICE:
		case CONFIGURATION:
			werrstr("illegal descriptor in loadconfig: %x", p[1]);
			goto error;
		case INTERFACE:
			if(checkep(alt, ep) < 0)
				goto error;
			di = (DInterface*)p;
			i = di->bInterfaceNumber;
			if(i >= c->nif) {
				werrstr("interface number out of bounds");
				goto error;
			}
			intf = &c->iface[i];
			alt = mallocz(sizeof(Dalt), 1);
			alt->intf = intf;
			alt->alt = di->bAlternateSetting;
			alt->npt = di->bNumEndpoints;
			alt->ep = emallocz(alt->npt*sizeof(Endpt), 1);
			alt->next = nil;
			if(alttail[i] == nil) {
				intf->alt = alt;
				intf->csp = CSP(di->bInterfaceClass, di->bInterfaceSubClass, di->bInterfaceProtocol);
				intf->interface = di->iInterface;
			}
			else
				alttail[i]->next = alt;
			alttail[i] = alt;
			ep = nil;
			break;
		case ENDPOINT:
			if(alt == nil) {
				werrstr("endpoint descriptor without interface");
				goto error;
			}
			if(ep == nil)
				ep = &alt->ep[0];
			else {
				ep++;
				if(ep - alt->ep > alt->npt) {
					werrstr("too many endpoints for interface");
					goto error;
				}
			}
			dep = (DEndpoint*)p;
			ep->addr = dep->bEndpointAddress & 0x0f;
			ep->dir = (dep->bEndpointAddress & 0x80) ? Ein : Eout;
			ep->type = type[dep->bmAttributes & 0x03];
			ep->isotype = isotype[(dep->bmAttributes>>2) & 0x03];
			ep->maxpkt = GET2(dep->wMaxPacketSize);
			ep->pollms = dep->bInterval;
			break;
		default:
			if(alt == nil)
				desc = &c->desc;
			else if(ep == nil)
				desc = &alt->desc;
			else
				desc = &ep->desc;
			adddesc(desc, p);
		}
		p += p[0];
	}
	free(alttail);
	return checkep(alt, ep);

error:
	free(alttail);
	return -1;
}

static int
checkep(Dalt *alt, Endpt *ep)
{
	int nep;

	if(alt == nil)
		return 0;
	if(ep == nil)
		nep = 0;
	else
		nep = ep-alt->ep+1;
	if(nep != alt->npt) {
		werrstr("incorrect number of endpoint descriptors %d != %d", nep, alt->npt);
		return -1;
	}
	return 0;
}

static void
adddesc(Ddesc *desc, uchar *p)
{
	int len;

	len = p[0];
	desc->data = realloc(desc->data, desc->bytes+len);
	memmove(desc->data+desc->bytes, p, len);
	desc->bytes += len;
	desc->ndesc++;
}

static void
dumplangids(Device *d)
{
	int nr, i;
	uchar buf[1023];

	nr = setupreq(d, RD2H|Rstandard|Rdevice, GET_DESCRIPTOR, (STRING<<8)|0, 0, buf, sizeof(buf)-2);
	if(nr < 0) {
		fprint(2, "dumplangids: %r\n");
		return;
	}
	if(nr < 2 || buf[0] != nr || buf[1] != STRING) {
		fprint(2, "dumplangids: bogus descriptor\n");
		return;
	}
	for(i = 1; i < nr/2; i++)
		fprint(2, "%d\n", GET2(&buf[i*2]));
}

void
loadstr(Device *d, int i, int langid)
{
	int nr;
	uchar buf[256+2];

	if(i == 0 || d->strings[i] != nil)
		return;
	nr = setupreq(d, RD2H|Rstandard|Rdevice, GET_DESCRIPTOR, (STRING<<8)|i, langid, buf, sizeof(buf)-2);
	if(nr < 0) {
		d->strings[i] = smprint("loadstr(%d): %r", i);
		return;
	}
	if(nr < 2 || buf[0] != nr || buf[1] != STRING) {
		d->strings[i] = smprint("loadstr(%d): bogus string", i);
		return;
	}
	buf[nr] = 0;
	buf[nr+1] = 0;
	d->strings[i] = smprint("%S", (ushort*)(buf+2));
}

void
loadstrings(Device *d, int langid)
{
	int i, j;
	Dconf *c;

	loadstr(d, d->manufacturer, langid);
	loadstr(d, d->product, langid);
	loadstr(d, d->serial, langid);

	for(i = 0; i < d->nconf; i++) {
		c = &d->config[i];
		loadstr(d, c->config, langid);
		for(j = 0; j < c->nif; j++)
			loadstr(d, c->iface[j].interface, langid);
	}
}
