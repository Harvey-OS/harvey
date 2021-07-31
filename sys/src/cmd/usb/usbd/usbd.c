#include <u.h>
#include <libc.h>
#include <thread.h>
#include <fcall.h>
#include "usb.h"
#include "usbfs.h"
#include "usbd.h"

static Channel *portc;
static int win;
static int verbose;

int mainstacksize = Stack;
static Hub *hubs;
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

/*
 * This may be used to detect overcurrent on the hub
 */
static void
checkhubstatus(Hub *h)
{
	uchar buf[4];
	int sts;

	if(h->isroot)	/* not for root hubs */
		return;
	if(usbcmd(h->dev, Rd2h|Rclass|Rdev, Rgetstatus, 0, 0, buf, 4) < 0){
		dprint(2, "%s: get hub status: %r\n", h->dev->dir);
		return;
	}
	sts = GET2(buf);
	dprint(2, "hub %s: status %#ux\n", h->dev->dir, sts);
}

static int
confighub(Hub *h)
{
	int type;
	uchar buf[128];	/* room for extra descriptors */
	int i;
	Usbdev *d;
	DHub *dd;
	Port *pp;
	int nr;
	int nmap;
	uchar *PortPwrCtrlMask;
	int offset;
	int mask;

	d = h->dev->usb;
	for(i = 0; i < nelem(d->ddesc); i++)
		if(d->ddesc[i] == nil)
			break;
		else if(d->ddesc[i]->data.bDescriptorType == Dhub){
			dd = (DHub*)&d->ddesc[i]->data;
			nr = Dhublen;
			goto Config;
		}
	type = Rd2h|Rclass|Rdev;
	nr = usbcmd(h->dev, type, Rgetdesc, Dhub<<8|0, 0, buf, sizeof buf);
	if(nr < Dhublen){
		dprint(2, "%s: %s: getdesc hub: %r\n", argv0, h->dev->dir);
		return -1;
	}
	dd = (DHub*)buf;
Config:
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

static void
configroothub(Hub *h)
{
	Dev *d;
	char buf[128];
	char *p;
	int nr;

	d = h->dev;
	h->nport = 2;
	h->maxpkt = 8;
	seek(d->cfd, 0, 0);
	nr = read(d->cfd, buf, sizeof(buf)-1);
	if(nr < 0)
		goto Done;
	buf[nr] = 0;

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
		if(opendevdata(h->dev, ORDWR) < 0){
			fprint(2, "%s: opendevdata: %s: %r\n", argv0, fn);
			goto Fail;
		}
		configroothub(h);	/* never fails */
	}else{
		h->dev = d;
		if(confighub(h) < 0){
			fprint(2, "%s: %s: config: %r\n", argv0, fn);
			goto Fail;
		}
	}
	if(h->dev == nil){
		fprint(2, "%s: opendev: %s: %r\n", argv0, fn);
		goto Fail;
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

static int
portstatus(Hub *h, int p)
{
	Dev *d;
	uchar buf[4];
	int t;
	int sts;
	int dbg;

	dbg = usbdebug;
	if(dbg != 0 && dbg < 4)
		usbdebug = 1;	/* do not be too chatty */
	d = h->dev;
	t = Rd2h|Rclass|Rother;
	if(usbcmd(d, t, Rgetstatus, 0, p, buf, sizeof(buf)) < 0)
		sts = -1;
	else
		sts = GET2(buf);
	usbdebug = dbg;
	return sts;
}

static char*
stsstr(int sts)
{
	static char s[80];
	char *e;

	e = s;
	if(sts&PSsuspend)
		*e++ = 'z';
	if(sts&PSreset)
		*e++ = 'r';
	if(sts&PSslow)
		*e++ = 'l';
	if(sts&PShigh)
		*e++ = 'h';
	if(sts&PSchange)
		*e++ = 'c';
	if(sts&PSenable)
		*e++ = 'e';
	if(sts&PSstatuschg)
		*e++ = 's';
	if(sts&PSpresent)
		*e++ = 'p';
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
portattach(Hub *h, int p, int sts)
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
	sleep(Connectdelay);
	if(hubfeature(h, p, Fportenable, 1) < 0)
		dprint(2, "%s: %s: port %d: enable: %r\n", argv0, d->dir, p);
	sleep(Enabledelay);
	if(hubfeature(h, p, Fportreset, 1) < 0){
		dprint(2, "%s: %s: port %d: reset: %r\n", argv0, d->dir, p);
		goto Fail;
	}
	sleep(Resetdelay);
	sts = portstatus(h, p);
	if(sts < 0)
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
	dprint(2, "%s: %s: port %d: attached status %#ux\n", argv0, d->dir, p, sts);

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
	if((sts & PSslow) != 0 && strcmp(sp, "full") == 0)
		dprint(2, "%s: %s: port %d: %s is full speed when port is low\n",
			argv0, d->dir, p, nd->dir);
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
	dprint(2, "%s: %U", argv0, nd);
	pp->state = Pconfiged;
	dprint(2, "%s: %s: port %d: configed: %s\n",
			argv0, d->dir, p, nd->dir);
	return pp->dev = nd;
Fail:
	pp->state = Pdisabled;
	pp->sts = 0;
	if(pp->hub != nil)
		pp->hub = nil;	/* hub closed by enumhub */
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
	extern void usbfsgone(char*);
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
	if(pp->devmaskp != nil)
		putdevnb(pp->devmaskp, pp->devnb);
	pp->devmaskp = nil;
	if(pp->dev != nil){
		devctl(pp->dev, "detach");
		usbfsgone(pp->dev->dir);
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
	int sts;
	Dev *d, *nd;
	Port *pp;

	d = h->dev;
	pp = &h->port[p];
	nd = pp->dev;
	dprint(2, "%s: %s: port %d: resetting\n", argv0, d->dir, p);
	if(hubfeature(h, p, Fportreset, 1) < 0){
		dprint(2, "%s: %s: port %d: reset: %r\n", argv0, d->dir, p);
		goto Fail;
	}
	sleep(Resetdelay);
	sts = portstatus(h, p);
	if(sts < 0)
		goto Fail;
	if((sts & PSenable) == 0){
		dprint(2, "%s: %s: port %d: not enabled?\n", argv0, d->dir, p);
		hubfeature(h, p, Fportenable, 1);
		sts = portstatus(h, p);
		if((sts & PSenable) == 0)
			goto Fail;
	}
	nd = pp->dev;
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
	if(nd->dfd >= 0)
		close(nd->dfd);
	return;
Fail:
	pp->state = Pdisabled;
	pp->sts = 0;
	if(pp->hub != nil)
		pp->hub = nil;	/* hub closed by enumhub */
	hubfeature(h, p, Fportenable, 0);
	if(nd != nil)
		devctl(nd, "detach");
	closedev(nd);
}

static int
portgone(Port *pp, int sts)
{
	if(sts < 0)
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
	int sts;
	Dev *d;
	Port *pp;
	int onhubs;

	if(h->failed)
		return 0;
	d = h->dev;
	if(usbdebug > 3)
		fprint(2, "%s: %s: port %d enumhub\n", argv0, d->dir, p);

	sts = portstatus(h, p);
	if(sts < 0){
		hubfail(h);		/* avoid delays on detachment */
		return -1;
	}
	pp = &h->port[p];
	onhubs = nhubs;
	if((sts & PSsuspend) != 0){
		if(hubfeature(h, p, Fportenable, 1) < 0)
			dprint(2, "%s: %s: port %d: enable: %r\n", argv0, d->dir, p);
		sleep(Enabledelay);
		sts = portstatus(h, p);
		fprint(2, "%s: %s: port %d: resumed (sts %#ux)\n", argv0, d->dir, p, sts);
	}
	if((pp->sts & PSpresent) == 0 && (sts & PSpresent) != 0){
		if(portattach(h, p, sts) != nil)
			if(startdev(pp) < 0)
				portdetach(h, p);
	}else if(portgone(pp, sts))
		portdetach(h, p);
	else if(portresetwanted(h, p))
		portreset(h, p);
	else if(pp->sts != sts){
		dprint(2, "%s: %s port %d: sts %s %#x ->",
			argv0, d->dir, p, stsstr(pp->sts), pp->sts);
		dprint(2, " %s %#x\n",stsstr(sts), sts);
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
			fprint(2, "%s: hub %#p %s port %d: %U",
				argv0, h, h->dev->dir, i, h->port[i].dev);
	usbfsdirdump();

}

static void
work(void *a)
{
	Channel *portc;
	char *fn;
	Hub *h;
	int i;

	portc = a;
	hubs = nil;
	/*
	 * Receive requests for root hubs
	 */
	while((fn = recvp(portc)) != nil){
		dprint(2, "%s: %s starting\n", argv0, fn);
		h = newhub(fn, nil);
		if(h == nil)
			fprint(2, "%s: %s: newhub failed: %r\n", argv0, fn);
		free(fn);
	}
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
Again:
		for(h = hubs; h != nil; h = h->next)
			for(i = 1; i <= h->nport; i++)
				if(enumhub(h, i) < 0){
					/* changes in hub list; repeat */
					goto Again;
				}
		if(portc != nil){
			sendp(portc, nil);
			portc = nil;
		}
		sleep(pollms);
		if(mustdump)
			dump();
	}
}

static int
cfswalk(Usbfs*, Fid *, char *)
{
	werrstr(Enotfound);
	return -1;
}

static int
cfsopen(Usbfs*, Fid *, int)
{
	return 0;
}

static long
cfsread(Usbfs*, Fid *, void *, long , vlong )
{
	return 0;
}

static void
setdrvargs(char *name, char *args)
{
	Devtab *dt;
	extern Devtab devtab[];

	for(dt = devtab; dt->name != nil; dt++)
		if(strstr(dt->name, name) != nil)
			dt->args = estrdup(args);
}

static long
cfswrite(Usbfs*, Fid *, void *data, long cnt, vlong )
{
	char buf[80];
	char *toks[4];

	if(cnt > sizeof(buf))
		cnt = sizeof(buf) - 1;
	strncpy(buf, data, cnt);
	buf[cnt] = 0;
	if(cnt > 0 && buf[cnt-1] == '\n')
		buf[cnt-1] = 0;
	if(strncmp(buf, "dump", 4) == 0){
		mustdump = 1;
		return cnt;
	}
	if(strncmp(buf, "reset", 5) == 0){
		werrstr("reset not implemented");
		return -1;
	}
	if(tokenize(buf, toks, nelem(toks)) != 2){
		werrstr("usage: debug|fsdebug n");
		return -1;
	}
	if(strcmp(toks[0], "debug") == 0)
		usbdebug = atoi(toks[1]);
	else if(strcmp(toks[0], "fsdebug") == 0)
		usbfsdebug = atoi(toks[1]);
	else if(strcmp(toks[0], "kbargs") == 0)
		setdrvargs("kb", toks[1]);
	else if(strcmp(toks[0], "diskargs") == 0)
		setdrvargs("disk", toks[1]);
	else{
		werrstr("unkown ctl '%s'", buf);
		return -1;
	}
	fprint(2, "%s: debug %d fsdebug %d\n", argv0, usbdebug, usbfsdebug);
	return cnt;
}

static int
cfsstat(Usbfs* fs, Qid qid, Dir *d)
{
	d->qid = qid;
	d->qid.path |= fs->qid;
	d->qid.type = 0;
	d->qid.vers = 0;
	d->name = "usbdctl";
	d->length = 0;
	d->mode = 0664;
	return 0;
}

static Usbfs ctlfs =
{
	.walk = cfswalk,
	.open = cfsopen,
	.read = cfsread,
	.write = cfswrite,
	.stat = cfsstat
};

static void
args(void)
{
	char *s;

	s = getenv("usbdebug");
	if(s != nil)
		usbdebug = atoi(s);
	free(s);
	s = getenv("usbfsdebug");
	if(s != nil)
		usbfsdebug = atoi(s);
	free(s);
	s = getenv("kbargs");
	if(s != nil)
		setdrvargs("kb", s);
	free(s);
	s = getenv("diskargs");
	if(s != nil)
		setdrvargs("disk", s);
	free(s);
}

static void
usage(void)
{
	fprint(2, "usage: %s [-Dd] [-s srv] [-m mnt] [dev...]\n", argv0);
	threadexitsall("usage");
}

extern void usbfsexits(int);

void
threadmain(int argc, char **argv)
{
	int i;
	Dir *d;
	int fd;
	int nd;
	char *err;
	char *srv;
	char *mnt;

	srv = "usb";
	mnt = "/dev";
	ARGBEGIN{
	case 'D':
		usbfsdebug++;
		break;
	case 'd':
		usbdebug++;
		break;
	case 's':
		srv = EARGF(usage());
		break;
	case 'i':
		pollms = atoi(EARGF(usage()));
		break;
	case 'm':
		mnt = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;
	if(access("/dev/usb", AEXIST) < 0 && bind("#u", "/dev", MBEFORE) < 0)
		sysfatal("#u: %r");

	args();

	fmtinstall('U', Ufmt);
	quotefmtinstall();
	rfork(RFNOTEG);
	portc = chancreate(sizeof(char *), 0);
	if(portc == nil)
		sysfatal("chancreate");
	proccreate(work, portc, Stack);
	if(argc == 0){
		fd = open("/dev/usb", OREAD);
		if(fd < 0)
			sysfatal("/dev/usb: %r");
		nd = dirreadall(fd, &d);
		close(fd);
		if(nd < 2)
			sysfatal("/dev/usb: no hubs");
		for(i = 0; i < nd; i++)
			if(strcmp(d[i].name, "ctl") != 0)
				sendp(portc, smprint("/dev/usb/%s", d[i].name));
		free(d);
	}else
		for(i = 0; i < argc; i++)
			sendp(portc, strdup(argv[i]));
	sendp(portc, nil);
	err = recvp(portc);
	chanfree(portc);
	usbfsexits(0);
	usbfsinit(srv, mnt, &usbdirfs, MAFTER);
	snprint(ctlfs.name, sizeof(ctlfs.name), "usbdctl");
	usbfsadd(&ctlfs);
	threadexits(err);
}
