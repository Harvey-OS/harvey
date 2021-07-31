#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usbproto.h"
#include "usb.h"

#include "dat.h"
#include "fns.h"

#define STACKSIZE 128*1024

Ref	busy;
int	debug, verbose;

typedef struct Enum Enum;
struct Enum
{
	Hub *hub;
	int port;
};

static void
usage(void)
{
	fprint(2, "usage: usbd [-v] [-d]\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char **argv)
{
	int i;
	Hub *h;

	ARGBEGIN {
	case 'd':
		debug++;
		break;
	case 'v':
		verbose = 1;
		break;
	} ARGEND

	if(argc)
		usage();

	rfork(RFNOTEG);
	if(access("/dev/usb0", 0) < 0 && bind("#U", "/dev", MAFTER) < 0)
		sysfatal("%s: can't bind #U after /dev: %r\n", argv0);

	usbfmtinit();
	fmtinstall('H', Hfmt);

	/* don't hold window open */
	close(0);
	close(1);
	if(!debug && !verbose)
		close(2);

	for(i = 0; (h = roothub(i)) != nil; i++) {
		incref(&busy);
		proccreate(roothubproc, h, STACKSIZE);
	}
	while(busy.ref)
		sleep(100);
	if(verbose)
		fprint(2, "usbd: enumeration complete\n");
	threadexits(nil);
}

void
roothubproc(void *a)
{
	int port;
	Hub *hub;
	Enum *arg;

	hub = a;
	for (port = 1; port <= hub->nport; port++) {
		if (debug)
			fprint(2, "enumerate port %H.%d\n", hub, port);
		arg = emallocz(sizeof(Enum), 1);
		arg->hub = hub;
		arg->port = port;
		incref(&busy);
		threadcreate(enumerate, arg, STACKSIZE);
	}

	decref(&busy);
	for(;;) {
		yield();
		if (busy.ref == 0)
			sleep(2000);
		else if(debug > 1)
			fprint(2, "usbd: busy.ref %ld\n", busy.ref);
	}
}

void
enumerate(void *v)
{
	Device *d;
	Enum *arg;
	Hub *h, *nh;
	int i, port, status;

	arg = v;
	h = arg->hub;
	port = arg->port;
	free(arg);

	for(;;) {
		status = portstatus(h, port);
		if(status < 0) {
badstatus:
			if(debug)
				fprint(2, "usbd: %H.%d: %r\n", h, port);
			threadexits(nil);
		}
		if((status & (1<<PORT_CONNECTION)) == 0) {
			decref(&busy);
			if(verbose)
				fprint(2, "usbd: %H: port %d empty\n", h, port);
			for(;;) {
				yield();
				if(debug > 1)
					fprint(2, "usbd: probing %H.%d\n", h, port);
				sleep(500);
				status = portstatus(h, port);
				if(status < 0)
					goto badstatus;
				if((status & (1<<PORT_CONNECTION)) != 0)
					break;
			}
			incref(&busy);
		}
		if(verbose)
			fprint(2, "usbd: %H: port %d attached\n", h, port);
		d = configure(h, port);
		if(d == nil) {
			if(verbose || debug)
				fprint(2, "usbd: can't configure port %H.%d: %r\n", h, port);
			decref(&busy);
			threadexits("configure");
		}
		if(Class(d->csp) == CL_HUB) {
			if(debug)
				fprint(2, "usbd: %H.%d: hub %d attached\n", h, port, d->id);
			setconfig(d, 1);
			nh = newhub(h, d);
			if(nh == nil) {
				detach(h, port);
				decref(&busy);
				threadexits("describehub");
			}
			if(debug)
				fprint(2, "usbd: traversing hub %H\n", nh);
			/* TO DO: initialise status endpoint */
			for(i=1; i<=nh->nport; i++)
				portpower(nh, i, 1);
			sleep(nh->pwrms);
			for(i=1; i<=nh->nport; i++) {
				arg = emallocz(sizeof(Enum), 1);
				arg->hub = nh;
				arg->port = i;
				incref(&busy);
				threadcreate(enumerate, arg, STACKSIZE);
			}
			h->port[port-1].hub = nh;
		}else{
			if(debug)
				fprint(2, "usbd: %H.%d: %d: not hub, %s speed\n", h, port, d->id, d->ls?"low":"high");
			setconfig(d, 1);	/* TO DO */
		}
		decref(&busy);
		for(;;) {
			status = portstatus(h, port);
			if(status < 0) {
				detach(h, port);
				goto badstatus;
			}
			if((status & (1<<PORT_CONNECTION)) == 0)
				break;
			if(debug > 1)
				fprint(2, "usbd: checking %H.%d\n", h, port);
			yield();
			if(d->state == Detached) {
				if(verbose)
					fprint(2, "usbd: %H: port %d detached by parent hub\n", h, port);
				/* parent hub died */
				threadexits(nil);
			}
		}
		incref(&busy);
		if(verbose)
			fprint(2, "usbd: %H: port %d detached\n", h, port);
		detach(h, port);
	}
	threadexits(nil);
}

Device*
configure(Hub *h, int port)
{
	Port *p;
	Device *d;
	int s, maxpkt, ls;

	if(portenable(h, port, 1) < 0)
		return nil;
	sleep(20);
	if(portreset(h, port) < 0)
		return nil;
	sleep(20);
	s = portstatus(h, port);
	if(s == -1)
		return nil;
	if(debug)
		fprint(2, "%H.%d status %#ux\n", h, port, s);
	if((s & (1<<PORT_CONNECTION)) == 0)
		return nil;
	if((s & (1<<PORT_SUSPEND)) == 0) {
		if (debug)
			fprint(2, "enabling port %H.%d\n", h, port);
		if(portenable(h, port, 1) < 0)
			return nil;
		s = portstatus(h, port);
		if(s == -1)
			return nil;
		if (debug)
			fprint(2, "%H.%d status now %#ux\n", h, port, s);
	}

	ls = (s & (1<<PORT_LOW_SPEED)) != 0;
	devspeed(h->dev0, ls);
	maxpkt = getmaxpkt(h->dev0);
	if(maxpkt < 0) {
Error0:
		portenable(h, port, 0);
		return nil;
	}
	d = opendev(h->ctlrno, -1);
	d->ls = ls;
	d->state = Enabled;
	if(fprint(d->ctl, "maxpkt 0 %d", maxpkt) < 0) {
Error1:
		closedev(d);
		goto Error0;
	}
	if(setaddress(h->dev0, d->id) < 0)
		goto Error1;
	d->state = Assigned;
	devspeed(d, ls);
	if(describedevice(d) < 0)
		goto Error1;
	if(verbose || debug) {
		loadstrings(d, 1033);
		dumpdevice(2, d);
	}

	p = &h->port[port-1];
	p->d = d;
	return d;
}

void
detach(Hub *h, int port)
{
	int i;
	Port *p;
	Hub *hh;
	Device *d;

	p = &h->port[port-1];
	hh = p->hub;
	if(hh != nil) {
		for(i = 1; i <= hh->nport; i++)
			detach(hh, i);
		freehub(hh);
		p->hub = nil;
	}
	d = p->d;
	if(d == nil || d->state == Detached)
		return;
	if(debug || verbose)
		fprint(2, "usbd: detaching %D from %H.%d\n", d, h, port);
	d->state = Detached;	/* return i/o error on access */
	closedev(d);
	p->d = nil;
}
