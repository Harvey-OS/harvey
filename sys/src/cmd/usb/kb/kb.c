/*
 * USB Human Interaction Device: keyboard and mouse.
 *
 * If there's no usb keyboard, it tries to setup the mouse, if any.
 * It should be started at boot time.
 *
 * Mouse events are converted to the format of mouse(3)'s
 * mousein file.
 * Keyboard keycodes are translated to scan codes and sent to kbin(3).
 *
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "hid.h"

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
char sctab[256] = 
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

typedef struct Dev Dev;
struct Dev {
	char*	name;		/* debug */
	void	(*proc)(void*);	/* handler process */
	int	csp;		/* the csp we want */
	int	enabled;	/* can we start it? */
	int	ctlrno;		/* controller number, for probing*/
	int	id;		/* device id, for probing*/
	int	ep;		/* endpoint used to get events */
	int	msz;		/* message size */
	int	fd;		/* to the endpoint data file */
	Device*	dev;		/* usb device*/
};


Dev	kf, pf;			/* kbd and pointer drivers*/
Channel *repeatc;		/* channel to request key repeating*/

void (*dprinter[])(Device *, int, ulong, void *b, int n) = {
	[STRING] pstring,
	[DEVICE] pdevice,
	[HID] phid,
};

int	accel;
int	hdebug;
int	dryrun;
int	verbose;

int mainstacksize = 32*1024;

int
robusthandler(void*, char *s)
{
	if(hdebug)
		fprint(2, "inthandler: %s\n", s);
	return (s && (strstr(s, "interrupted")|| strstr(s, "hangup")));
}

long
robustread(int fd, void *buf, long sz)
{
	long r;
	char err[ERRMAX];

	do {
		r = read(fd , buf, sz);
		if(r < 0)
			rerrstr(err, sizeof err);
	} while(r < 0 && robusthandler(nil, err));
	return r;
}

static int
scale(int x)
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
		x = 6 + (accel>>2);
		break;
	case 5:
		x = 9 + (accel>>1);
		break;
	default:
		x *= MaxAcc;
		break;
	}
	return sign*x;
}

void
ptrwork(void* a)
{
	int x, y, b, c, mfd, ptrfd;
	char	buf[32];
	Dev*	f = a;
	static char maptab[] = {0x0, 0x1, 0x4, 0x5, 0x2, 0x3, 0x6, 0x7};

	ptrfd = f->fd;
	if(ptrfd < 0)
		return;
	mfd = -1;
	if(f->msz < 3 || f->msz > sizeof buf)
		sysfatal("bug: ptrwork: bad mouse maxpkt");
	if(!dryrun){
		mfd = open("#m/mousein", OWRITE);
		if(mfd < 0)
			sysfatal("%s: mousein: %r", f->name);
	}
	for(;;){
		memset(buf, 0, sizeof buf);
		c = robustread(ptrfd, buf, f->msz);
		if(c == 0)
			fprint(2, "%s: %s: eof\n", argv0, f->name);
		if(c < 0)
			fprint(2, "%s: %s: read: %r\n", argv0, f->name);
		if(c <= 0){
			if(!dryrun)
				close(mfd);
			close(f->fd);
			threadexits("read");
		}
		if(c < 3)
			continue;
		if(accel) {
			x = scale(buf[1]);
			y = scale(buf[2]);
		} else {
			x = buf[1];
			y = buf[2];
		}
		b = maptab[buf[0] & 0x7];
		if(c > 3 && buf[3] == 1)	/* up */
			b |= 0x08;
		if(c > 3 && buf[3] == -1)	/* down */
			b |= 0x10;
		if(hdebug)
			fprint(2, "%s: %s: m%11d %11d %11d\n",
				argv0, f->name, x, y, b);
		if(!dryrun)
			if(fprint(mfd, "m%11d %11d %11d", x, y, b) < 0){
				fprint(2, "%s: #m/mousein: write: %r", argv0);
				close(mfd);
				close(f->fd);
				threadexits("write");
			}
	}
}

static void
stoprepeat(void)
{
	sendul(repeatc, Awakemsg);
}

static void
startrepeat(uchar esc1, uchar sc)
{
	ulong c;

	if(esc1)
		c = SCesc1 << 8 | (sc & 0xff);
	else
		c = sc;
	sendul(repeatc, c);
}

static int kbinfd = -1;

static void
putscan(uchar esc, uchar sc)
{
	static uchar s[2] = {SCesc1, 0};

	if(sc == 0x41){
		hdebug = 1;
		return;
	}
	if(sc == 0x42){
		hdebug = 0;
		return;
	}
	if(kbinfd < 0 && !dryrun)
		kbinfd = open("#Ι/kbin", OWRITE);
	if(kbinfd < 0 && !dryrun)
		sysfatal("/dev/kbin: %r");
	if(hdebug)
		fprint(2, "sc: %x %x\n", (esc? SCesc1: 0), sc);
	if(!dryrun){
		s[1] = sc;
		if(esc && sc != 0)
			write(kbinfd, s, 2);
		else if(sc != 0)
			write(kbinfd, s+1, 1);
	}
}

static void
repeatproc(void*)
{
	ulong l;
	uchar esc1, sc;

	assert(sizeof(int) <= sizeof(void*));

	for(;;){
		l = recvul(repeatc);		/*  wait for work*/
		if(l == 0xdeaddead)
			continue;
		esc1 = l >> 8;
		sc = l;
		for(;;){
			putscan(esc1, sc);
			sleep(80);
			l = nbrecvul(repeatc);
			if(l != 0){		/*  stop repeating*/
				if(l != Awakemsg)
					fprint(2, "kb: race: should be awake mesg\n");
				break;
			}
		}
	}
}


#define hasesc1(sc)	(((sc) > 0x47) || ((sc) == 0x38))

static void
putmod(uchar mods, uchar omods, uchar mask, uchar esc, uchar sc)
{
	if((mods&mask) && !(omods&mask))
		putscan(esc, sc);
	if(!(mods&mask) && (omods&mask))
		putscan(esc, Keyup|sc);
}

/*
 * This routine diffs the state with the last known state
 * and invents the scan codes that would have been sent
 * by a non-usb keyboard in that case. This also requires supplying
 * the extra esc1 byte as well as keyup flags.
 * The aim is to allow future addition of other keycode pages
 * for other keyboards.
 */
static void
putkeys(uchar buf[], uchar obuf[], int n)
{
	int i, j;
	uchar sc;
	static int repeating = 0, times = 0;
	static uchar last = 0;

	putmod(buf[0], obuf[0], Mctrl, 0, SCctrl);
	putmod(buf[0], obuf[0], (1<<Mlshift), 0, SClshift);
	putmod(buf[0], obuf[0], (1<<Mrshift), 0, SCrshift);
	putmod(buf[0], obuf[0], Mcompose, 0, SCcompose);
	putmod(buf[0], obuf[0], Maltgr, 1, SCcompose);

	/*
	 * If we get three times the same (single) key, we start
	 * repeating. Otherwise, we stop repeating and
	 * perform normal processing.
	 */
	if(buf[2] != 0 && buf[3] == 0 && buf[2] == last){
		if(repeating)
			return;	/* already being done */
		times++;
		if(times >= 2){
			repeating = 1;
			sc = sctab[buf[2]];
			startrepeat(hasesc1(sc), sc);
			return;
		}
	} else
		times = 0;
	last = buf[2];
	if(repeating){
		repeating = 0;
		stoprepeat();
	}

	/* Report key downs */
	for(i = 2; i < n; i++){
		for(j = 2; j < n; j++)
			if(buf[i] == obuf[j])
			 	break;
		if(j == n && buf[i] != 0){
			sc = sctab[buf[i]];
			putscan(hasesc1(sc), sc);
		}
	}

	/* Report key ups */
	for(i = 2; i < n; i++){
		for(j = 2; j < n; j++)
			if(obuf[i] == buf[j])
				break;
		if(j == n && obuf[i] != 0){
			sc = sctab[obuf[i]];
			putscan(hasesc1(sc), sc|Keyup);
		}
	}
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

void
kbdwork(void *a)
{
	int c, i, kbdfd;
	uchar buf[64], lbuf[64];
	Dev *f = a;

	kbdfd = f->fd;
	if(kbdfd < 0)
		return;
	if(f->msz < 3 || f->msz > sizeof buf)
		sysfatal("bug: ptrwork: bad kbd maxpkt");
	repeatc = chancreate(sizeof(ulong), 0);
	if(repeatc == nil)
		sysfatal("repeat chan: %r");
	proccreate(repeatproc, nil, 32*1024);
	memset(lbuf, 0, sizeof lbuf);
	for(;;) {
		memset(buf, 0, sizeof buf);
		c = robustread(kbdfd, buf, f->msz);
		if(c == 0)
			fprint(2, "%s: %s: eof\n", argv0, f->name);
		else if(c < 0)
			fprint(2, "%s: %s: read: %r\n", argv0, f->name);
		if(c <= 0){
			if(!dryrun)
				close(kbinfd);
			close(f->fd);
			threadexits("read");
		}
		if(c < 3)
			continue;
		if(kbdbusy(buf + 2, c - 2))
			continue;
		if(hdebug > 1){
			fprint(2, "kbd mod %x: ", buf[0]);
			for(i = 2; i < c; i++)
				fprint(2, "kc %x ", buf[i]);
			fprint(2, "\n");
		}
		putkeys(buf, lbuf, f->msz);
		memmove(lbuf, buf, c);
	}
}

static int
probeif(Dev* f, int ctlrno, int i, int kcsp, int csp, int, int)
{
	int found, n, sfd;
	char buf[256], cspstr[50], kcspstr[50];

	seprint(cspstr, cspstr + sizeof cspstr, "0x%0.06x", csp);
	seprint(buf, buf + sizeof buf, "/dev/usb%d/%d/status", ctlrno, i);
	sfd = open(buf, OREAD);
	if(sfd < 0)
		return -1;
	n = read(sfd, buf, sizeof buf - 1);
	if(n <= 0){
		close(sfd);
		return -1;
	}
	buf[n] = 0;
	close(sfd);
	/*
	 * Mouse + keyboard combos may report the interface as Kbdcsp,
	 * because it's the endpoint the one with the right csp.
	 */
	sprint(cspstr, "Enabled 0x%0.06x", csp);
	found = (strncmp(buf, cspstr, strlen(cspstr)) == 0);
	if(!found){
		sprint(cspstr, " 0x%0.06x", csp);
		sprint(kcspstr, "Enabled 0x%0.06x", kcsp);
		if(strncmp(buf, kcspstr, strlen(kcspstr)) == 0 &&
		   strstr(buf, cspstr) != nil)
			found = 1;
	}
	if(found){
		f->ctlrno = ctlrno;
		f->id = i;
		f->enabled = 1;
		if(hdebug)
			fprint(2, "%s: csp 0x%x at /dev/usb%d/%d\n",
				f->name, csp, f->ctlrno, f->id);
		return 0;
	}
	if(hdebug)
		fprint(2, "%s: not found %s\n", f->name, cspstr);
	return -1;

}

static int
probedev(Dev* f, int kcsp, int csp, int vid, int did)
{
	int c, i;

	for(c = 0; c < 16; c++)
		if(f->ctlrno == 0 || c == f->ctlrno)
			for(i = 1; i < 128; i++)
				if(f->id == 0 || i == f->id)
				if(probeif(f, c, i, kcsp, csp, vid, did) != -1){
					f->csp = csp;
					return 0;
				}
	f->enabled = 0;
	if(hdebug || verbose)
		fprint(2, "%s: csp 0x%x: not found\n", f->name, csp);
	return -1;
}

static void
initdev(Dev* f)
{
	int i;

	f->dev = opendev(f->ctlrno, f->id);
	if(f->dev == nil || describedevice(f->dev) < 0) {
		fprint(2, "init failed: %s: %r\n", f->name);
		f->enabled = 0;
		f->ctlrno = f->id = -1;
		return;
	}
	memset(f->dev->config, 0, sizeof f->dev->config);
	for(i = 0; i < f->dev->nconf; i++){
		f->dev->config[i] = mallocz(sizeof *f->dev->config[i], 1);
		loadconfig(f->dev, i);
	}
	if(verbose || hdebug){
		fprint(2, "%s found: ctlrno=%d id=%d\n",
			f->name, f->ctlrno, f->id);
//		printdevice(f->dev);	// TODO
	}
}

static int
setbootproto(Dev* f)
{
	int r, id;
	Endpt* ep;

	ep = f->dev->ep[0];
	r = RH2D | Rclass | Rinterface;
	id = f->dev->ep[f->ep]->iface->interface;
	return setupreq(ep, r, SET_PROTO, BOOT_PROTO, id, 0);
}

static void
startdev(Dev* f)
{
	int i;
	char buf[128];
	Endpt* ep;

	f->ep = -1;
	ep = nil;
	for(i = 0; i < Nendpt; i++)
		if((ep = f->dev->ep[i]) != nil &&
		    ep->csp == f->csp && ep->type == Eintr && ep->dir == Ein){
			f->ep = i;
			f->msz = ep->maxpkt;
			break;
		}
	if(ep == nil){
		fprint(2, "%s: %s: bug: no endpoint\n", argv0, f->name);
		return;
	}
	sprint(buf, "ep %d 10 r %d", f->ep, f->msz);
	if(hdebug)
		fprint(2, "%s: %s: ep %d: ctl %s\n", argv0, f->name, f->ep, buf);
	if(write(f->dev->ctl, buf, strlen(buf)) != strlen(buf)){
		fprint(2, "%s: %s: startdev: %r\n", argv0, f->name);
		return;
	}
	sprint(buf, "/dev/usb%d/%d/ep%ddata", f->ctlrno, f->id, f->ep);
	f->fd = open(buf, OREAD);
	if(f->fd < 0){
		fprint(2, "%s: opening %s: %s: %r", argv0, f->name, buf);
		return;
	}
	if(setbootproto(f) < 0)
		fprint(2, "%s: %s: setbootproto: %r\n", argv0, f->name);
	if(hdebug)
		fprint(2, "starting %s\n", f->name);
	proccreate(f->proc, f, 32*1024);
}

static void
usage(void)
{
	fprint(2, "usage: %s [-dkmn] [-a n] [ctlrno usbport]\n", argv0);
	threadexitsall("usage");
}

void
threadmain(int argc, char **argv)
{

	quotefmtinstall();
	usbfmtinit();

	pf.enabled = kf.enabled = 1;
	ARGBEGIN{
	case 'a':
		accel = strtol(EARGF(usage()), nil, 0);
		break;
	case 'd':
		hdebug++;
		usbdebug++;
		break;
	case 'k':
		kf.enabled = 1;
		pf.enabled = 0;
		break;
	case 'm':
		kf.enabled = 0;
		pf.enabled = 1;
		break;
	case 'n':
		dryrun = 1;
		break;
	default:
		usage();
	}ARGEND;

	switch(argc){
	case 0:
		break;
	case 2:
		pf.ctlrno = kf.ctlrno = atoi(argv[0]);
		pf.id = kf.id = atoi(argv[1]);
		break;
	default:
		usage();
	}
	threadnotify(robusthandler, 1);

	kf.name = "kbd";
	kf.proc = kbdwork;
	pf.name = "mouse";
	pf.proc = ptrwork;

	if(kf.enabled)
		probedev(&kf, KbdCSP, KbdCSP, 0, 0);
	if(kf.enabled)
		initdev(&kf);
	if(pf.enabled)
		probedev(&pf, KbdCSP, PtrCSP, 0, 0);
	if(pf.enabled)
		if(kf.enabled && pf.ctlrno == kf.ctlrno && pf.id == kf.id)
			pf.dev = kf.dev;
		else
			initdev(&pf);
	rfork(RFNOTEG);
	if(kf.enabled)
		startdev(&kf);
	if(pf.enabled)
		startdev(&pf);
	threadexits(nil);
}
