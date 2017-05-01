/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <auth.h>
#include <ip.h>
#include <mp.h>

/* nanosecond times */
#define SEC 1000000000LL
#define MIN (60LL*SEC)
#define HOUR (60LL*MIN)
#define DAY (24LL*HOUR)

enum {
	Fs,
	Rtc,
	Ntp,
	Utc,
	Gps,

	HZAvgSecs= 3*60,  /* target averaging period for frequency in seconds */
	MinSampleSecs= 60,	/* minimum sampling time in seconds */
};


char *dir = "/tmp";	/* directory sample files live in */
char *logfile = "timesync";
char *timeserver;
char *Rootid;
int utcfil;
int gpsfil;
int debug;
int impotent;
int logging;
int type;
int gmtdelta;		/* rtc+gmtdelta = gmt */
uint64_t avgerr;

/* ntp server info */
int stratum = 14;
int64_t mydisp, rootdisp;
int64_t mydelay, rootdelay;
int64_t avgdelay;
int64_t lastutc;
uint8_t rootid[4];
char *sysid;
int myprec;

/* list of time samples */
typedef struct Sample Sample;
struct Sample
{
	Sample	*next;
	uint64_t	ticks;
	int64_t	ltime;
	int64_t	stime;
};

/* ntp packet */
typedef struct NTPpkt NTPpkt;
struct NTPpkt
{
	uint8_t	mode;
	uint8_t	stratum;
	uint8_t	poll;
	uint8_t	precision;
	uint8_t	rootdelay[4];
	uint8_t	rootdisp[4];
	uint8_t	rootid[4];
	uint8_t	refts[8];
	uint8_t	origts[8];	/* departed client */
	uint8_t	recvts[8];	/* arrived at server */
	uint8_t	xmitts[8];	/* departed server */
	uint8_t	keyid[4];
	uint8_t	digest[16];
};

/* ntp server */
typedef struct NTPserver NTPserver;
struct NTPserver
{
	NTPserver *next;
	char	*name;
	uint8_t	stratum;
	uint8_t	precision;
	int64_t	rootdelay;
	int64_t	rootdisp;
	int64_t	rtt;
	int64_t	dt;
};

NTPserver *ntpservers;

enum
{
	NTPSIZE= 	48,	/* basic ntp packet */
	NTPDIGESTSIZE=	20,	/* key and digest */
};

/* error bound of last sample */
uint32_t	etha;

static void	addntpserver(char *name);
static int	adjustperiod(int64_t diff, int64_t accuracy, int secs);
static void	background(void);
static int	caperror(int64_t dhz, int tsecs, int64_t taccuracy);
static int32_t	fstime(void);
static int	gettime(int64_t *nsec, uint64_t *ticks, uint64_t *hz); /* returns time, ticks, hz */
static int	getclockprecision(int64_t);
static int64_t	gpssample(void);
static void	hnputts(void *p, int64_t nsec);
static void	hnputts(void *p, int64_t nsec);
static void	inittime(void);
static int64_t	nhgetts(void *p);
static int64_t	nhgetts(void *p);
static void	ntpserver(char*);
static int64_t	ntpsample(void);
static int	ntptimediff(NTPserver *ns);
static int	openfreqfile(void);
static int64_t	readfreqfile(int fd, int64_t ohz, int64_t minhz,
				   int64_t maxhz);
static int32_t	rtctime(void);
static int64_t	sample(int32_t (*get)(void));
static void	setpriority(void);
static void	setrootid(char *d);
static void	settime(int64_t now, uint64_t hz, int64_t delta, int n); /* set time, hz, delta, period */
static int64_t	utcsample(void);
static uint64_t	vabs(int64_t);
static uint64_t	whatisthefrequencykenneth(uint64_t hz, uint64_t minhz,
						 uint64_t maxhz,
			int64_t dt, int64_t ticks, int64_t period);
static void	writefreqfile(int fd, int64_t hz, int secs, int64_t diff);

// ((1970-1900)*365 + 17 /*leap days*/)*24*60*60
#define EPOCHDIFF 2208988800UL

static void
usage(void)
{
	fprint(2, "usage: %s [-a accuracy][-d dir][-I rootid][-s net]"
		"[-S stratum][-DfGilLnrU] timesource ...\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i, t, fd, nservenet;
	int secs;		/* sampling period */
	int tsecs;		/* temporary sampling period */
	uint64_t hz, minhz, maxhz, period, nhz;
	int64_t diff, accuracy, taccuracy;
	char *servenet[4];
	Sample *s, *x, *first, **l;
	Tm tl, tg;

	type = Fs;		/* by default, sync with the file system */
	debug = 0;
	accuracy = 1000000LL;	/* default accuracy is 1 millisecond */
	nservenet = 0;
	tsecs = secs = MinSampleSecs;
	timeserver = "";

	ARGBEGIN{
	case 'a':
		accuracy = strtoll(EARGF(usage()), 0, 0); /* specified in ns */
		if(accuracy <= 1)
			sysfatal("bad accuracy specified");
		break;
	case 'd':
		dir = EARGF(usage());
		break;
	case 'D':
		debug = 1;
		break;
	case 'f':
		type = Fs;
		stratum = 2;
		break;
	case 'G':
		type = Gps;
		stratum = 1;
		break;
	case 'i':
		impotent = 1;
		break;
	case 'I':
		Rootid = EARGF(usage());
		break;
	case 'l':
		logging = 1;
		break;
	case 'L':
		/*
		 * Assume time source in local time rather than GMT.
		 * Calculate difference so that rtctime can return GMT.
		 * This is useful with the rtc on PC's that run Windows
		 * since Windows keeps the local time in the rtc.
		 */
		t = time(0);
		tl = *localtime(t);
		tg = *gmtime(t);

		/*
		 * if the years are different, we're at most a day off,
		 * so just rewrite
		 */
		if(tl.year < tg.year){
			tg.year--;
			tg.yday = tl.yday + 1;
		}else if(tl.year > tg.year){
			tl.year--;
			tl.yday = tg.yday+1;
		}
		assert(tl.year == tg.year);

		tg.sec -= tl.sec;
		tg.min -= tl.min;
		tg.hour -= tl.hour;
		tg.yday -= tl.yday;
		gmtdelta = tg.sec+60*(tg.min+60*(tg.hour+tg.yday*24));

		assert(abs(gmtdelta) <= 24*60*60);
		break;
	case 'n':
		type = Ntp;
		break;
	case 'r':
		type = Rtc;
		stratum = 0;
		break;
	case 'U':
		type = Utc;
		stratum = 1;
		break;
	case 's':
		if(nservenet >= nelem(servenet))
			sysfatal("too many networks to serve on");
		servenet[nservenet++] = EARGF(usage());
		break;
	case 'S':
		stratum = strtoll(EARGF(usage()), 0, 0);
		break;
	default:
		usage();
	}ARGEND;

	fmtinstall('E', eipfmt);
	fmtinstall('I', eipfmt);
	fmtinstall('V', eipfmt);
	sysid = getenv("sysname");

	/* detach from the current namespace */
	if(debug)
		rfork(RFNAMEG);

	switch(type){
	case Utc:
		if(argc > 0)
			timeserver = argv[0];
		else
			sysfatal("bad time source");
		break;
	case Gps:
		if(argc > 0)
			timeserver = argv[0];
		else
			timeserver = "/mnt/gps/time";
		break;
	case Fs:
		if(argc > 0)
			timeserver = argv[0];
		else
			timeserver = "/srv/boot";
		break;
	case Ntp:
		if(argc > 0)
			for(i = 0; i < argc; i++)
				addntpserver(argv[i]);
		else
			addntpserver("$ntp");
		break;
	}

	setpriority();

	/* figure out our time interface and initial frequency */
	inittime();
	gettime(0, 0, &hz);
	minhz = hz/10;
	maxhz = hz*10;
	myprec = getclockprecision(hz);

	/* convert the accuracy from nanoseconds to ticks */
	taccuracy = hz*accuracy/SEC;

	/*
	 * bind in clocks
	 */
	switch(type){
	case Fs:
		fd = open(timeserver, ORDWR);
		if(fd < 0)
			sysfatal("opening %s: %r", timeserver);
		if(amount(fd, "/n/boot", MREPL, "") < 0)
			sysfatal("mounting %s: %r", timeserver);
		close(fd);
		break;
	case Rtc:
		bind("#r", "/dev", MAFTER);
		if(access("/dev/rtc", AREAD) < 0)
			sysfatal("accessing /dev/rtc: %r");
		break;
	case Utc:
		fd = open(timeserver, OREAD);
		if(fd < 0)
			sysfatal("opening %s: %r", timeserver);
		utcfil = fd;
		break;
	case Gps:
		fd = open(timeserver, OREAD);
		if(fd < 0)
			sysfatal("opening %s: %r", timeserver);
		gpsfil = fd;
		break;
	}

	/*
	 * start a local ntp server(s)
	 */
	for(i = 0; i < nservenet; i++)
		switch(rfork(RFPROC|RFFDG|RFMEM|RFNOWAIT)){
		case -1:
			sysfatal("forking: %r");
		case 0:
			ntpserver(servenet[i]);
			_exits(0);
		}

	/* get the last known frequency from the file */
	fd = openfreqfile();
	hz = readfreqfile(fd, hz, minhz, maxhz);

	/*
	 * this is the main loop.  it gets a sample, adjusts the
	 * clock and computes a sleep period until the next loop.
	 * we balance frequency drift against the length of the
	 * period to avoid blowing the accuracy limit.
	 */
	first = nil;
	l = &first;
	avgerr = accuracy >> 1;
	for(;; background(), sleep(tsecs*1000)){
		s = mallocz(sizeof *s, 1);
		diff = 0;

		/* get times for this sample */
		etha = ~0;
		switch(type){
		case Fs:
			s->stime = sample(fstime);
			break;
		case Rtc:
			s->stime = sample(rtctime);
			break;
		case Utc:
			s->stime = utcsample();
			if(s->stime == 0LL){
				if(logging)
					syslog(0, logfile, "no sample");
				free(s);
				if (secs > 60 * 15)
					tsecs = 60*15;
				continue;
			}
			break;
		case Ntp:
			diff = ntpsample();
			if(diff == 0LL){
				if(logging)
					syslog(0, logfile, "no sample");
				free(s);
				if(secs > 60*15)
					tsecs = 60*15;
				continue;
			}
			break;
		case Gps:
			diff = gpssample();
			if(diff == 0LL){
				if(logging)
					syslog(0, logfile, "no sample");
				free(s);
				if(secs > 60*15)
					tsecs = 60*15;
				continue;
			}
		}

		/* use fastest method to read local clock and ticks */
		gettime(&s->ltime, &s->ticks, 0);
		if(type == Ntp || type == Gps)
			s->stime = s->ltime + diff;

		/* if the sample was bad, ignore it */
		if(s->stime < 0){
			free(s);
			continue;
		}

		/* reset local time */
		diff = s->stime - s->ltime;
		if(diff > 10*SEC || diff < -10*SEC){
			/* we're way off, just set the time */
			secs = MinSampleSecs;
			settime(s->stime, 0, 0, 0);
		} else {
			/* keep a running average of the error. */
			avgerr = (avgerr>>1) + (vabs(diff)>>1);

			/*
			 * the time to next sample depends on how good or
			 * bad we're doing.
			 */
			tsecs = secs = adjustperiod(diff, accuracy, secs);

			/*
			 * work off the fixed difference.  This is done
			 * by adding a ramp to the clock.  Each 100th of a
			 * second (or so) the kernel will add diff/(4*secs*100)
			 * to the clock.  we only do 1/4 of the difference per
			 * period to dampen any measurement noise.
			 *
			 * any difference greater than etha we work off during the
			 * sampling period.
			 */
			if(abs(diff) > etha)
				if(diff > 0)
					settime(-1, 0, diff-((3*etha)/4), secs);
				else
					settime(-1, 0, diff+((3*etha)/4), secs);
			else
				settime(-1, 0, diff, 4*secs);

		}
		if(debug)
			fprint(2, "δ %lld avgδ %lld f %lld\n", diff, avgerr, hz);

		/* dump old samples (keep at least one) */
		while(first != nil){
			if(first->next == nil)
				break;
			if(s->stime - first->next->stime < DAY)
				break;
			x = first;
			first = first->next;
			free(x);
		}

		/*
		 * The sampling error is limited by the total error.  If
		 * we make sure the sampling period is at least 16 million
		 * times the average error, we should calculate a frequency
		 * with on average a 1e-7 error.
		 *
		 * So that big hz changes don't blow our accuracy requirement,
		 * we shorten the period to make sure that δhz*secs will be
		 * greater than the accuracy limit.
		 */
		period = avgerr << 24;
		for(x = first; x != nil; x = x->next)
			if(s->stime - x->stime < period ||
			   x->next == nil || s->stime - x->next->stime < period)
				break;
		if(x != nil){
			nhz = whatisthefrequencykenneth(
				hz, minhz, maxhz,
				s->stime - x->stime,
				s->ticks - x->ticks,
				period);
			tsecs = caperror(vabs(nhz-hz), tsecs, taccuracy);
			hz = nhz;
			writefreqfile(fd, hz, (s->stime - x->stime)/SEC, diff);
		}

		/* add current sample to list. */
		*l = s;
		l = &s->next;

		if(logging)
			syslog(0, logfile, "δ %lld avgδ %lld hz %lld",
				diff, avgerr, hz);
	}
}

/*
 * adjust the sampling period with some histeresis
 */
static int
adjustperiod(int64_t diff, int64_t accuracy, int secs)
{
	uint64_t absdiff;

	absdiff = vabs(diff);

	if(absdiff < (accuracy>>1))
		secs += 60;
	else if(absdiff > accuracy)
		secs >>= 1;
	else
		secs -= 60;
	if(secs < MinSampleSecs)
		secs = MinSampleSecs;
	return secs;
}

/*
 * adjust the frequency
 */
static uint64_t
whatisthefrequencykenneth(uint64_t hz, uint64_t minhz, uint64_t maxhz,
			  int64_t dt,
	int64_t ticks, int64_t period)
{
	uint64_t ohz = hz;
	static mpint *mpdt, *mpticks, *mphz, *mpbillion;

	/* sanity check */
	if(dt <= 0 || ticks <= 0)
		return hz;

	if(mphz == nil){
		mphz = mpnew(0);
		mpbillion = uvtomp(SEC, nil);
	}

	/* hz = (ticks*SEC)/dt */
	mpdt = vtomp(dt, mpdt);
	mpticks = vtomp(ticks, mpticks);
	mpmul(mpticks, mpbillion, mpticks);
	mpdiv(mpticks, mpdt, mphz, nil);
	hz = mptoui(mphz);

	/* sanity */
	if(hz < minhz || hz > maxhz)
		return ohz;

	/* damp the change if we're shorter than the target period */
	if(period > dt)
		hz = (12ULL*ohz + 4ULL*hz)/16ULL;

	settime(-1, hz, 0, 0);
	return hz;
}

/*
 * We may be changing the frequency to match a bad measurement
 * or to match a condition no longer in effect.  To make sure
 * that this doesn't blow our error budget over the next measurement
 * period, shorten the period to make sure that δhz*secs will be
 * less than the accuracy limit.  Here taccuracy is accuracy converted
 * from nanoseconds to ticks.
 */
static int
caperror(int64_t dhz, int tsecs, int64_t taccuracy)
{
	if(dhz*tsecs <= taccuracy)
		return tsecs;

	if(debug)
		fprint(2, "δhz %lld tsecs %d tacc %lld\n", dhz, tsecs, taccuracy);

	tsecs = taccuracy/dhz;
	if(tsecs < MinSampleSecs)
		tsecs = MinSampleSecs;
	return tsecs;
}

/*
 *  kernel interface
 */
enum
{
	Ibintime,
	Insec,
	Itiming,
};
int ifc;
int bintimefd = -1;
int timingfd = -1;
int nsecfd = -1;
int fastclockfd = -1;

static void
inittime(void)
{
	int mode;

	if(impotent)
		mode = OREAD;
	else
		mode = ORDWR;

	/* bind in clocks */
	if(access("/dev/time", 0) < 0)
		bind("#c", "/dev", MAFTER);
	if(access("/dev/rtc", 0) < 0)
		bind("#r", "/dev", MAFTER);

	/* figure out what interface we have */
	ifc = Ibintime;
	bintimefd = open("/dev/bintime", mode);
	if(bintimefd >= 0)
		return;
	ifc = Insec;
	nsecfd = open("/dev/nsec", mode);
	if(nsecfd < 0)
		sysfatal("opening /dev/nsec");
	fastclockfd = open("/dev/fastclock", mode);
	if(fastclockfd < 0)
		sysfatal("opening /dev/fastclock");
	timingfd = open("/dev/timing", OREAD);
	if(timingfd < 0)
		return;
	ifc = Itiming;
}

/*
 *  convert binary numbers from/to kernel
 */
static uint64_t uvorder = 0x0001020304050607ULL;

static uint8_t*
be2int64_t(int64_t *to, uint8_t *f)
{
	uint8_t *t, *o;
	int i;

	t = (uint8_t*)to;
	o = (uint8_t*)&uvorder;
	for(i = 0; i < sizeof(int64_t); i++)
		t[o[i]] = f[i];
	return f+sizeof(int64_t);
}

static uint8_t*
int64_t2be(uint8_t *t, int64_t from)
{
	uint8_t *f, *o;
	int i;

	f = (uint8_t*)&from;
	o = (uint8_t*)&uvorder;
	for(i = 0; i < sizeof(int64_t); i++)
		t[i] = f[o[i]];
	return t+sizeof(int64_t);
}

static int32_t order = 0x00010203;

static uint8_t*
long2be(uint8_t *t, int32_t from)
{
	uint8_t *f, *o;
	int i;

	f = (uint8_t*)&from;
	o = (uint8_t*)&order;
	for(i = 0; i < sizeof(int32_t); i++)
		t[i] = f[o[i]];
	return t+sizeof(int32_t);
}

/*
 * read ticks and local time in nanoseconds
 */
static int
gettime(int64_t *nsec, uint64_t *ticks, uint64_t *hz)
{
	int i, n;
	uint8_t ub[3*8], *p;
	char b[2*24+1];

	switch(ifc){
	case Ibintime:
		n = sizeof(int64_t);
		if(hz != nil)
			n = 3*sizeof(int64_t);
		if(ticks != nil)
			n = 2*sizeof(int64_t);
		i = read(bintimefd, ub, n);
		if(i != n)
			break;
		p = ub;
		if(nsec != nil)
			be2int64_t(nsec, ub);
		p += sizeof(int64_t);
		if(ticks != nil)
			be2int64_t((int64_t*)ticks, p);
		p += sizeof(int64_t);
		if(hz != nil)
			be2int64_t((int64_t*)hz, p);
		return 0;
	case Itiming:
		n = sizeof(int64_t);
		if(ticks != nil)
			n = 2*sizeof(int64_t);
		i = read(timingfd, ub, n);
		if(i != n)
			break;
		p = ub;
		if(nsec != nil)
			be2int64_t(nsec, ub);
		p += sizeof(int64_t);
		if(ticks != nil)
			be2int64_t((int64_t*)ticks, p);
		if(hz != nil){
			seek(fastclockfd, 0, 0);
			n = read(fastclockfd, b, sizeof(b)-1);
			if(n <= 0)
				break;
			b[n] = 0;
			*hz = strtoll(b+24, 0, 0);
		}
		return 0;
	case Insec:
		if(nsec != nil){
			seek(nsecfd, 0, 0);
			n = read(nsecfd, b, sizeof(b)-1);
			if(n <= 0)
				break;
			b[n] = 0;
			*nsec = strtoll(b, 0, 0);
		}
		if(ticks != nil){
			seek(fastclockfd, 0, 0);
			n = read(fastclockfd, b, sizeof(b)-1);
			if(n <= 0)
				break;
			b[n] = 0;
			*ticks = strtoll(b, 0, 0);
		}
		if(hz != nil){
			seek(fastclockfd, 0, 0);
			n = read(fastclockfd, b, sizeof(b)-1);
			if(n <= 24)
				break;
			b[n] = 0;
			*hz = strtoll(b+24, 0, 0);
		}
		return 0;
	}
	return -1;
}

static void
settime(int64_t now, uint64_t hz, int64_t delta, int n)
{
	uint8_t b[1+sizeof(int64_t)+sizeof(int32_t)], *p;

	if(debug)
		fprint(2, "settime(now=%lld, hz=%llu, delta=%lld, period=%d)\n",
			now, hz, delta, n);
	if(impotent)
		return;
	switch(ifc){
	case Ibintime:
		if(now >= 0){
			p = b;
			*p++ = 'n';
			p = int64_t2be(p, now);
			if(write(bintimefd, b, p-b) < 0)
				sysfatal("writing /dev/bintime: %r");
		}
		if(delta != 0){
			p = b;
			*p++ = 'd';
			p = int64_t2be(p, delta);
			p = long2be(p, n);
			if(write(bintimefd, b, p-b) < 0)
				sysfatal("writing /dev/bintime: %r");
		}
		if(hz != 0){
			p = b;
			*p++ = 'f';
			p = int64_t2be(p, hz);
			if(write(bintimefd, b, p-b) < 0)
				sysfatal("writing /dev/bintime: %r");
		}
		break;
	case Itiming:
	case Insec:
		seek(nsecfd, 0, 0);
		if(now >= 0 || delta != 0){
			if(fprint(nsecfd, "%lld %lld %d", now, delta, n) < 0)
				sysfatal("writing /dev/nsec: %r");
		}
		if(hz > 0){
			seek(fastclockfd, 0, 0);
			if(fprint(fastclockfd, "%lld", hz) < 0)
				sysfatal("writing /dev/fastclock: %r");
		}
	}
}

/*
 *  set priority high and wire process to a processor
 */
static void
setpriority(void)
{
	int fd;
	char buf[32];

	sprint(buf, "/proc/%d/ctl", getpid());
	fd = open(buf, ORDWR);
	if(fd < 0){
		fprint(2, "can't set priority\n");
		return;
	}
	if(fprint(fd, "pri 100") < 0)
		fprint(2, "can't set priority\n");
	if(fprint(fd, "wired 2") < 0)
		fprint(2, "can't wire process\n");
	close(fd);
}

/* convert to ntp timestamps */
static void
hnputts(void *p, int64_t nsec)
{
	uint8_t *a;
	uint32_t tsh, tsl;

	a = p;

	/* zero is a special case */
	if(nsec == 0)
		return;

	tsh = nsec/SEC;
	nsec -= tsh*SEC;
	tsl = (nsec<<32)/SEC;
	hnputl(a, tsh+EPOCHDIFF);
	hnputl(a+4, tsl);
}

/* convert from ntp timestamps */
static int64_t
nhgetts(void *p)
{
	uint8_t *a;
	uint32_t tsh, tsl;
	int64_t nsec;

	a = p;
	tsh = nhgetl(a);
	tsl = nhgetl(a+4);
	nsec = tsl*SEC;
	nsec >>= 32;
	nsec += (tsh - EPOCHDIFF)*SEC;
	return nsec;
}

/* convert to ntp 32 bit fixed point */
static void
hnputfp(void *p, int64_t nsec)
{
	uint8_t *a;
	uint32_t fp;

	a = p;

	fp = nsec/(SEC/((int64_t)(1<<16)));
	hnputl(a, fp);
}

/* convert from ntp fixed point to nanosecs */
static int64_t
nhgetfp(void *p)
{
	uint8_t *a;
	uint32_t fp;
	int64_t nsec;

	a = p;
	fp = nhgetl(a);
	nsec = ((int64_t)fp)*(SEC/((int64_t)(1<<16)));
	return nsec;
}

/* get network address of the server */
static void
setrootid(char *d)
{
	char buf[128];
	int fd, n;
	char *p;

	snprint(buf, sizeof buf, "%s/remote", d);
	fd = open(buf, OREAD);
	if(fd < 0)
		return;
	n = read(fd, buf, sizeof buf);
	close(fd);
	if(n <= 0)
		return;
	p = strchr(buf, '!');
	if(p != nil)
		*p = 0;
	v4parseip(rootid, buf);
}

static void
ding(void *v, char *s)
{
	if(strstr(s, "alarm") != nil)
		noted(NCONT);
	noted(NDFLT);
}

static void
addntpserver(char *name)
{
	NTPserver *ns, **l;

	ns = mallocz(sizeof(NTPserver), 1);
	if(ns == nil)
		sysfatal("addntpserver: %r");
	timeserver = strdup(name);
	ns->name = name;
	for(l = &ntpservers; *l != nil; l = &(*l)->next)
		;
	*l = ns;
}

/*
 *  sntp client, we keep calling if the delay seems
 *  unusually high, i.e., 30% longer than avg.
 */
static int
ntptimediff(NTPserver *ns)
{
	int fd, tries, n;
	NTPpkt ntpin, ntpout;
	int64_t dt, recvts, origts, xmitts, destts, x;
	char dir[64];
	static int whined;

	notify(ding);
	alarm(30*1000);	/* don't wait forever if ns->name is unreachable */
	fd = dial(netmkaddr(ns->name, "udp", "ntp"), 0, dir, 0);
	alarm(0);
	if(fd < 0){
		if (!whined++)
			syslog(0, logfile, "can't reach %s: %r", ns->name);
		return -1;
	}
	setrootid(dir);

	memset(&ntpout, 0, sizeof(ntpout));
	ntpout.mode = 3 | (3 << 3);

	for(tries = 0; tries < 3; tries++){
		alarm(2*1000);

		gettime(&x, 0, 0);
		hnputts(ntpout.xmitts, x);
		if(write(fd, &ntpout, NTPSIZE) < 0){
			alarm(0);
			continue;
		}

		n = read(fd, &ntpin, sizeof ntpin);
		alarm(0);
		gettime(&destts, 0, 0);
		if(n >= NTPSIZE){
			close(fd);

			/* we got one, use it */
			recvts = nhgetts(ntpin.recvts);
			origts = nhgetts(ntpin.origts);
			xmitts = nhgetts(ntpin.xmitts);
			dt = ((recvts - origts) + (xmitts - destts))/2;

			/* save results */
			ns->rtt = ((destts - origts) - (xmitts - recvts))/2;
			ns->dt = dt;
			ns->stratum = ntpin.stratum;
			ns->precision = ntpin.precision;
			ns->rootdelay = nhgetfp(ntpin.rootdelay);
			ns->rootdisp = nhgetfp(ntpin.rootdisp);

			if(debug)
				fprint(2, "ntp %s stratum %d ntpdelay(%lld)\n",
					ns->name, ntpin.stratum, ns->rtt);
			return 0;
		}

		/* try again */
		sleep(250);
	}
	close(fd);
	return -1;
}

static int64_t
gpssample(void)
{
	int64_t	l, g, d;
	int	i, n;
	char	*v[4], buf[128];

	d = -1000000000000000000LL;
	for(i = 0; i < 5; i++){
		sleep(1100);
		seek(gpsfil, 0, 0);
		n = read(gpsfil, buf, sizeof buf - 1);
		if (n <= 0)
			return 0;
		buf[n] = 0;
		n = tokenize(buf, v, nelem(v));
		if(n != 4 || strcmp(v[3], "A") != 0)
			return 0;
		g = atoll(v[1]);
		l = atoll(v[2]);
		if(g-l > d)
			d = g-l;
	}
	return d;
}

static int64_t
ntpsample(void)
{
	NTPserver *tns, *ns;
	int64_t metric, x;

	metric = 1000LL*SEC;
	ns = nil;
	for(tns = ntpservers; tns != nil; tns = tns->next){
		if(ntptimediff(tns) < 0)
			continue;
		x = vabs(tns->rootdisp) + (vabs(tns->rtt+tns->rootdelay)>>1);
		if(debug)
			fprint(2, "ntp %s rootdelay %lld rootdisp %lld metric %lld\n",
				tns->name, tns->rootdelay, tns->rootdisp, x);
		if(x < metric){
			metric = x;
			ns = tns;
		}
	}

	if(ns == nil)
		return 0;

	/* save data for our server */
	rootdisp = ns->rootdisp;
	rootdelay = ns->rootdelay;
	mydelay = ns->rtt;
	mydisp = avgerr;
	if(ns->stratum == 0)
		stratum = 0;
	else
		stratum = ns->stratum + 1;

	etha = abs(ns->rtt/2);
	return ns->dt;
}

/*
 * sample the utc file
 */
static int64_t
utcsample(void)
{
	int64_t	s;
	int	n;
	char	*v[2], buf[128];

	s = 0;
	seek(utcfil, 0, 0);
	n = read(utcfil, buf, sizeof buf - 1);
	if (n <= 0)
		return 0;
	buf[n] = 0;
	n = tokenize(buf, v, nelem(v));
	if (strcmp(v[0], "0") == 0)
		return 0;
	if (n == 2) {
		gettime(&s, nil, nil);
		s -= atoll(v[1]);
	}
	lastutc = atoll(v[0]) + s;
	return lastutc;
}

/*
 *  sntp server
 */
static int
openlisten(char *net)
{
	int fd, cfd;
	char data[128], devdir[40];

	sprint(data, "%s/udp!*!ntp", net);
	cfd = announce(data, devdir);
	if(cfd < 0)
		sysfatal("can't announce");
	if(fprint(cfd, "headers") < 0)
		sysfatal("can't set header mode");

	sprint(data, "%s/data", devdir);
	fd = open(data, ORDWR);
	if(fd < 0)
		sysfatal("open %s: %r", data);
	return fd;
}

static void
ntpserver(char *servenet)
{
	int fd, n, vers, mode;
	int64_t recvts, x;
	char buf[512];
	NTPpkt *ntp;

	fd = openlisten(servenet);

	if (Rootid == nil)
		switch(type){
		case Fs:
			Rootid = "WWV";
			break;
		case Rtc:
			Rootid = "LOCL";
			break;
		case Utc:
			Rootid = "UTC";
			break;
		case Gps:
			Rootid = "GPS";
			break;
		case Ntp:
			/* set by the ntp client */
			break;
		}
	if (Rootid != nil)
		memmove(rootid, Rootid, strlen(Rootid) > 4? 4: strlen(Rootid));

	for(;;){
		n = read(fd, buf, sizeof buf);
		gettime(&recvts, 0, 0);
		if(n <= 0) {
			/* don't croak on input error, but don't spin either */
			sleep(500);
			continue;
		}
		if(n < Udphdrsize + NTPSIZE)
			continue;

		ntp = (NTPpkt*)(buf + Udphdrsize);
		mode = ntp->mode & 7;
		vers = (ntp->mode>>3) & 7;
		if(mode != 3)
			continue;

		ntp->mode = (vers<<3)|4;
		ntp->stratum = stratum;
		ntp->precision = myprec;
		hnputfp(ntp->rootdelay, rootdelay + mydelay);
		hnputfp(ntp->rootdisp, rootdisp + mydisp);
		hnputts(ntp->refts, lastutc);
		memmove(ntp->origts, ntp->xmitts, sizeof(ntp->origts));
		hnputts(ntp->recvts, recvts);
		memmove(ntp->rootid, rootid, sizeof(ntp->rootid));
		gettime(&x, 0, 0);
		hnputts(ntp->xmitts, x);
		write(fd, buf, NTPSIZE + Udphdrsize);
	}
}

/*
 *  get the current time from the file system
 */
static int32_t
fstime(void)
{
	Dir *d;
	uint32_t t;

	d = dirstat("/n/boot");
	if(d != nil){
		t = d->atime;
		free(d);
	} else
		t = 0;
	return t;
}

/*
 *  get the current time from the real time clock
 */
static int32_t
rtctime(void)
{
	char b[20];
	static int f = -1;
	int i, retries;

	memset(b, 0, sizeof(b));
	for(retries = 0; retries < 100; retries++){
		if(f < 0)
			f = open("/dev/rtc", OREAD|OCEXEC);
		if(f < 0)
			break;
		if(seek(f, 0, 0) < 0 || (i = read(f, b, sizeof b)) < 0){
			close(f);
			f = -1;
		} else
			if(i != 0)
				break;
	}
	return strtoul(b, 0, 10)+gmtdelta;
}


/*
 *  Sample a clock.  We wait for the clock to always
 *  be at the leading edge of a clock period.
 */
static int64_t
sample(int32_t (*get)(void))
{
	int32_t this, last;
	int64_t start, end;

	/*
	 *  wait for the second to change
	 */
	last = (*get)();
	for(;;){
		gettime(&start, 0, 0);
		this = (*get)();
		gettime(&end, 0, 0);
		if(this != last)
			break;
		last = this;
	}
	return SEC*this - (end-start)/2;
}

/*
 * the name of the frequency file has the method and possibly the
 * server name encoded in it.
 */
static int
openfreqfile(void)
{
	char *p;
	int fd;

	if(sysid == nil)
		return -1;

	switch(type){
	case Ntp:
		p = smprint("%s/ts.%s.%d.%s", dir, sysid, type, timeserver);
		break;
	default:
		p = smprint("%s/ts.%s.%d", dir, sysid, type);
		break;
	}
	fd = open(p, ORDWR);
	if(fd < 0)
		fd = create(p, ORDWR, 0666);
	free(p);
	if(fd < 0)
		return -1;
	return fd;
}

/*
 *  the file contains the last known frequency and the
 *  number of seconds it was sampled over
 */
static int64_t
readfreqfile(int fd, int64_t ohz, int64_t minhz, int64_t maxhz)
{
	int n;
	char buf[128];
	int64_t hz;

	n = read(fd, buf, sizeof buf-1);
	if(n <= 0)
		return ohz;
	buf[n] = 0;
	hz = strtoll(buf, nil, 0);

	if(hz > maxhz || hz < minhz)
		return ohz;

	settime(-1, hz, 0, 0);
	return hz;
}

/*
 *  remember hz and averaging period
 */
static void
writefreqfile(int fd, int64_t hz, int secs, int64_t diff)
{
	int32_t now;
	static int32_t last;

	if(fd < 0)
		return;
	now = time(0);
	if(now - last < 10*60)
		return;
	last = now;
	if(seek(fd, 0, 0) < 0)
		return;
	fprint(fd, "%lld %d %d %lld\n", hz, secs, type, diff);
}

static uint64_t
vabs(int64_t x)
{
	if(x < 0)
		return -x;
	else
		return x;
}

static void
background(void)
{
	static int inbackground;

	if(inbackground)
		return;

	if(!debug)
		switch(rfork(RFPROC|RFFDG|RFNAMEG|RFNOTEG|RFNOWAIT)){
		case -1:
			sysfatal("forking: %r");
			break;
		case 0:
			break;
		default:
			exits(0);
		}
	inbackground = 1;
}

static int
getclockprecision(int64_t hz)
{
	int i;

	i = 8;
	while(hz > 0){
		i--;
		hz >>= 1;
	}
	return i;
}
