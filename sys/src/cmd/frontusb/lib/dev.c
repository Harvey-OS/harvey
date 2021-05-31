#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"

/*
 * epN.M -> N
 */
static int
nameid(char *s)
{
	char *r;
	char nm[20];

	r = strrchr(s, 'p');
	if(r == nil)
		return -1;
	strecpy(nm, nm+sizeof(nm), r+1);
	r = strchr(nm, '.');
	if(r == nil)
		return -1;
	*r = 0;
	return atoi(nm);
}

Dev*
openep(Dev *d, int id)
{
	char *mode;	/* How many modes? */
	Ep *ep;
	Altc *ac;
	Dev *epd;
	Usbdev *ud;
	char name[40];

	if(access("/dev/usb", AEXIST) < 0 && bind("#u", "/dev", MBEFORE) < 0)
		return nil;
	if(d->cfd < 0 || d->usb == nil){
		werrstr("device not configured");
		return nil;
	}
	ud = d->usb;
	if(id < 0 || id >= nelem(ud->ep) || ud->ep[id] == nil){
		werrstr("bad enpoint number");
		return nil;
	}
	ep = ud->ep[id];
	snprint(name, sizeof(name), "/dev/usb/ep%d.%d", d->id, id);
	if(access(name, AEXIST) == 0){
		dprint(2, "%s: %s already exists; trying to open\n", argv0, name);
		epd = opendev(name);
		if(epd != nil){
			epd->id = id;
			epd->maxpkt = ep->maxpkt;	/* guess */
		}
		return epd;
	}
	mode = "rw";
	if(ep->dir == Ein)
		mode = "r";
	if(ep->dir == Eout)
		mode = "w";
	if(devctl(d, "new %d %d %s", id, ep->type, mode) < 0){
		dprint(2, "%s: %s: new: %r\n", argv0, d->dir);
		return nil;
	}
	epd = opendev(name);
	if(epd == nil)
		return nil;
	epd->id = id;
	if(devctl(epd, "maxpkt %d", ep->maxpkt) < 0)
		fprint(2, "%s: %s: openep: maxpkt: %r\n", argv0, epd->dir);
	else
		dprint(2, "%s: %s: maxpkt %d\n", argv0, epd->dir, ep->maxpkt);
	epd->maxpkt = ep->maxpkt;
	ac = ep->iface->altc[0];
	if(ep->ntds > 1 && devctl(epd, "ntds %d", ep->ntds) < 0)
		fprint(2, "%s: %s: openep: ntds: %r\n", argv0, epd->dir);
	else
		dprint(2, "%s: %s: ntds %d\n", argv0, epd->dir, ep->ntds);

	if(ac != nil && (ep->type == Eintr || ep->type == Eiso) && ac->interval != 0)
		if(devctl(epd, "pollival %d", ac->interval) < 0)
			fprint(2, "%s: %s: openep: pollival: %r\n", argv0, epd->dir);
	return epd;
}

Dev*
opendev(char *fn)
{
	Dev *d;
	int l;

	if(access("/dev/usb", AEXIST) < 0 && bind("#u", "/dev", MBEFORE) < 0)
		return nil;
	d = emallocz(sizeof(Dev), 1);
	incref(d);

	l = strlen(fn);
	d->dfd = -1;
	/*
	 * +30 to allocate extra size to concat "/<epfilename>"
	 * we should probably remove that feature from the manual
	 * and from the code after checking out that nobody relies on
	 * that.
	 */
	d->dir = emallocz(l + 30, 0);
	strcpy(d->dir, fn);
	strcpy(d->dir+l, "/ctl");
	d->cfd = open(d->dir, ORDWR|OCEXEC);
	d->dir[l] = 0;
	d->id = nameid(fn);
	if(d->cfd < 0){
		werrstr("can't open endpoint %s: %r", d->dir);
		free(d->dir);
		free(d);
		return nil;
	}
	d->hname = nil;
	dprint(2, "%s: opendev %#p %s\n", argv0, d, fn);
	return d;
}

int
opendevdata(Dev *d, int mode)
{
	char buf[80]; /* more than enough for a usb path */

	seprint(buf, buf+sizeof(buf), "%s/data", d->dir);
	d->dfd = open(buf, mode|OCEXEC);
	return d->dfd;
}

enum
{
	/*
	 * Max device conf is also limited by max control request size as
	 * limited by Maxctllen in the kernel usb.h (both limits are arbitrary).
	 * Some devices ignore the high byte of control transfer reads so keep
	 * the low byte all ones. asking for 16K kills Newsham's disk.
	 */
	Maxdevconf = 4*1024 - 1,
};

int
loaddevconf(Dev *d, int n)
{
	uchar *buf;
	int nr;
	int type;

	if(n >= nelem(d->usb->conf)){
		werrstr("loaddevconf: bug: out of configurations in device");
		fprint(2, "%s: %r\n", argv0);
		return -1;
	}
	buf = emallocz(Maxdevconf, 0);
	type = Rd2h|Rstd|Rdev;
	nr = usbcmd(d, type, Rgetdesc, Dconf<<8|n, 0, buf, Maxdevconf);
	if(nr < Dconflen){
		free(buf);
		return -1;
	}
	if(d->usb->conf[n] == nil)
		d->usb->conf[n] = emallocz(sizeof(Conf), 1);
	nr = parseconf(d->usb, d->usb->conf[n], buf, nr);
	free(buf);
	return nr;
}

Ep*
mkep(Usbdev *d, int id)
{
	Ep *ep;

	d->ep[id] = ep = emallocz(sizeof(Ep), 1);
	ep->id = id;
	return ep;
}

static char*
mkstr(uchar *b, int n)
{
	Rune r;
	char *us;
	char *s;

	if(n > 0 && n > b[0])
		n = b[0];
	if(n <= 2 || (n & 1) != 0)
		return strdup("none");
	n = (n - 2)/2;
	b += 2;
	us = s = emallocz(n*UTFmax+1, 0);
	for(; --n >= 0; b += 2){
		r = GET2(b);
		s += runetochar(s, &r);
	}
	*s = 0;
	return us;
}

char*
loaddevstr(Dev *d, int sid)
{
	uchar buf[256-2];	/* keep size < 256 */
	int langid;
	int type;
	int nr;

	if(sid == 0)
		return estrdup("none");
	type = Rd2h|Rstd|Rdev;

	/*
	 * there are devices which do not return a string if used
	 * with invalid language id, so at least try to use the first
	 * one and choose english if failed
	 */
	nr=usbcmd(d, type, Rgetdesc, Dstr<<8, 0, buf, sizeof(buf));
	if(nr < 4)
		langid = 0x0409;	// english
	else
		langid = buf[3]<<8|buf[2];

	nr=usbcmd(d, type, Rgetdesc, Dstr<<8|sid, langid, buf, sizeof(buf));
	return mkstr(buf, nr);
}

int
loaddevdesc(Dev *d)
{
	uchar buf[Ddevlen];
	int nr;
	int type;
	Ep *ep0;

	type = Rd2h|Rstd|Rdev;
	nr = sizeof(buf);
	memset(buf, 0, nr);
	if((nr=usbcmd(d, type, Rgetdesc, Ddev<<8|0, 0, buf, nr)) < 0)
		return -1;
	/*
	 * Several hubs are returning descriptors of 17 bytes, not 18.
	 * We accept them and leave number of configurations as zero.
	 * (a get configuration descriptor also fails for them!)
	 */
	if(nr < Ddevlen){
		print("%s: %s: warning: device with short descriptor\n",
			argv0, d->dir);
		if(nr < Ddevlen-1){
			werrstr("short device descriptor (%d bytes)", nr);
			return -1;
		}
	}
	d->usb = emallocz(sizeof(Usbdev), 1);
	ep0 = mkep(d->usb, 0);
	ep0->dir = Eboth;
	ep0->type = Econtrol;
	ep0->maxpkt = d->maxpkt = 8;		/* a default */
	nr = parsedev(d, buf, nr);
	if(nr >= 0){
		d->usb->vendor = loaddevstr(d, d->usb->vsid);
		if(strcmp(d->usb->vendor, "none") != 0){
			d->usb->product = loaddevstr(d, d->usb->psid);
			d->usb->serial = loaddevstr(d, d->usb->ssid);
		}
	}
	return nr;
}

int
configdev(Dev *d)
{
	int i;

	if(d->dfd < 0)
		opendevdata(d, ORDWR);
	if(d->dfd < 0)
		return -1;
	if(loaddevdesc(d) < 0)
		return -1;
	for(i = 0; i < d->usb->nconf; i++)
		if(loaddevconf(d, i) < 0)
			return -1;
	return 0;
}

static void
closeconf(Conf *c)
{
	int i;
	int a;

	if(c == nil)
		return;
	for(i = 0; i < nelem(c->iface); i++)
		if(c->iface[i] != nil){
			for(a = 0; a < nelem(c->iface[i]->altc); a++)
				free(c->iface[i]->altc[a]);
			free(c->iface[i]);
		}
	free(c);
}

void
closedev(Dev *d)
{
	int i;
	Usbdev *ud;

	if(d==nil || decref(d) != 0)
		return;
	dprint(2, "%s: closedev %#p %s\n", argv0, d, d->dir);
	if(d->cfd >= 0)
		close(d->cfd);
	if(d->dfd >= 0)
		close(d->dfd);
	d->cfd = d->dfd = -1;
	free(d->dir);
	d->dir = nil;
	free(d->hname);
	d->hname = nil;
	ud = d->usb;
	d->usb = nil;
	if(ud != nil){
		free(ud->vendor);
		free(ud->product);
		free(ud->serial);
		for(i = 0; i < nelem(ud->ep); i++)
			free(ud->ep[i]);
		for(i = 0; i < nelem(ud->ddesc); i++)
			free(ud->ddesc[i]);

		for(i = 0; i < nelem(ud->conf); i++)
			closeconf(ud->conf[i]);
		free(ud);
	}
	free(d);
}

static char*
reqstr(int type, int req)
{
	char *s;
	static char* ds[] = { "dev", "if", "ep", "oth" };
	static char buf[40];

	if(type&Rd2h)
		s = seprint(buf, buf+sizeof(buf), "d2h");
	else
		s = seprint(buf, buf+sizeof(buf), "h2d");
	if(type&Rclass)
		s = seprint(s, buf+sizeof(buf), "|cls");
	else if(type&Rvendor)
		s = seprint(s, buf+sizeof(buf), "|vnd");
	else
		s = seprint(s, buf+sizeof(buf), "|std");
	s = seprint(s, buf+sizeof(buf), "|%s", ds[type&3]);

	switch(req){
	case Rgetstatus: s = seprint(s, buf+sizeof(buf), " getsts"); break;
	case Rclearfeature: s = seprint(s, buf+sizeof(buf), " clrfeat"); break;
	case Rsetfeature: s = seprint(s, buf+sizeof(buf), " setfeat"); break;
	case Rsetaddress: s = seprint(s, buf+sizeof(buf), " setaddr"); break;
	case Rgetdesc: s = seprint(s, buf+sizeof(buf), " getdesc"); break;
	case Rsetdesc: s = seprint(s, buf+sizeof(buf), " setdesc"); break;
	case Rgetconf: s = seprint(s, buf+sizeof(buf), " getcnf"); break;
	case Rsetconf: s = seprint(s, buf+sizeof(buf), " setcnf"); break;
	case Rgetiface: s = seprint(s, buf+sizeof(buf), " getif"); break;
	case Rsetiface: s = seprint(s, buf+sizeof(buf), " setif"); break;
	}
	USED(s);
	return buf;
}

static int
cmdreq(Dev *d, int type, int req, int value, int index, uchar *data, int count)
{
	int ndata, n;
	uchar *wp;
	uchar buf[8];
	char *hd, *rs;

	assert(d != nil);
	if(data == nil){
		wp = buf;
		ndata = 0;
	}else{
		ndata = count;
		wp = emallocz(8+ndata, 0);
		memmove(wp+8, data, ndata);
	}
	wp[0] = type;
	wp[1] = req;
	PUT2(wp+2, value);
	PUT2(wp+4, index);
	PUT2(wp+6, count);
	if(usbdebug>2){
		hd = hexstr(wp, ndata+8);
		rs = reqstr(type, req);
		fprint(2, "%s: %s val %d|%d idx %d cnt %d out[%d] %s\n",
			d->dir, rs, value>>8, value&0xFF,
			index, count, ndata+8, hd);
		free(hd);
	}
	n = write(d->dfd, wp, 8+ndata);
	if(wp != buf)
		free(wp);
	if(n < 0)
		return -1;
	if(n != 8+ndata){
		dprint(2, "%s: cmd: short write: %d\n", argv0, n);
		return -1;
	}
	return ndata;
}

static int
cmdrep(Dev *d, void *buf, int nb)
{
	char *hd;

	nb = read(d->dfd, buf, nb);
	if(nb > 0 && usbdebug > 2){
		hd = hexstr(buf, nb);
		fprint(2, "%s: in[%d] %s\n", d->dir, nb, hd);
		free(hd);
	}
	return nb;
}

int
usbcmd(Dev *d, int type, int req, int value, int index, uchar *data, int count)
{
	int i, r, nerr;
	char err[64];

	/*
	 * Some devices do not respond to commands some times.
	 * Others even report errors but later work just fine. Retry.
	 */
	r = -1;
	*err = 0;
	for(i = nerr = 0; i < Uctries; i++){
		if(type & Rd2h)
			r = cmdreq(d, type, req, value, index, nil, count);
		else
			r = cmdreq(d, type, req, value, index, data, count);
		if(r >= 0){
			if((type & Rd2h) == 0)
				break;
			r = cmdrep(d, data, count);
			if(r > 0)
				break;
			if(r == 0)
				werrstr("no data from device");
		}
		nerr++;
		if(*err == 0)
			rerrstr(err, sizeof(err));
		sleep(Ucdelay);
	}
	if(r >= 0 && i >= 2)
		/* let the user know the device is not in good shape */
		fprint(2, "%s: usbcmd: %s: required %d attempts (%s)\n",
			argv0, d->dir, i, err);
	return r;
}

int
unstall(Dev *dev, Dev *ep, int dir)
{
	int r;

	if(dir == Ein)
		dir = 0x80;
	else
		dir = 0;
	r = Rh2d|Rstd|Rep;
	if(usbcmd(dev, r, Rclearfeature, Fhalt, (ep->id&0xF)|dir, nil, 0)<0){
		werrstr("unstall: %s: %r", ep->dir);
		return -1;
	}
	if(devctl(ep, "clrhalt") < 0){
		werrstr("clrhalt: %s: %r", ep->dir);
		return -1;
	}
	return 0;
}

/*
 * To be sure it uses a single write.
 */
int
devctl(Dev *dev, char *fmt, ...)
{
	char buf[128];
	va_list arg;
	char *e;

	va_start(arg, fmt);
	e = vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	return write(dev->cfd, buf, e-buf);
}

Dev *
getdev(char *devid)
{
	char buf[40], *p;
	Dev *d;

	if(devid[0] == '/' || devid[0] == '#'){
		snprint(buf, sizeof buf, "%s", devid);
		p = strrchr(buf, '/');
		if(p != nil){
			p = strrchr(buf, ':');
			if(p != nil)
				*p = 0;
		}
	} else {
		p = nil;
		snprint(buf, sizeof buf, "/dev/usb/ep%ld.0", strtol(devid, &p, 10));
		if(*p != ':')
			p = nil;
	}

	d = opendev(buf);
	if(d == nil)
		return nil;
	if(configdev(d) < 0){
		closedev(d);
		return nil;
	}

	if(p == nil){
		snprint(buf, sizeof buf, ":%d", d->id);
		p = buf;
	}
	d->hname = strdup(p+1);

	return d;
}
