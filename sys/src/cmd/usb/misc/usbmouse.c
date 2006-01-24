#include <u.h>
#include <libc.h>
#include <thread.h>

int mousefd, ctlfd, mousein;

char hbm[]		= "Enabled 0x020103";
char *mouseinfile	= "/dev/mousein";
char *statfmt		= "/dev/usb%d/%d/status";
char *ctlfmt		= "/dev/usb%d/%d/ctl";
char *msefmt		= "/dev/usb%d/%d/ep1data";
char *ctl3str		= "ep 1 bulk r 3 32";
char *ctl5str		= "ep 1 bulk r 5 32";
char ctlfile[32];
char msefile[32];

int verbose;
int nofork;
int accel;
int scroll;
int maxacc = 3;
int debug;

void work(void *);

int
robusthandler(void*, char *s)
{
	if (debug) fprint(2, "inthandler: %s\n", s);
	return (s && (strstr(s, "interrupted")|| strstr(s, "hangup")));
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
	fprint(2, "usage: %s [-fsv] [-a accel] [ctlrno usbport]\n", argv0);
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	int ctlrno, i, sfd;
	char line[256];

	ARGBEGIN{
	case 's':
		scroll=1;
		break;
	case 'v':
		verbose=1;
		break;
	case 'f':
		nofork=1;
		break;
	case 'a':
		accel=strtol(EARGF(usage()), nil, 0);
		break;
	default:
		usage();
	}ARGEND

	switch (argc) {
	case 0:
		for (ctlrno = 0; ctlrno < 16; ctlrno++) {
			for (i = 0; i < 128; i++) {
				snprint(line, sizeof line, statfmt, ctlrno, i);
				sfd = open(line, OREAD);
				if (sfd < 0)
					break;
				if (read(sfd, line, strlen(hbm)) && strncmp(hbm, line, strlen(hbm)) == 0) {
					snprint(ctlfile, sizeof ctlfile, ctlfmt, ctlrno, i);
					snprint(msefile, sizeof msefile, msefmt, ctlrno, i);
					close(sfd);
					goto found;
				}
				close(sfd);
			}
		}
		threadexitsall("no mouse");
	found:
		break;
	case 2:
		ctlrno = atoi(argv[0]);
		i = atoi(argv[1]);
		snprint(ctlfile, sizeof ctlfile, ctlfmt, ctlrno, i);
		snprint(msefile, sizeof msefile, msefmt, ctlrno, i);
		break;
	default:
		usage();
	}

	if ((ctlfd = open(ctlfile, OWRITE)) < 0)
		sysfatal("%s: %r", ctlfile);
	if (verbose)
		fprint(2, "Send %s to %s\n", scroll?ctl5str:ctl3str, ctlfile);
	write(ctlfd, scroll?ctl5str:ctl3str, scroll?strlen(ctl5str):strlen(ctl3str));
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
work(void *){
	char buf[6];
	int x, y, buts;

	for (;;) {
		buts = 0;
		switch (robustread(mousefd, buf, scroll?5:3)) {
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
			fprint(mousein, "m%11d %11d %11d", x, y, buts | maptab[buf[0]&0x7]);
			break;
		case -1:
			sysfatal("read error: %r");
		}
	}
}
