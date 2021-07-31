/*
 * usbmouse - listen for usb mouse events and turn them into
 *	writes on /dev/mousein.
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include "usb.h"

typedef struct {
	int	epno;
	int	maxpkt;
	int	pollms;
} Mouseinfo;

void (*dprinter[])(Device *, int, ulong, void *b, int n) = {
	[STRING] pstring,
	[DEVICE] pdevice,
	[HID] phid,
};

int mousefd, ctlfd, mousein;

char hbm[]		= "0x020103";
char *mouseinfile	= "/dev/mousein";

char *statfmt		= "/dev/usb%d/%d/status";
char *ctlfmt		= "/dev/usb%d/%d/ctl";
char *msefmt		= "/dev/usb%d/%d/ep%ddata";

char *ctlmsgfmt		= "ep %d %d r %d";

char ctlfile[32];
char msefile[32];

int verbose;
int nofork;
int debug;

int accel;
int scroll;
int maxacc = 3;
int nbuts;

void work(void *);

int
findendpoint(int ctlr, int id, Mouseinfo *mp)
{
	int i;
	Device *d;
	Endpt *ep;

	d = opendev(ctlr, id);
	d->config[0] = emallocz(sizeof(*d->config[0]), 1);
	if (describedevice(d) < 0 || loadconfig(d, 0) < 0) {
		closedev(d);
		return -1;
	}
	for (i = 1; i < Nendpt; i++) {
		if ((ep = d->ep[i]) == nil)
			continue;
		if (ep->csp == 0x020103 && ep->type == Eintr && ep->dir != Eout) {
			if (ep->iface == nil || ep->iface->dalt[0] == nil)
				continue;
			mp->epno = i;
			mp->maxpkt = ep->maxpkt;
			mp->pollms = ep->iface->dalt[0]->interval;
			closedev(d);
			return 0;
		}
	}
	closedev(d);
	return -1;
}

int
robusthandler(void*, char *s)
{
	if (debug)
		fprint(2, "inthandler: %s\n", s);
	return s && (strstr(s, "interrupted") || strstr(s, "hangup"));
}

long
robustread(int fd, void *buf, long sz)
{
	long r;
	char err[ERRMAX];

	do {
		r = read(fd , buf, sz);
		if (r < 0)
			rerrstr(err, sizeof(err));
	} while (r < 0 && robusthandler(nil, err));
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
		x *= maxacc;
		break;
	}
	return sign*x;
}

char maptab[] = {
	0x0, 0x1, 0x4, 0x5, 0x2, 0x3, 0x6, 0x7
};

void
usage(void)
{
	fprint(2, "usage: %s [-dfsv] [-a accel] [ctlrno usbport]\n", argv0);
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	int ctlrno, i;
	char *p;
	char buf[256];
	Biobuf *f;
	Mouseinfo mouse;

	ARGBEGIN{
	case 'a':
		accel = strtol(EARGF(usage()), nil, 0);
		break;
	case 'd':
		debug = usbdebug = 1;
		break;
	case 'f':
		nofork = 1;
		break;
	case 's':
		scroll = 1;
		break;
	case 'v':
		verbose = 1;
		break;
	default:
		usage();
	}ARGEND

	memset(&mouse, 0, sizeof mouse);
	f = nil;
	switch (argc) {
	case 0:
		for (ctlrno = 0; ctlrno < 16; ctlrno++) {
			sprint(buf, "/dev/usb%d", ctlrno);
			if (access(buf, AEXIST) < 0)
				continue;
			for (i = 1; i < 128; i++) {
				snprint(buf, sizeof buf, statfmt, ctlrno, i);
				f = Bopen(buf, OREAD);
				if (f == nil)
					break;
				while ((p = Brdline(f, '\n')) != 0) {
					p[Blinelen(f)-1] = '\0';
					if (strncmp(p, "Enabled ", 8) == 0)
						continue;
					if (strstr(p, hbm) != nil) {
						Bterm(f);
						goto found;
					}
				}
				Bterm(f);
			}
		}
		threadexitsall("no mouse");
	case 2:
		ctlrno = atoi(argv[0]);
		i = atoi(argv[1]);
found:
		if(findendpoint(ctlrno, i, &mouse) < 0) {
			fprint(2, "%s: invalid usb device configuration\n",
				argv0);
			threadexitsall("no mouse");
		}
		snprint(ctlfile, sizeof ctlfile, ctlfmt, ctlrno, i);
		snprint(msefile, sizeof msefile, msefmt, ctlrno, i, mouse.epno);
		break;
	default:
		usage();
	}
	if (f)
		Bterm(f);

	nbuts = (scroll? 5: 3);
	if (nbuts > mouse.maxpkt)
		nbuts = mouse.maxpkt;
	if ((ctlfd = open(ctlfile, OWRITE)) < 0)
		sysfatal("%s: %r", ctlfile);
	if (verbose)
		fprint(2, "Send mouse.ep %d %d r %d to %s\n",
			mouse.epno, mouse.pollms, mouse.maxpkt, ctlfile);
	fprint(ctlfd, ctlmsgfmt, mouse.epno, mouse.pollms, mouse.maxpkt);
	close(ctlfd);

	if ((mousefd = open(msefile, OREAD)) < 0)
		sysfatal("%s: %r", msefile);
	if (verbose)
		fprint(2, "Start reading from %s\n", msefile);
	if ((mousein = open(mouseinfile, OWRITE)) < 0)
		sysfatal("%s: %r", mouseinfile);
	atnotify(robusthandler, 1);
	if (nofork)
		work(nil);
	else
		proccreate(work, nil, 4*1024);
	threadexits(nil);
}

void
work(void *)
{
	char buf[6];
	int x, y, buts;

	for (;;) {
		buts = 0;
		switch (robustread(mousefd, buf, nbuts)) {
		case 4:
			if(buf[3] == 1)
				buts |= 0x08;
			else if(buf[3] == -1)
				buts |= 0x10;
			/* Fall through */
		case 5:
			if(buf[3] > 10)
				buts |= 0x08;
			else if(buf[3] < -10)
				buts |= 0x10;
			/* Fall through */
		case 3:
			if (accel) {
				x = scale(buf[1]);
				y = scale(buf[2]);
			} else {
				x = buf[1];
				y = buf[2];
			}
			fprint(mousein, "m%11d %11d %11d",
				x, y, buts | maptab[buf[0]&0x7]);
			break;
		case -1:
			sysfatal("read error: %r");
		}
	}
}
