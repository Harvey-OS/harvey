#include <u.h>
#include <libc.h>
#include <thread.h>
#include <stdio.h>

int mousefd, ctlfd, mousein;

char hbm[]			= "Enabled 0x020103\n";
char *mouseinfile	= "#m/mousein";
char *statfmt		= "#U/usb%d/%d/status";
char *ctlfmt		= "#U/usb%d/%d/ctl";
char *msefmt		= "#U/usb%d/%d/ep1data";
char *ctlstr		= "ep 1 bulk r 3 32";
char ctlfile[32];
char msefile[32];

int verbose;
int nofork;
int accel;
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
	fprint(2, "usage: %s [-vf] [-a accel] [ctlrno usbport]\n", argv0);
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	FILE *f;
	int ctlrno, i;
	char line[256];

	rfork(RFNOTEG);

	ARGBEGIN{
	case 'v':
		verbose=1;
		break;
	case 'f':
		nofork=1;
		break;
	case 'a':
		accel=strtol(ARGF(), nil, 0);
		break;
	default:
		usage();
	}ARGEND

	switch (argc) {
	case 0:
		for (ctlrno = 0; ctlrno < 16; ctlrno++) {
			for (i = 0; i < 128; i++) {
				sprint(line, statfmt, ctlrno, i);
				f = fopen(line, "r");
				if (f == nil)
					break;
				if (fgets(line, sizeof line, f) && strcmp(hbm, line) == 0) {
					snprint(ctlfile, sizeof ctlfile, ctlfmt, ctlrno, i);
					snprint(msefile, sizeof msefile, msefmt, ctlrno, i);
					fclose(f);
					goto found;
				}
				fclose(f);
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
		fprint(2, "Send %s to %s\n", ctlstr, ctlfile);
	write(ctlfd, ctlstr, strlen(ctlstr));
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
	char buf[4];
	int x, y;

	for (;;) {
		switch (robustread(mousefd, buf, 3)) {
		case 3:
			if (accel) {
				x = scale(buf[1]);
				y = scale(buf[2]);
			} else {
				x = buf[1];
				y = buf[2];
			}
			fprint(mousein, "m%11d %11d %11d", x, y, maptab[buf[0]&0x7]);
			break;
		case -1:
			sysfatal("read error: %r");
		}
	}
}
