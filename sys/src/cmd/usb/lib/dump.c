#include <u.h>
#include <libc.h>
#include <thread.h>
#include <bio.h>
#include "usb.h"

static char *type[] =
{
	[Econtrol]		"Control",
	[Eiso]		"Isochronous",
	[Ebulk]		"Bulk",
	[Eintr]		"Interrupt",
};

static char *isotype[] =
{
	[Eunknown]	"",
	[Easync]		" (Asynchronous)",
	[Eadapt]		" (Adaptive)",
	[Esync]		" (Synchronous)",
};

void
dumpstring(int fd, char *prefix, Device *d, int index)
{
	char *s;

	if(index == 0)
		return;
	s = d->strings[index];
	if(s == nil)
		fprint(fd, "%s<string %d>\n", prefix, index);
	else
		fprint(fd, "%s%s\n", prefix, s);
}

void
dumpdesc(int fd, char *prefix, Ddesc *desc)
{
	uchar *p;
	int i, j, len;

	if(desc->ndesc == 0)
		return;
	fprint(fd, "%s%d descriptors\n", prefix, desc->ndesc);
	p = desc->data;
	for(i = 0; i < desc->ndesc; i++) {
		fprint(fd, "%s\t", prefix);
		len = *p++;
		for(j = 1; j < len; j++)
			fprint(fd, "%.2ux ", *p++);
		fprint(fd, "\n");
	}
}

void
dumpdevice(int fd, Device *d)
{
	int i, j, k;
	Dconf *c;
	Dinf *intf;
	Dalt *alt;
	Endpt *ep;

	fprint(fd, "device %D: %lud.%lud.%lud %.4x/%.4x\n",
		d, Class(d->csp), Subclass(d->csp), Proto(d->csp), d->vid, d->did);
	fprint(fd, "\tUSB version: %x.%.2x\n", d->vers>>8, d->vers&0xff);
	fprint(fd, "\tmaximum control packet size: %d\n", d->max0);
	fprint(fd, "\tdevice release: %x.%.2x\n", d->release>>8, d->release&0xff);
	dumpstring(fd, "\tmanufacturer: ", d, d->manufacturer);
	dumpstring(fd, "\tproduct: ", d, d->product);
	dumpstring(fd, "\tserial: ", d, d->serial);
	fprint(fd, "\tconfigurations: %d\n", d->nconf);
	for(i = 0; i < d->nconf; i++) {
		c = &d->config[i];
		fprint(fd, "\t\tconfiguration %d: %d interfaces, %dmA required\n", c->cval, c->nif, c->milliamps);
		dumpstring(fd, "\t\tname: ", d, c->config);
		fprint(fd, "\t\tattributes:");
		if(c->attrib & Cbuspowered)
			fprint(fd, " buspowered");
		if(c->attrib & Cselfpowered)
			fprint(fd, " selfpowered");
		if(c->attrib & Cremotewakeup)
			fprint(fd, " remotewakeup");
		fprint(fd, "\n");
		dumpdesc(fd, "\t\t\t", &c->desc);
		for(j = 0; j < c->nif; j++) {
			intf = &c->iface[j];
			fprint(fd, "\t\tinterface %d: %lud.%lud.%lud\n", j, Class(intf->csp), Subclass(intf->csp), Proto(intf->csp));
			dumpstring(fd, "\t\tname: ", d, intf->interface);
			for(alt = intf->alt; alt != nil; alt = alt->next) {
				fprint(fd, "\t\t\talternate %d: %d endpoints\n", alt->alt, alt->npt);
				dumpdesc(fd, "\t\t\t\t", &alt->desc);
				for(k = 0; k < alt->npt; k++) {
					ep = &alt->ep[k];
					fprint(2, "\t\t\t\tendpoint %d-%s: type %s%s maxpkt %d poll %dms\n",
						ep->addr, ep->dir == Ein ? "in" : "out", type[ep->type], isotype[ep->isotype],
						ep->maxpkt, ep->pollms);
					dumpdesc(fd, "\t\t\t\t\t", &ep->desc);
				}
			}
		}
	}
}
