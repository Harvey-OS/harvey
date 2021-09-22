/*
 * Receive tcp just for bandwidth testing, much like ttcp.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>

#define NS(x)	((vlong)(x))
#define US(x)	(NS(x) * 1000LL)
#define MS(x)	(US(x) * 1000LL)
#define S(x)	(MS(x) * 1000LL)

uvlong tv0;		/* start time */
uvlong now, tt;

uint ms;		/* milliseconds since start */
double total;		/* total bytes received */
uint lastms;
double lasttotal;

int alarmhappened;

int quiet = 1;
int dumpflag;
int interval = 1000;	/* -i, milliseconds between b/w prints */
int fflag;		/* -f, output suitable for gnuplot */
int readsize;
int lfd = -1;

int	gotalarm(void *, char *);
void 	stats(void);
void	timestamp(vlong offs, uchar *p, int nb, vlong *);

static int nsecfd = -1;

uvlong
rdnsec(void)
{
	char buf[32];

	if(nsecfd < 0) {
		nsecfd = open("/dev/nsec", OREAD);
		if(nsecfd < 0)
			sysfatal("open /dev/nsec: %r");
	}
	seek(nsecfd, 0, 0);
	read(nsecfd, buf, sizeof buf);
	return strtoll(buf, nil, 0);
}

static char client_host_ip[64];

static int
getclientip(char *tcpdir)
{
	int fd, n;
	char fn[128];

	snprint(fn, sizeof(fn), "%s/%s", tcpdir, "remote");
	if ((fd = open(fn, OREAD)) < 0)
		sysfatal("open remote: %r");

	if ((n = read(fd, client_host_ip, 31)) <= 0)
		sysfatal("read remote");

	client_host_ip[n] = 0;
	return 1;
}

void
usage(void)
{
	fprint(2, "usage: %s [-dfqtv] [-a announce-str] [-i secs] [-L logfile] "
		"[-l readsize] [-T secs]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int cfd, pagesize, s, i, cc, touchflag = 0, seconds = -1;
	char *logfile, *p, *srcaddr = "*";
	char adir[40], ldir[40];
	vlong offs, µs;

	pagesize = readsize = 8192;

	ARGBEGIN {
	case 'a':
		srcaddr = EARGF(usage());
		break;
	case 'd':
		dumpflag++;
		quiet = 1;
		break;
	case 'f':
		fflag = 1;
		break;
	case 'i':
		interval = atof(EARGF(usage())) * 1000;
		break;
	case 'l':
		readsize = atoi(EARGF(usage()));
		break;
	case 'L':
		logfile = EARGF(usage());
		if ((lfd = create(logfile, OWRITE, 0644)) < 0)
			sysfatal("%s: Cannot create %s: %r", argv[0], logfile);
		break;
	case 'q':
		quiet = 1;
		break;
	case 't':
		touchflag = 1;
		break;
	case 'T':
		seconds = atoi(EARGF(usage()));
		break;
	case 'v':
		quiet = 0;
		break;
	default:
		usage();
	}ARGEND;

	p = malloc(readsize);
	if(p == nil)
		sysfatal("malloc: %r");

	if (announce(netmkaddr(srcaddr, "tcp", "ttcp"), adir) < 0)
		sysfatal("announce: %r");

	if ((cfd = listen(adir, ldir)) < 0)
		sysfatal("listen: %r");

	if ((s = accept(cfd, ldir)) < 0)
		sysfatal("accept: %r");

	if(quiet == 0)
		if(client_host_ip[0])
			print("connected from %s: ", client_host_ip);
		else
			print("connected: ");

	/*  tv0 = rdnsec(); */
	tv0 = nsec();
	lasttotal = total = 0;
	atnotify(gotalarm, 1);
	alarm(interval);

	offs = 0;
	while(seconds == -1 || ms < seconds * 1000){
		vlong tt, curts;

		if(alarmhappened)
			stats();
		cc = read(s, p, readsize);
		if(cc == 0)
			break;
		if(cc < 0){
			if(alarmhappened)
				continue;
			sysfatal("recv: %r");
		}
		if (dumpflag){
			if (write(1, p, cc) != cc)
				sysfatal("write error");
		}else if(0){
			curts = nsec();
			timestamp(offs, (uchar *)p, cc, &tt);
			if (lfd >= 0)
				fprint(lfd, "%lld %lld %d\n", curts, tt, cc);
		}
		if(touchflag)
			for(i = 0; i < cc - 1; i += pagesize)
				p[i] = p[i+1];
		offs += cc;
		total += cc;
	}

	now = nsec();
	µs = (now - tv0)/1000;
	if (!quiet)
		print("\n");
	print("%g / %,lld µs. = %g MB/s.\n", total, µs, total/µs);
	exits(0);
}

int
gotalarm(void *, char * sig)
{
	if(strstr(sig, "alarm") == nil)
		return 0;
	alarmhappened = 1;
	return 1;
}

void
stats(void)
{
	static char *bs = "";

	now = nsec();
	ms = (now - tv0)/1000000;

	if(quiet == 0)
		if(fflag)
			fprint(2, "%f %d\n", ((double)ms)/1000.0,
				ms > lastms? (int)((total-lasttotal)*8 /
					(ms-lastms)): 0);
		else {
			if(ms == lastms)
				sysfatal("whoops: %d %d", ms, lastms);
			fprint(2, "%s%7d %7d", bs,
				(int)((total-lasttotal)*8/(ms-lastms)),
				(int)(total*8/ms));
			bs = "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";
		}

	alarmhappened = 0;
	alarm(interval);

	lastms = ms;
	lasttotal = total;
}

void
timestamp(vlong offs, uchar *p, int nb, vlong *tt)
{
	static vlong nextoffs;

	*tt = 0;
	if (offs <= nextoffs && offs + nb > nextoffs) {
		if (offs + nb < nextoffs + sizeof(vlong)) {
			/* Too hard, I would need to think. */
			nextoffs += readsize;
			return;
		}

		memcpy(tt, p + (int)(nextoffs - offs), sizeof(vlong));
		nextoffs += readsize;
	}
}
