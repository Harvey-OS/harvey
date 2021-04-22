/*
 * USB Human Interaction Device: keyboard and mouse.
 *
 * If there's no usb keyboard, it tries to setup the mouse, if any.
 * It should be started at boot time.
 *
 * Mouse events are converted to the format of mouse(3)'s mousein file.
 * Keyboard keycodes are translated to scan codes and sent to kbin(3).
 *
 * If there is no keyboard, it tries to setup the mouse properly, else it falls
 * back to boot protocol.
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "hid.h"

enum
{
	Awakemsg= 0xdeaddead,
	Diemsg	= 0xbeefbeef,
	Dwcidle	= 8,
};

typedef struct KDev KDev;
typedef struct Kin Kin;

struct KDev
{
	Dev*	dev;		/* usb device*/
	Dev*	ep;		/* endpoint to get events */
	Kin*	in;		/* used to send events to kernel */
	int	idle;		/* min time between reports (× 4ms) */
	Channel*repeatc;	/* only for keyboard */
	int	accel;		/* only for mouse */
	int	bootp;		/* has associated keyboard */
	int	debug;
	HidRepTempl templ;
	int	(*ptrvals)(KDev *kd, Chain *ch, int *px, int *py, int *pb);
};

/*
 * Kbdin and mousein files must be shared among all instances.
 */
struct Kin
{
	int	ref;
	int	fd;
	char*	name;
};

/*
 * Map for the logitech bluetooth mouse with 8 buttons and wheels.
 *	{ ptr ->mouse}
 *	{ 0x01, 0x01 },	// left
 *	{ 0x04, 0x02 },	// middle
 *	{ 0x02, 0x04 },	// right
 *	{ 0x40, 0x08 },	// up
 *	{ 0x80, 0x10 },	// down
 *	{ 0x10, 0x08 },	// side up
 *	{ 0x08, 0x10 },	// side down
 *	{ 0x20, 0x02 }, // page
 * besides wheel and regular up/down report the 4th byte as 1/-1
 */

/*
 * key code to scan code; for the page table used by
 * the logitech bluetooth keyboard.
 */
static char sctab[256] =
{
[0x00]	0x0,	0x0,	0x0,	0x0,	0x1e,	0x30,	0x2e,	0x20,
[0x08]	0x12,	0x21,	0x22,	0x23,	0x17,	0x24,	0x25,	0x26,
[0x10]	0x32,	0x31,	0x18,	0x19,	0x10,	0x13,	0x1f,	0x14,
[0x18]	0x16,	0x2f,	0x11,	0x2d,	0x15,	0x2c,	0x2,	0x3,
[0x20]	0x4,	0x5,	0x6,	0x7,	0x8,	0x9,	0xa,	0xb,
[0x28]	0x1c,	0x1,	0xe,	0xf,	0x39,	0xc,	0xd,	0x1a,
[0x30]	0x1b,	0x2b,	0x2b,	0x27,	0x28,	0x29,	0x33,	0x34,
[0x38]	0x35,	0x3a,	0x3b,	0x3c,	0x3d,	0x3e,	0x3f,	0x40,
[0x40]	0x41,	0x42,	0x43,	0x44,	0x57,	0x58,	0x63,	0x46,
[0x48]	0x77,	0x52,	0x47,	0x49,	0x53,	0x4f,	0x51,	0x4d,
[0x50]	0x4b,	0x50,	0x48,	0x45,	0x35,	0x37,	0x4a,	0x4e,
[0x58]	0x1c,	0x4f,	0x50,	0x51,	0x4b,	0x4c,	0x4d,	0x47,
[0x60]	0x48,	0x49,	0x52,	0x53,	0x56,	0x7f,	0x74,	0x75,
[0x68]	0x55,	0x59,	0x5a,	0x5b,	0x5c,	0x5d,	0x5e,	0x5f,
[0x70]	0x78,	0x79,	0x7a,	0x7b,	0x0,	0x0,	0x0,	0x0,
[0x78]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x71,
[0x80]	0x73,	0x72,	0x0,	0x0,	0x0,	0x7c,	0x0,	0x0,
[0x88]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0x90]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0x98]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xa0]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xa8]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xb0]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xb8]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xc0]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xc8]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xd0]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xd8]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xe0]	0x1d,	0x2a,	0x38,	0x7d,	0x61,	0x36,	0x64,	0x7e,
[0xe8]	0x0,	0x0,	0x0,	0x0,	0x0,	0x73,	0x72,	0x71,
[0xf0]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
[0xf8]	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
};

static QLock inlck;
static Kin kbdin =
{
	.ref = 0,
	.name = "#Ι/kbin",
	.fd = -1,
};
static Kin ptrin =
{
	.ref = 0,
	.name = "#m/mousein",
	.fd = -1,
};

static int ptrbootpvals(KDev *kd, Chain *ch, int *px, int *py, int *pb);
static int ptrrepvals(KDev *kd, Chain *ch, int *px, int *py, int *pb);

static int
setbootproto(KDev* f, int eid, uchar *, int)
{
	int nr, r, id;

	f->ptrvals = ptrbootpvals;
	r = Rh2d|Rclass|Riface;
	dprint(2, "setting boot protocol\n");
	id = f->dev->usb->ep[eid]->iface->id;
	nr = usbcmd(f->dev, r, Setproto, Bootproto, id, nil, 0);
	if(nr < 0)
		return -1;
	usbcmd(f->dev, r, Setidle, f->idle<<8, id, nil, 0);
	return nr;
}

static uchar ignoredesc[128];

static int
setfirstconfig(KDev* f, int eid, uchar *desc, int descsz)
{
	int nr, r, id, i;

	dprint(2, "setting first config\n");
	if(desc == nil){
		descsz = sizeof ignoredesc;
		desc = ignoredesc;
	}
	id = f->dev->usb->ep[eid]->iface->id;
	r = Rh2d | Rstd | Rdev;
	nr = usbcmd(f->dev,  r, Rsetconf, 1, 0, nil, 0);
	if(nr < 0)
		return -1;
	r = Rh2d | Rclass | Riface;
	nr = usbcmd(f->dev, r, Setidle, f->idle<<8, id, nil, 0);
	if(nr < 0)
		return -1;
	r = Rd2h | Rstd | Riface;
	nr=usbcmd(f->dev,  r, Rgetdesc, Dreport<<8, id, desc, descsz);
	if(nr <= 0)
		return -1;
	if(f->debug){
		fprint(2, "report descriptor:");
		for(i = 0; i < nr; i++){
			if(i%8 == 0)
				fprint(2, "\n\t");
			fprint(2, "%#2.2ux ", desc[i]);
		}
		fprint(2, "\n");
	}
	f->ptrvals = ptrrepvals;
	return nr;
}

/*
 * Try to recover from a babble error. A port reset is the only way out.
 * BUG: we should be careful not to reset a bundle with several devices.
 */
static void
recoverkb(KDev *f)
{
	int i;

	close(f->dev->dfd);		/* it's for usbd now */
	devctl(f->dev, "reset");
	for(i = 0; i < 10; i++){
		if(i == 5)
			f->bootp++;
		sleep(500);
		if(opendevdata(f->dev, ORDWR) >= 0){
			if(f->bootp)
				/* TODO func pointer */
				setbootproto(f, f->ep->id, nil, 0);
			else
				setfirstconfig(f, f->ep->id, nil, 0);
			break;
		}
		/* else usbd still working... */
	}
}

static void
kbfatal(KDev *kd, char *sts)
{
	Dev *dev;

	if(sts != nil)
		fprint(2, "kb: fatal: %s\n", sts);
	else
		fprint(2, "kb: exiting\n");
	if(kd->repeatc != nil)
		nbsendul(kd->repeatc, Diemsg);
	dev = kd->dev;
	kd->dev = nil;
	if(kd->ep != nil)
		closedev(kd->ep);
	kd->ep = nil;
	devctl(dev, "detach");
	closedev(dev);
	/*
	 * free(kd); done by closedev.
	 */
	threadexits(sts);
}

static int
scale(KDev *f, int x)
{
	int sign = 1;

	if(x < 0){
		sign = -1;
		x = -x;
	}
	switch(x){
	case 0:
	case 1:
	case 2:
	case 3:
		break;
	case 4:
		x = 6 + (f->accel>>2);
		break;
	case 5:
		x = 9 + (f->accel>>1);
		break;
	default:
		x *= MaxAcc;
		break;
	}
	return sign*x;
}

/*
 * ps2 mouse is processed mostly at interrupt time.
 * for usb we do what we can.
 */
static void
sethipri(void)
{
	char fn[30];
	int fd;

	snprint(fn, sizeof fn, "/proc/%d/ctl", getpid());
	fd = open(fn, OWRITE);
	if(fd >= 0) {
		fprint(fd, "pri 13");
		close(fd);
	}
}

static int
ptrrepvals(KDev *kd, Chain *ch, int *px, int *py, int *pb)
{
	int i, x, y, b, c;
	static char buts[] = {0x0, 0x2, 0x1};

	c = ch->e / 8;

	/* sometimes there is a report id, sometimes not */
	if(c == kd->templ.sz + 1)
		if(ch->buf[0] == kd->templ.id)
			ch->b += 8;
		else
			return -1;
	parsereport(&kd->templ, ch);

	if(kd->debug > 1)
		dumpreport(&kd->templ);
	if(c < 3)
		return -1;
	x = hidifcval(&kd->templ, KindX, 0);
	y = hidifcval(&kd->templ, KindY, 0);
	b = 0;
	for(i = 0; i<sizeof buts; i++)
		b |= (hidifcval(&kd->templ, KindButtons, i) & 1) << buts[i];
	if(c > 3 && hidifcval(&kd->templ, KindWheel, 0) > 0)	/* up */
		b |= 0x08;
	if(c > 3 && hidifcval(&kd->templ, KindWheel, 0) < 0)	/* down */
		b |= 0x10;

	*px = x;
	*py = y;
	*pb = b;
	return 0;
}

static int
ptrbootpvals(KDev *kd, Chain *ch, int *px, int *py, int *pb)
{
	int b, c;
	char x, y;
	static char maptab[] = {0x0, 0x1, 0x4, 0x5, 0x2, 0x3, 0x6, 0x7};

	c = ch->e / 8;
	if(c < 3)
		return -1;
	if(kd->templ.nifcs){
		x = hidifcval(&kd->templ, KindX, 0);
		y = hidifcval(&kd->templ, KindY, 0);
	}else{
		/* no report descriptor for boot protocol */
		x = ((signed char*)ch->buf)[1];
		y = ((signed char*)ch->buf)[2];
	}

	b = maptab[ch->buf[0] & 0x7];
	if(c > 3 && ch->buf[3] == 1)		/* up */
		b |= 0x08;
	if(c > 3 && ch->buf[3] == 0xff)		/* down */
		b |= 0x10;
	*px = x;
	*py = y;
	*pb = b;
	return 0;
}

static void
ptrwork(void* a)
{
	int hipri, mfd, nerrs, x, y, b, c, ptrfd;
	char mbuf[80];
	Chain ch;
	KDev* f = a;

	threadsetname("ptr %s", f->ep->dir);
	hipri = nerrs = 0;
	ptrfd = f->ep->dfd;
	mfd = f->in->fd;
	if(f->ep->maxpkt < 3 || f->ep->maxpkt > MaxChLen)
		kbfatal(f, "weird mouse maxpkt");
	for(;;){
		memset(ch.buf, 0, MaxChLen);
		if(f->ep == nil)
			kbfatal(f, nil);
		c = read(ptrfd, ch.buf, f->ep->maxpkt);
		assert(f->dev != nil);
		assert(f->ep != nil);
		if(c < 0){
			dprint(2, "kb: mouse: %s: read: %r\n", f->ep->dir);
			if(++nerrs < 3){
				recoverkb(f);
				continue;
			}
		}
		if(c <= 0)
			kbfatal(f, nil);
		ch.b = 0;
		ch.e = 8 * c;
		if(f->ptrvals(f, &ch, &x, &y, &b) < 0)
			continue;
		if(f->accel){
			x = scale(f, x);
			y = scale(f, y);
		}
		if(f->debug > 1)
			fprint(2, "kb: m%11d %11d %11d\n", x, y, b);
		seprint(mbuf, mbuf+sizeof(mbuf), "m%11d %11d %11d", x, y,b);
		if(write(mfd, mbuf, strlen(mbuf)) < 0)
			kbfatal(f, "mousein i/o");
		if(hipri == 0){
			sethipri();
			hipri = 1;
		}
	}
}

static void
stoprepeat(KDev *f)
{
	sendul(f->repeatc, Awakemsg);
}

static void
startrepeat(KDev *f, uchar esc1, uchar sc)
{
	ulong c;

	if(esc1)
		c = SCesc1 << 8 | (sc & 0xff);
	else
		c = sc;
	sendul(f->repeatc, c);
}

static void
putscan(KDev *f, uchar esc, uchar sc)
{
	int kbinfd;
	uchar s[2] = {SCesc1, 0};

	kbinfd = f->in->fd;
	if(sc == 0x41){
		f->debug += 2;
		return;
	}
	if(sc == 0x42){
		f->debug = 0;
		return;
	}
	if(f->debug > 1)
		fprint(2, "sc: %x %x\n", (esc? SCesc1: 0), sc);
	s[1] = sc;
	if(esc && sc != 0)
		write(kbinfd, s, 2);
	else if(sc != 0)
		write(kbinfd, s+1, 1);
}

static void
repeatproc(void* a)
{
	KDev *f;
	Channel *repeatc;
	ulong l, t, i;
	uchar esc1, sc;

	threadsetname("kbd repeat");
	/*
	 * too many jumps here.
	 * Rewrite instead of debug, if needed.
	 */
	f = a;
	repeatc = f->repeatc;
	l = Awakemsg;
Repeat:
	if(l == Diemsg)
		goto Abort;
	while(l == Awakemsg)
		l = recvul(repeatc);
	if(l == Diemsg)
		goto Abort;
	esc1 = l >> 8;
	sc = l;
	t = 160;
	for(;;){
		for(i = 0; i < t; i += 5){
			if(l = nbrecvul(repeatc))
				goto Repeat;
			sleep(5);
		}
		putscan(f, esc1, sc);
		t = 30;
	}
Abort:
	chanfree(repeatc);
	threadexits("aborted");

}


#define hasesc1(sc)	((sc) >= 0x47 || (sc) == 0x38)

static void
putmod(KDev *f, uchar mods, uchar omods, uchar mask, uchar esc, uchar sc)
{
	/* BUG: Should be a single write */
	if((mods&mask) && !(omods&mask))
		putscan(f, esc, sc);
	if(!(mods&mask) && (omods&mask))
		putscan(f, esc, Keyup|sc);
}

/*
 * This routine diffs the state with the last known state
 * and invents the scan codes that would have been sent
 * by a non-usb keyboard in that case. This also requires supplying
 * the extra esc1 byte as well as keyup flags.
 * The aim is to allow future addition of other keycode pages
 * for other keyboards.
 */
static uchar
putkeys(KDev *f, uchar buf[], uchar obuf[], int n, uchar dk)
{
	int i, j;
	uchar uk;

	putmod(f, buf[0], obuf[0], Mctrl, 0, SCctrl);
	putmod(f, buf[0], obuf[0], (1<<Mlshift), 0, SClshift);
	putmod(f, buf[0], obuf[0], (1<<Mrshift), 0, SCrshift);
	putmod(f, buf[0], obuf[0], Mcompose, 0, SCcompose);
	putmod(f, buf[0], obuf[0], Maltgr, 1, SCcompose);

	/* Report key downs */
	for(i = 2; i < n; i++){
		for(j = 2; j < n; j++)
			if(buf[i] == obuf[j])
			 	break;
		if(j == n && buf[i] != 0){
			dk = sctab[buf[i]];
			putscan(f, hasesc1(dk), dk);
			startrepeat(f, hasesc1(dk), dk);
		}
	}

	/* Report key ups */
	uk = 0;
	for(i = 2; i < n; i++){
		for(j = 2; j < n; j++)
			if(obuf[i] == buf[j])
				break;
		if(j == n && obuf[i] != 0){
			uk = sctab[obuf[i]];
			putscan(f, hasesc1(uk), uk|Keyup);
		}
	}
	if(uk && (dk == 0 || dk == uk)){
		stoprepeat(f);
		dk = 0;
	}
	return dk;
}

static int
kbdbusy(uchar* buf, int n)
{
	int i;

	for(i = 1; i < n; i++)
		if(buf[i] == 0 || buf[i] != buf[0])
			return 0;
	return 1;
}

static void
kbdwork(void *a)
{
	int c, i, kbdfd, nerrs;
	uchar dk, buf[64], lbuf[64];
	char err[128];
	KDev *f = a;

	threadsetname("kbd %s", f->ep->dir);
	kbdfd = f->ep->dfd;

	if(f->ep->maxpkt < 3 || f->ep->maxpkt > sizeof buf)
		kbfatal(f, "weird maxpkt");

	f->repeatc = chancreate(sizeof(ulong), 0);
	if(f->repeatc == nil)
		kbfatal(f, "chancreate failed");

	proccreate(repeatproc, f, Stack);
	memset(lbuf, 0, sizeof lbuf);
	dk = nerrs = 0;
	for(;;){
		memset(buf, 0, sizeof buf);
		c = read(kbdfd, buf, f->ep->maxpkt);
		assert(f->dev != nil);
		assert(f->ep != nil);
		if(c < 0){
			rerrstr(err, sizeof(err));
			fprint(2, "kb: %s: read: %s\n", f->ep->dir, err);
			if(strstr(err, "babble") != 0 && ++nerrs < 3){
				recoverkb(f);
				continue;
			}
		}
		if(c <= 0)
			kbfatal(f, nil);
		if(c < 3)
			continue;
		if(kbdbusy(buf + 2, c - 2))
			continue;
		if(usbdebug > 2 || f->debug > 1){
			fprint(2, "kbd mod %x: ", buf[0]);
			for(i = 2; i < c; i++)
				fprint(2, "kc %x ", buf[i]);
			fprint(2, "\n");
		}
		dk = putkeys(f, buf, lbuf, f->ep->maxpkt, dk);
		memmove(lbuf, buf, c);
		nerrs = 0;
	}
}

static void
freekdev(void *a)
{
	KDev *kd;

	kd = a;
	if(kd->in != nil){
		qlock(&inlck);
		if(--kd->in->ref == 0){
			close(kd->in->fd);
			kd->in->fd = -1;
		}
		qunlock(&inlck);
	}
	dprint(2, "freekdev\n");
	free(kd);
}

static void
kbstart(Dev *d, Ep *ep, Kin *in, void (*f)(void*), KDev *kd)
{
	uchar desc[512];
	int n, res;

	qlock(&inlck);
	if(in->fd < 0){
		in->fd = open(in->name, OWRITE);
		if(in->fd < 0){
			fprint(2, "kb: %s: %r\n", in->name);
			qunlock(&inlck);
			return;
		}
	}
	in->ref++;	/* for kd->in = in */
	qunlock(&inlck);
	d->free = freekdev;
	kd->in = in;
	kd->dev = d;
	res = -1;
	kd->ep = openep(d, ep->id);
	if(kd->ep == nil){
		fprint(2, "kb: %s: openep %d: %r\n", d->dir, ep->id);
		return;
	}
	if(in == &kbdin){
		/*
		 * DWC OTG controller misses some split transaction inputs.
		 * Set nonzero idle time to return more frequent reports
		 * of keyboard state, to avoid losing key up/down events.
		 */
		n = read(d->cfd, desc, sizeof desc - 1);
		if(n > 0){
			desc[n] = 0;
			if(strstr((char*)desc, "dwcotg") != nil)
				kd->idle = Dwcidle;
		}
	}
	if(!kd->bootp)
		res= setfirstconfig(kd, ep->id, desc, sizeof desc);
	if(res > 0)
		res = parsereportdesc(&kd->templ, desc, sizeof desc);
	/* if we could not set the first config, we give up */
	if(kd->bootp || res < 0){
		kd->bootp = 1;
		if(setbootproto(kd, ep->id, nil, 0) < 0){
			fprint(2, "kb: %s: bootproto: %r\n", d->dir);
			return;
		}
	}else if(kd->debug)
		dumpreport(&kd->templ);
	if(opendevdata(kd->ep, OREAD) < 0){
		fprint(2, "kb: %s: opendevdata: %r\n", kd->ep->dir);
		closedev(kd->ep);
		kd->ep = nil;
		return;
	}

	incref(d);
	proccreate(f, kd, Stack);
}

static int
usage(void)
{
	werrstr("usage: usb/kb [-bdkm] [-a n] [-N nb]");
	return -1;
}

int
kbmain(Dev *d, int argc, char* argv[])
{
	int bootp, i, kena, pena, accel, devid, debug;
	Ep *ep;
	KDev *kd;
	Usbdev *ud;

	kena = pena = 1;
	bootp = 0;
	accel = 0;
	debug = 0;
	devid = d->id;
	ARGBEGIN{
	case 'a':
		accel = strtol(EARGF(usage()), nil, 0);
		break;
	case 'd':
		debug++;
		break;
	case 'k':
		kena = 1;
		pena = 0;
		break;
	case 'm':
		kena = 0;
		pena = 1;
		break;
	case 'N':
		devid = atoi(EARGF(usage()));		/* ignore dev number */
		break;
	case 'b':
		bootp++;
		break;
	default:
		return usage();
	}ARGEND;
	if(argc != 0)
		return usage();
	USED(devid);
	ud = d->usb;
	d->aux = nil;
	dprint(2, "kb: main: dev %s ref %ld\n", d->dir, d->ref);

	if(kena)
		for(i = 0; i < nelem(ud->ep); i++)
			if((ep = ud->ep[i]) == nil)
				break;
			else if(ep->iface->csp == KbdCSP)
				bootp = 1;

	for(i = 0; i < nelem(ud->ep); i++){
		if((ep = ud->ep[i]) == nil)
			continue;
		if(kena && ep->type == Eintr && ep->dir == Ein &&
		    ep->iface->csp == KbdCSP){
			kd = d->aux = emallocz(sizeof(KDev), 1);
			kd->accel = 0;
			kd->bootp = 1;
			kd->debug = debug;
			kbstart(d, ep, &kbdin, kbdwork, kd);
		}
		if(pena && ep->type == Eintr && ep->dir == Ein &&
		    ep->iface->csp == PtrCSP){
			kd = d->aux = emallocz(sizeof(KDev), 1);
			kd->accel = accel;
			kd->bootp = bootp;
			kd->debug = debug;
			kbstart(d, ep, &ptrin, ptrwork, kd);
		}
	}
	return 0;
}
