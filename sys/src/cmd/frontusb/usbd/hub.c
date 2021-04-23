#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "dat.h"
#include "fns.h"

Hub *hubs;
QLock hublock;
static int nhubs;
static int mustdump;
static int pollms = Pollms;

static char *dsname[] = { "disabled", "attached", "configed" };

static int
hubfeature(Hub *h, int port, int f, int on)
{
	int cmd;

	if(on)
		cmd = Rsetfeature;
	else
		cmd = Rclearfeature;
	return usbcmd(h->dev, Rh2d|Rclass|Rother, cmd, f, port, nil, 0);
}

static int
configusb2hub(Hub *h, DHub *dd, int nr)
{
	uchar *PortPwrCtrlMask;
	int i, offset, mask, nmap;
	Port *pp;

	h->nport = dd->bNbrPorts;
	nmap = 1 + h->nport/8;
	if(nr < 7 + 2*nmap){
		fprint(2, "%s: %s: descr. too small\n", argv0, h->dev->dir);
		return -1;
	}
	h->port = emallocz((h->nport+1)*sizeof(Port), 1);
	h->pwrms = dd->bPwrOn2PwrGood*2;
	if(h->pwrms < Powerdelay)
		h->pwrms = Powerdelay;
	h->maxcurrent = dd->bHubContrCurrent;
	h->pwrmode = dd->wHubCharacteristics[0] & 3;
	h->compound = (dd->wHubCharacteristics[0] & (1<<2))!=0;
	h->leds = (dd->wHubCharacteristics[0] & (1<<7)) != 0;
	PortPwrCtrlMask = dd->DeviceRemovable + nmap;
	for(i = 1; i <= h->nport; i++){
		pp = &h->port[i];
		offset = i/8;
		mask = 1<<(i%8);
		pp->removable = (dd->DeviceRemovable[offset] & mask) != 0;
		pp->pwrctl = (PortPwrCtrlMask[offset] & mask) != 0;
	}
	return 0;
}

static int
configusb3hub(Hub *h, DSSHub *dd, int nr)
{
	int i, offset, mask, nmap;
	Port *pp;

	h->nport = dd->bNbrPorts;
	nmap = 1 + h->nport/8;
	if(nr < 10 + nmap){
		fprint(2, "%s: %s: descr. too small\n", argv0, h->dev->dir);
		return -1;
	}
	h->port = emallocz((h->nport+1)*sizeof(Port), 1);
	h->pwrms = dd->bPwrOn2PwrGood*2;
	if(h->pwrms < Powerdelay)
		h->pwrms = Powerdelay;
	h->maxcurrent = dd->bHubContrCurrent;
	h->pwrmode = dd->wHubCharacteristics[0] & 3;
	h->compound = (dd->wHubCharacteristics[0] & (1<<2))!=0;
	h->leds = (dd->wHubCharacteristics[0] & (1<<7)) != 0;
	for(i = 1; i <= h->nport; i++){
		pp = &h->port[i];
		offset = i/8;
		mask = 1<<(i%8);
		pp->removable = (dd->DeviceRemovable[offset] & mask) != 0;
	}
	if(usbcmd(h->dev, Rh2d|Rclass|Rdev, Rsethubdepth, h->dev->depth, 0, nil, 0) < 0){
		fprint(2, "%s: %s: sethubdepth: %r\n", argv0, h->dev->dir);
		return -1;
	}
	return 0;
}

static int
confighub(Hub *h)
{
	uchar buf[128];	/* room for extra descriptors */
	int i, dt, dl, nr;
	Usbdev *d;
	void *dd;

	d = h->dev->usb;
	if(h->dev->isusb3){
		dt = Dsshub;
		dl = Dsshublen;
	} else {
		dt = Dhub;
		dl = Dhublen;
	}
	for(i = 0; i < nelem(d->ddesc); i++){
		if(d->ddesc[i] == nil)
			break;
		if(d->ddesc[i]->data.bDescriptorType == dt){
			dd = &d->ddesc[i]->data;
			nr = d->ddesc[i]->data.bLength;
			goto Config;
		}
	}
	nr = usbcmd(h->dev, Rd2h|Rclass|Rdev, Rgetdesc, dt<<8|0, 0, buf, sizeof buf);
	if(nr < 0){
		fprint(2, "%s: %s: getdesc hub: %r\n", argv0, h->dev->dir);
		return -1;
	}
	dd = buf;
Config:
	if(nr < dl){
		fprint(2, "%s: %s: hub descriptor too small (%d < %d)\n", argv0, h->dev->dir, nr, dl);
		return -1;
	}
	if(h->dev->isusb3)
		return configusb3hub(h, dd, nr);
	else
		return configusb2hub(h, dd, nr);
}

static void
configroothub(Hub *h)
{
	char buf[1024];
	char *p;
	int nr;
	Dev *d;

	d = h->dev;
	h->nport = 2;
	h->maxpkt = 8;
	seek(d->cfd, 0, 0);
	nr = read(d->cfd, buf, sizeof(buf)-1);
	if(nr < 0)
		goto Done;
	buf[nr] = 0;
	d->isusb3 = strstr(buf, "speed super") != nil;
	p = strstr(buf, "ports ");
	if(p == nil)
		fprint(2, "%s: %s: no port information\n", argv0, d->dir);
	else
		h->nport = atoi(p+6);
	p = strstr(buf, "maxpkt ");
	if(p == nil)
		fprint(2, "%s: %s: no maxpkt information\n", argv0, d->dir);
	else
		h->maxpkt = atoi(p+7);
Done:
	h->port = emallocz((h->nport+1)*sizeof(Port), 1);
	dprint(2, "%s: %s: ports %d maxpkt %d\n", argv0, d->dir, h->nport, h->maxpkt);
}

Hub*
newhub(char *fn, Dev *d)
{
	Hub *h;
	int i;
	Usbdev *ud;

	h = emallocz(sizeof(Hub), 1);
	h->isroot = (d == nil);
	if(h->isroot){
		h->dev = opendev(fn);
		if(h->dev == nil){
			fprint(2, "%s: opendev: %s: %r", argv0, fn);
			goto Fail;
		}
		h->dev->depth = -1;
		configroothub(h);	/* never fails */
		if(opendevdata(h->dev, ORDWR) < 0){
			fprint(2, "%s: opendevdata: %s: %r\n", argv0, fn);
			closedev(h->dev);
			goto Fail;
		}
	}else{
		h->dev = d;
		if(confighub(h) < 0){
			fprint(2, "%s: %s: config: %r\n", argv0, fn);
			goto Fail;
		}
	}
	devctl(h->dev, "hub");
	ud = h->dev->usb;
	if(h->isroot)
		devctl(h->dev, "info roothub csp %#08ux ports %d",
			0x000009, h->nport);
	else{
		devctl(h->dev, "info hub csp %#08ulx ports %d %q %q",
			ud->csp, h->nport, ud->vendor, ud->product);
		for(i = 1; i <= h->nport; i++)
			if(hubfeature(h, i, Fportpower, 1) < 0)
				fprint(2, "%s: %s: power: %r\n", argv0, fn);
		sleep(h->pwrms);
		for(i = 1; i <= h->nport; i++)
			if(h->leds != 0)
				hubfeature(h, i, Fportindicator, 1);
	}
	h->next = hubs;
	hubs = h;
	nhubs++;
	dprint(2, "%s: hub %#p allocated:", argv0, h);
	dprint(2, " ports %d pwrms %d max curr %d pwrm %d cmp %d leds %d\n",
		h->nport, h->pwrms, h->maxcurrent,
		h->pwrmode, h->compound, h->leds);
	incref(h->dev);
	return h;
Fail:
	if(d != nil)
		devctl(d, "detach");
	free(h->port);
	free(h);
	dprint(2, "%s: hub %#p failed to start:", argv0, h);
	return nil;
}

static void portdetach(Hub *h, int p);

/*
 * If during enumeration we get an I/O error the hub is gone or
 * in pretty bad shape. Because of retries of failed usb commands
 * (and the sleeps they include) it can take a while to detach all
 * ports for the hub. This detaches all ports and makes the hub void.
 * The parent hub will detect a detach (probably right now) and
 * close it later.
 */
static void
hubfail(Hub *h)
{
	int i;

	for(i = 1; i <= h->nport; i++)
		portdetach(h, i);
	h->failed = 1;
}

static void
closehub(Hub *h)
{
	Hub **hl;

	dprint(2, "%s: closing hub %#p\n", argv0, h);
	for(hl = &hubs; *hl != nil; hl = &(*hl)->next)
		if(*hl == h)
			break;
	if(*hl == nil)
		sysfatal("closehub: no hub");
	*hl = h->next;
	nhubs--;
	hubfail(h);		/* detach all ports */
	free(h->port);
	assert(h->dev != nil);
	devctl(h->dev, "detach");
	closedev(h->dev);
	free(h);
}

static u32int
portstatus(Hub *h, int p)
{
	Dev *d;
	uchar buf[4];
	u32int sts;
	int t;
	int dbg;

	dbg = usbdebug;
	if(dbg != 0 && dbg < 4)
		usbdebug = 1;	/* do not be too chatty */
	d = h->dev;
	t = Rd2h|Rclass|Rother;
	if(usbcmd(d, t, Rgetstatus, 0, p, buf, sizeof(buf)) < 0)
		sts = -1;
	else
		sts = GET4(buf);
	usbdebug = dbg;
	return sts;
}

static char*
stsstr(int sts, int isusb3)
{
	static char s[80];
	char *e;

	e = s;
	if(sts&PSpresent)
		*e++ = 'p';
	if(sts&PSenable)
		*e++ = 'e';
	if(sts&PSovercurrent)
		*e++ = 'o';
	if(sts&PSreset)
		*e++ = 'r';
	if(!isusb3){
		if(sts&PSslow)
			*e++ = 'l';
		if(sts&PShigh)
			*e++ = 'h';
		if(sts&PSchange)
			*e++ = 'c';
		if(sts&PSstatuschg)
			*e++ = 's';
		if(sts&PSsuspend)
			*e++ = 'z';
	}
	if(e == s)
		*e++ = '-';
	*e = 0;
	return s;
}

static int
getmaxpkt(Dev *d, int islow)
{
	uchar buf[64];	/* More room to try to get device-specific descriptors */
	DDev *dd;

	if(d->isusb3)
		return 512;
	dd = (DDev*)buf;
	if(islow)
		dd->bMaxPacketSize0 = 8;
	else
		dd->bMaxPacketSize0 = 64;
	if(usbcmd(d, Rd2h|Rstd|Rdev, Rgetdesc, Ddev<<8|0, 0, buf, sizeof(buf)) < 0)
		return -1;
	return dd->bMaxPacketSize0;
}

/*
 * BUG: does not consider max. power avail.
 */
static Dev*
portattach(Hub *h, int p, u32int sts)
{
	Dev *d;
	Port *pp;
	Dev *nd;
	char fname[80];
	char buf[40];
	char *sp;
	int mp;
	int nr;

	d = h->dev;
	pp = &h->port[p];
	nd = nil;
	pp->state = Pattached;
	dprint(2, "%s: %s: port %d attach sts %#ux\n", argv0, d->dir, p, sts);
	if(h->dev->isusb3){
		sleep(Enabledelay);
		sts = portstatus(h, p);
		if(sts == -1)
			goto Fail;
		if((sts & PSenable) == 0){
			dprint(2, "%s: %s: port %d: not enabled?\n", argv0, d->dir, p);
			goto Fail;
		}
		sp = "super";
	} else {
		sleep(Enabledelay);
		if(hubfeature(h, p, Fportreset, 1) < 0){
			dprint(2, "%s: %s: port %d: reset: %r\n", argv0, d->dir, p);
			goto Fail;
		}
		sleep(Resetdelay);
		sts = portstatus(h, p);
		if(sts == -1)
			goto Fail;
		if((sts & PSenable) == 0){
			dprint(2, "%s: %s: port %d: not enabled?\n", argv0, d->dir, p);
			hubfeature(h, p, Fportenable, 1);
			sts = portstatus(h, p);
			if((sts & PSenable) == 0)
				goto Fail;
		}
		sp = "full";
		if(sts & PSslow)
			sp = "low";
		if(sts & PShigh)
			sp = "high";
	}
	dprint(2, "%s: %s: port %d: attached status %#ux, speed %s\n", argv0, d->dir, p, sts, sp);

	if(devctl(d, "newdev %s %d", sp, p) < 0){
		fprint(2, "%s: %s: port %d: newdev: %r\n", argv0, d->dir, p);
		goto Fail;
	}
	seek(d->cfd, 0, 0);
	nr = read(d->cfd, buf, sizeof(buf)-1);
	if(nr == 0){
		fprint(2, "%s: %s: port %d: newdev: eof\n", argv0, d->dir, p);
		goto Fail;
	}
	if(nr < 0){
		fprint(2, "%s: %s: port %d: newdev: %r\n", argv0, d->dir, p);
		goto Fail;
	}
	buf[nr] = 0;
	snprint(fname, sizeof(fname), "/dev/usb/%s", buf);
	nd = opendev(fname);
	if(nd == nil){
		fprint(2, "%s: %s: port %d: opendev: %r\n", argv0, d->dir, p);
		goto Fail;
	}
	nd->depth = h->dev->depth+1;
	nd->isusb3 = h->dev->isusb3;
	if(usbdebug > 2)
		devctl(nd, "debug 1");
	if(opendevdata(nd, ORDWR) < 0){
		fprint(2, "%s: %s: opendevdata: %r\n", argv0, nd->dir);
		goto Fail;
	}
	if(usbcmd(nd, Rh2d|Rstd|Rdev, Rsetaddress, nd->id, 0, nil, 0) < 0){
		dprint(2, "%s: %s: port %d: setaddress: %r\n", argv0, d->dir, p);
		goto Fail;
	}
	if(devctl(nd, "address") < 0){
		dprint(2, "%s: %s: port %d: set address: %r\n", argv0, d->dir, p);
		goto Fail;
	}
	mp=getmaxpkt(nd, strcmp(sp, "low") == 0);
	if(mp < 0){
		dprint(2, "%s: %s: port %d: getmaxpkt: %r\n", argv0, d->dir, p);
		goto Fail;
	}else{
		dprint(2, "%s; %s: port %d: maxpkt %d\n", argv0, d->dir, p, mp);
		devctl(nd, "maxpkt %d", mp);
	}
	if(configdev(nd) < 0){
		dprint(2, "%s: %s: port %d: configdev: %r\n", argv0, d->dir, p);
		goto Fail;
	}
	/*
	 * We always set conf #1. BUG.
	 */
	if(usbcmd(nd, Rh2d|Rstd|Rdev, Rsetconf, 1, 0, nil, 0) < 0){
		dprint(2, "%s: %s: port %d: setconf: %r\n", argv0, d->dir, p);
		unstall(nd, nd, Eout);
		if(usbcmd(nd, Rh2d|Rstd|Rdev, Rsetconf, 1, 0, nil, 0) < 0)
			goto Fail;
	}
	pp->state = Pconfiged;
	dprint(2, "%s: %s: port %d: configured: %s\n", argv0, d->dir, p, nd->dir);
	return pp->dev = nd;
Fail:
	pp->state = Pdisabled;
	pp->sts = 0;
	if(pp->hub != nil)
		pp->hub = nil;	/* hub closed by enumhub */
	if(!h->dev->isusb3)
		hubfeature(h, p, Fportenable, 0);
	if(nd != nil)
		devctl(nd, "detach");
	closedev(nd);
	return nil;
}

static void
portdetach(Hub *h, int p)
{
	Dev *d;
	Port *pp;
	d = h->dev;
	pp = &h->port[p];

	/*
	 * Clear present, so that we detect an attach on reconnects.
	 */
	pp->sts &= ~(PSpresent|PSenable);

	if(pp->state == Pdisabled)
		return;
	pp->state = Pdisabled;
	dprint(2, "%s: %s: port %d: detached\n", argv0, d->dir, p);

	if(pp->hub != nil){
		closehub(pp->hub);
		pp->hub = nil;
	}
	if(pp->dev != nil){
		detachdev(pp);

		devctl(pp->dev, "detach");
		closedev(pp->dev);
		pp->dev = nil;
	}
}

/*
 * The next two functions are included to
 * perform a port reset asked for by someone (usually a driver).
 * This must be done while no other device is in using the
 * configuration address and with care to keep the old address.
 * To keep drivers decoupled from usbd they write the reset request
 * to the #u/usb/epN.0/ctl file and then exit.
 * This is unfortunate because usbd must now poll twice as much.
 *
 * An alternative to this reset process would be for the driver to detach
 * the device. The next function could see that, issue a port reset, and
 * then restart the driver once to see if it's a temporary error.
 *
 * The real fix would be to use interrupt endpoints for non-root hubs
 * (would probably make some hubs fail) and add an events file to
 * the kernel to report events to usbd. This is a severe change not
 * yet implemented.
 */
static int
portresetwanted(Hub *h, int p)
{
	char buf[5];
	Port *pp;
	Dev *nd;

	pp = &h->port[p];
	nd = pp->dev;
	if(nd != nil && nd->cfd >= 0 && pread(nd->cfd, buf, 5, 0LL) == 5)
		return strncmp(buf, "reset", 5) == 0;
	else
		return 0;
}

static void
portreset(Hub *h, int p)
{
	u32int sts;
	Dev *d, *nd;
	Port *pp;

	d = h->dev;
	pp = &h->port[p];
	dprint(2, "%s: %s: port %d: resetting\n", argv0, d->dir, p);
	if(hubfeature(h, p, Fportreset, 1) < 0){
		dprint(2, "%s: %s: port %d: reset: %r\n", argv0, d->dir, p);
		goto Fail;
	}
	sleep(Resetdelay);
	sts = portstatus(h, p);
	if(sts == -1)
		goto Fail;
	if((sts & PSenable) == 0){
		dprint(2, "%s: %s: port %d: not enabled?\n", argv0, d->dir, p);
		goto Fail;
	}
	if((nd = pp->dev) == nil)
		return;
	opendevdata(nd, ORDWR);
	if(usbcmd(nd, Rh2d|Rstd|Rdev, Rsetaddress, nd->id, 0, nil, 0) < 0){
		dprint(2, "%s: %s: port %d: setaddress: %r\n", argv0, d->dir, p);
		goto Fail;
	}
	if(devctl(nd, "address") < 0){
		dprint(2, "%s: %s: port %d: set address: %r\n", argv0, d->dir, p);
		goto Fail;
	}
	if(usbcmd(nd, Rh2d|Rstd|Rdev, Rsetconf, 1, 0, nil, 0) < 0){
		dprint(2, "%s: %s: port %d: setconf: %r\n", argv0, d->dir, p);
		unstall(nd, nd, Eout);
		if(usbcmd(nd, Rh2d|Rstd|Rdev, Rsetconf, 1, 0, nil, 0) < 0)
			goto Fail;
	}
	if(nd->dfd >= 0){
		close(nd->dfd);
		nd->dfd = -1;
	}
	return;
Fail:
	pp->sts = 0;
	portdetach(h, p);
	if(!d->isusb3)
		hubfeature(h, p, Fportenable, 0);
}

static int
portgone(Port *pp, u32int sts)
{
	if(sts == -1)
		return 1;
	/*
	 * If it was enabled and it's not now then it may be reconnect.
	 * We pretend it's gone and later we'll see it as attached.
	 */
	if((pp->sts & PSenable) != 0 && (sts & PSenable) == 0)
		return 1;
	return (pp->sts & PSpresent) != 0 && (sts & PSpresent) == 0;
}

static int
enumhub(Hub *h, int p)
{
	u32int sts;
	Dev *d;
	Port *pp;
	int onhubs;

	if(h->failed)
		return 0;
	d = h->dev;
	if(usbdebug > 3)
		fprint(2, "%s: %s: port %d enumhub\n", argv0, d->dir, p);

	sts = portstatus(h, p);
	if(sts == -1){
		hubfail(h);		/* avoid delays on detachment */
		return -1;
	}
	pp = &h->port[p];
	onhubs = nhubs;
	if(!h->dev->isusb3){
		if((sts & PSsuspend) != 0){
			if(hubfeature(h, p, Fportsuspend, 0) < 0)
				dprint(2, "%s: %s: port %d: unsuspend: %r\n", argv0, d->dir, p);
			sleep(Enabledelay);
			sts = portstatus(h, p);
			fprint(2, "%s: %s: port %d: unsuspended (sts %#ux)\n", argv0, d->dir, p, sts);
		}
	}
	if((pp->sts & PSpresent) == 0 && (sts & PSpresent) != 0){
		if(portattach(h, p, sts) != nil)
			if(attachdev(pp) < 0)
				portdetach(h, p);
	}else if(portgone(pp, sts)){
		portdetach(h, p);
	}else if(portresetwanted(h, p))
		portreset(h, p);
	else if(pp->sts != sts){
		dprint(2, "%s: %s port %d: sts %s %#ux ->",
			argv0, d->dir, p, stsstr(pp->sts, h->dev->isusb3), pp->sts);
		dprint(2, " %s %#ux\n",stsstr(sts, h->dev->isusb3), sts);
	}
	pp->sts = sts;
	if(onhubs != nhubs)
		return -1;
	return 0;
}

static void
dump(void)
{
	Hub *h;
	int i;

	mustdump = 0;
	for(h = hubs; h != nil; h = h->next)
		for(i = 1; i <= h->nport; i++)
			fprint(2, "%s: hub %#p %s port %d: %U\n",
				argv0, h, h->dev->dir, i, h->port[i].dev);

}

void
work(void)
{
	Hub *h;
	int i;

	/*
	 * Enumerate (and acknowledge after first enumeration).
	 * Do NOT perform enumeration concurrently for the same
	 * controller. new devices attached respond to a default
	 * address (0) after reset, thus enumeration has to work
	 * one device at a time at least before addresses have been
	 * assigned.
	 * Do not use hub interrupt endpoint because we
	 * have to poll the root hub(s) in any case.
	 */
	for(;;){
		qlock(&hublock);
Again:
		for(h = hubs; h != nil; h = h->next)
			for(i = 1; i <= h->nport; i++)
				if(enumhub(h, i) < 0){
					/* changes in hub list; repeat */
					goto Again;
				}
		qunlock(&hublock);
		checkidle();
		sleep(pollms);
		if(mustdump)
			dump();
	}
}
