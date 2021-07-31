#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"

#include "dat.h"
#include "fns.h"

#define STACKSIZE 128*1024

static int dontfork;

Ref	busy;

int debug;

typedef struct Enum Enum;
struct Enum
{
	Hub *hub;
	int port;
};

void (*dprinter[])(Device *, int, ulong, void *b, int n) = {
	[STRING] pstring,
	[DEVICE] pdevice,
	[0x29] phub,
};

static void
usage(void)
{
	fprint(2, "usage: usbd [-DfV] [-d mask] [-u root-hub]\n");
	threadexitsall("usage");
}

void
work(void *a)
{
	int port;
	Hub *hub;
	Enum *arg;

	hub = a;
	for(port = 1; port <= hub->nport; port++){
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
	}
}

void
threadmain(int argc, char **argv)
{
	int i;
	Hub *h;
	int usbnum;

	usbnum = -1;
	ARGBEGIN{
	case 'd':
		debug = atoi(EARGF(usage()));
		break;
	case 'D':
		debugdebug++;
		break;
	case 'f':
		dontfork = 1;
		break;
	case 'u':
		usbnum = atoi(EARGF(usage()));
		break;
	case 'V':
		verbose = 1;
		break;
	}ARGEND
	if(argc)
		usage();
	if(access("/dev/usb0", 0) < 0 && bind("#U", "/dev", MBEFORE) < 0)
		sysfatal("%s: can't bind -b #U /dev: %r", argv0);

	usbfmtinit();
	fmtinstall('H', Hfmt);

	if(usbnum < 0){
		/* always fork off usb[1—n] */
		for(i=1; (h = roothub(i)) != nil; i++) {
			incref(&busy);
			proccreate(work, h, STACKSIZE);
		}
		usbnum = 0;
	}
	/* usb0 might be handled in this proc */
	if((h = roothub(usbnum)) != nil){
		incref(&busy);
		if (dontfork) {
			work(h);
		} else {
			rfork(RFNOTEG);
			proccreate(work, h, STACKSIZE);
			/* don't hold window open */
			close(0);
			close(1);
			if(!debug && !verbose)
				close(2);
		}
	}
	if (debug)
		fprint(2, "done\n");
	while (busy.ref)
		sleep(100);
	threadexits(nil);
}

void
enumerate(void *v)
{
	int i, port;
	Device *d;
	Enum *arg;
	Hub *h, *nh;

	arg = v;
	h = arg->hub;
	port = arg->port;
	free(arg);

	for(;;){
		if((portstatus(h, port) & (1<<PORT_CONNECTION)) == 0){
			decref(&busy);
			if(verbose)
				fprint(2, "usbd: %H: port %d empty\n", h, port);
			do{
				yield();
				if(debugdebug)
					fprint(2, "usbd: probing %H.%d\n",
						h, port);
				sleep(500);
			}while((portstatus(h, port) & (1<<PORT_CONNECTION)) == 0);
			incref(&busy);
		}
		if(verbose)
			fprint(2, "usbd: %H: port %d attached\n", h, port);
		d = configure(h, port);
		if(d == nil){
			if(verbose)
				fprint(2, "usbd: can't configure port %H.%d\n", h, port);
			decref(&busy);
			threadexits("configure");
		}
		if(d->class == Hubclass){
			if(debug)
				fprint(2, "usbd: %H.%d: hub %d attached\n",
					h, port, d->id);
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
		}else{
			if(debug)
				fprint(2,
					"usbd: %H.%d: %d: not hub, %s speed\n",
					h, port, d->id, d->ls?"low":"high");
			setconfig(d, 1);	/* TO DO */
//unconscionable kludge (testing camera)
if(d->class == 10) setup0(d, RH2D|Rinterface, SET_INTERFACE, 10, 0, 0);
		}
		decref(&busy);
		while(portstatus(h, port) & (1<<PORT_CONNECTION)) {
			if (debugdebug)
				fprint(2, "checking %H.%d\n", h, port);
			yield();
			if (d->state == Detached) {
				if (verbose)
					fprint(2,
					 "%H: port %d detached by parent hub\n",
						h, port);
				/* parent hub died */
				threadexits(nil);
			}
		}
		if(verbose)
			fprint(2, "%H: port %d detached\n", h, port);
		detach(h, port);
	}
}

Device*
configure(Hub *h, int port)
{
	Port *p;
	Device *d;
	int i, s, maxpkt, ls;

	portenable(h, port, 1);
	sleep(20);
	portreset(h, port);
	sleep(20);
	s = portstatus(h, port);
	if (debug)
		fprint(2, "%H.%d status %#ux\n", h, port, s);
	if ((s & (1<<PORT_CONNECTION)) == 0)
		return nil;
	if ((s & (1<<PORT_SUSPEND)) == 0) {
		if (debug)
			fprint(2, "enabling port %H.%d\n", h, port);
		portenable(h, port, 1);
		s = portstatus(h, port);
		if (debug)
			fprint(2, "%H.%d status now %#ux\n", h, port, s);
	}

	ls = (s & (1<<PORT_LOW_SPEED)) != 0;
	devspeed(h->dev0, ls);
	maxpkt = getmaxpkt(h->dev0);
	if(debugdebug)
		fprint(2, "%H.%d maxpkt: %d\n", h, port, maxpkt);
	if(maxpkt < 0){
Error0:
		portenable(h, port, 0);
		return nil;
	}
	d = opendev(h->ctlrno, -1);
	d->ls = ls;
	d->state = Enabled;
	d->ep[0]->maxpkt = maxpkt;
	if(fprint(d->ctl, "maxpkt 0 %d", maxpkt) < 0){
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

	/* read configurations 0 to n */
	for(i=0; i<d->nconf; i++){
		if(d->config[i] == nil)
			d->config[i] = mallocz(sizeof(*d->config[i]),1);
		loadconfig(d, i);
	}
	for(i=0; i<16; i++)
		setdevclass(d, i);
	p = &h->port[port-1];
	p->d = d;
	return d;
}

void
detach(Hub *h, int port)
{
	Port *p;
	Device *d;

	p = &h->port[port-1];
	if(p->hub != nil) {
		freehub(p->hub);
		p->hub = nil;
	}
	d = p->d;
	d->state = Detached;	/* return i/o error on access */
	closedev(d);
}
