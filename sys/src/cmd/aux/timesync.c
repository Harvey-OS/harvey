#include <u.h>
#include <libc.h>
#include <ip.h>


#define SEC 1000000000LL
#define MAXSECS 5*60
double gain = .25;

enum {
	Fs,
	Rtc,
	Ntp,
};


char *dir = "/tmp";	// directory sample files live in
char *logfile = "timesync";
char *timeserver;
int debug;
int impotent;
int logging;
int type;
int gmtdelta;	// rtc+gmtdelta = gmt

// ntp server info
int stratum = 14;
int mydisp, rootdisp;
int mydelay, rootdelay;
vlong avgdelay;
uchar rootid[4];
char *sysid;

// list of time samples
typedef struct Sample Sample;
struct Sample
{
	Sample	*next;
	uvlong	ticks;
	vlong	ltime;
	vlong	stime;
};

// ntp packet
typedef struct NTPpkt NTPpkt;
struct NTPpkt
{
	uchar	mode;
	uchar	stratum;
	uchar	poll;
	uchar	precision;
	uchar	rootdelay[4];
	uchar	rootdisp[4];
	uchar	rootid[4];
	uchar	refts[8];
	uchar	origts[8];		// departed client
	uchar	recvts[8];		// arrived at server
	uchar	xmitts[8];		// departed server
	uchar	keyid[4];
	uchar	digest[16];
};

enum
{
	NTPSIZE= 	48,		// basic ntp packet
	NTPDIGESTSIZE=	20,		// key and digest
};

static void	inittime(void);
static int	gettime(vlong*, uvlong*, uvlong*);	// returns time, ticks, hz
static void	settime(vlong, uvlong, vlong, int);	// set time, hz, delta, period
static void	setpriority(void);

// ((1970-1900)*365 + 17/*leap days*/)*24*60*60
#define EPOCHDIFF 2208988800UL

// convert to ntp timestamps
void
hnputts(void *p, vlong nsec)
{
	uchar *a;
	ulong tsh;
	ulong tsl;

	a = p;

	// zero is a special case
	if(nsec == 0)
		return;

	tsh = (nsec/SEC);
	nsec -= tsh*SEC;
	tsl = (nsec<<32)/SEC;
	hnputl(a, tsh+EPOCHDIFF);
	hnputl(a+4, tsl);
}

// convert from ntp timestamps
vlong
nhgetts(void *p)
{
	uchar *a;
	ulong tsh, tsl;
	vlong nsec;

	a = p;
	tsh = nhgetl(a);
	tsl = nhgetl(a+4);
	nsec = tsl*SEC;
	nsec >>= 32;
	nsec += (tsh - EPOCHDIFF)*SEC;
	return nsec;
}

// get network address of the server
void
setrootid(char *d)
{
	char buf[128];
	int fd, n;
	char *p;

	snprint(buf, sizeof(buf), "%s/remote", d);
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

void
ding(void*, char *s)
{
	if(strstr(s, "alarm") != nil)
		noted(NCONT);
	noted(NDFLT);
}

//
//  sntp client, we keep calling if the delay seemed
//  unusually high, i.e., 30% longer than avg.
//
vlong
ntptimediff(char *server)
{
	int fd, tries, n;
	NTPpkt ntpin, ntpout;
	vlong dt, recvts, origts, xmitts, destts, delay, x;
	char dir[64];

	fd = dial(netmkaddr(server, "udp", "ntp"), 0, 0, 0);
	if(fd < 0){
		syslog(0, logfile, "can't reach %s: %r", server);
		return 0LL;
	}
	setrootid(dir);
	notify(ding);

	memset(&ntpout, 0, sizeof(ntpout));
	ntpout.mode = 3 | (3 << 3);

	for(tries = 0; tries < 100; tries++){
		alarm(2*1000);

		gettime(&x, 0, 0);
		hnputts(ntpout.xmitts, x);
		if(write(fd, &ntpout, NTPSIZE) < 0){
			alarm(0);
			continue;
		}

		n = read(fd, &ntpin, sizeof(ntpin));
		alarm(0);
		gettime(&destts, 0, 0);
		if(n >= NTPSIZE){
			recvts = nhgetts(ntpin.recvts);
			origts = nhgetts(ntpin.origts);
			xmitts = nhgetts(ntpin.xmitts);
			stratum = ntpin.stratum;
			dt = ((recvts - origts) + (xmitts - destts))/2;
			delay = ((destts - origts) - (xmitts - recvts))/2;
			mydelay = (avgdelay<<16)/SEC;
			rootdelay = nhgetl(ntpin.rootdelay);
			mydisp = (dt<<16)/SEC;
			rootdisp = nhgetl(ntpin.rootdisp);
			if(100*delay < avgdelay*130){
				avgdelay = 7*(avgdelay>>3) + (delay>>3);
				if(debug)
					fprint(2, "ntpdelay(%lld)\n", delay);
				close(fd);
				return dt;
			}
			avgdelay = 7*(avgdelay>>3) + (delay>>3);
		}

		// try again
		sleep(1000);
	}
	close(fd);
	return 0LL;
}

//
//  sntp server
//
void
ntpserver(void)
{
	int fd, cfd, n;
	NTPpkt *ntp;
	char buf[512];
	int vers, mode;
	vlong recvts, x;

	fd = dial("udp!0!ntp", "123", 0, &cfd);
	if(fd < 0)
		return;
	if(fprint(cfd, "headers") < 0)
		return;
	close(cfd);

	switch(type){
	case Fs:
		memmove(rootid, "WWV", 3);
		break;
	case Rtc:
		memmove(rootid, "LOCL", 3);
		break;
	case Ntp:
		/* set by the ntp client */
		break;
	}

	for(;;){
		n = read(fd, buf, sizeof(buf));
		gettime(&recvts, 0, 0);
		if(n < 0)
			return;
		if(n < Udphdrsize + NTPSIZE)
			continue;

		ntp = (NTPpkt*)(buf+Udphdrsize);
		mode = ntp->mode & 7;
		vers = (ntp->mode>>3) & 7;
		if(mode != 3)
			continue;

		ntp->mode = (vers<<3)|4;
		ntp->stratum = stratum + 1;
		hnputl(ntp->rootdelay, rootdelay + mydelay);
		hnputl(ntp->rootdisp, rootdelay + mydisp);
		memmove(ntp->origts, ntp->xmitts, sizeof(ntp->origts));
		hnputts(ntp->recvts, recvts);
		memmove(ntp->rootid, rootid, sizeof(ntp->rootid));
		gettime(&x, 0, 0);
		hnputts(ntp->xmitts, x);
		write(fd, buf, NTPSIZE+Udphdrsize);
	}
}

long
fstime(void)
{
	Dir d;

	if(dirstat("/n/boot", &d) < 0)
		sysfatal("stating /n/boot: %r");
	return d.atime;
}

long
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
		if(seek(f, 0, 0) < 0 || (i = read(f, b, sizeof(b))) < 0){
			close(f);
			f = -1;
		} else {
			if(i != 0)
				break;
		}
	}
	return strtoul(b, 0, 10)+gmtdelta;
}

vlong
sample(long (*get)(void))
{
	long this, last;
	vlong start, end;

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

void
getsysid(void)
{
	sysid = getenv("sysname");
	if(sysid != nil)
		return;
}

int
openfreqfile(void)
{
	char buf[64];
	int fd;

	if(sysid == nil)
		return -1;

	switch(type){
	case Ntp:
		snprint(buf, sizeof buf, "%s/ts.%s.%d.%s", dir, sysid, type, timeserver);
		break;
	default:
		snprint(buf, sizeof buf, "%s/ts.%s.%d", dir, sysid, type);
		break;
	}
	fd = open(buf, ORDWR);
	if(fd < 0)
		fd = create(buf, ORDWR, 0666);
	if(fd < 0)
		return -1;
	return fd;
}

//
//  the file contains the last known frequency and the
//  number of seconds it was sampled over
//
vlong
readfreqfile(int fd, vlong ohz, vlong minhz, vlong maxhz)
{
	int n;
	char buf[128];
	vlong hz;

	n = read(fd, buf, sizeof(buf)-1);
	if(n <= 0)
		return ohz;
	buf[n] = 0;
	hz = strtoll(buf, nil, 0);

	if(hz > maxhz || hz < minhz)
		return ohz;
	return hz;
}

//
//  remember hz and averaging period
//
void
writefreqfile(int fd, vlong hz, int secs, vlong diff)
{
	if(fd < 0)
		return;
	if(seek(fd, 0, 0) < 0)
		return;
	fprint(fd, "%lld %d %d %lld\n", hz, secs, type, diff);
}

void
main(int argc, char **argv)
{
	int diffsecs, secs, minsecs, avgdiff;
	int t, fd;
	Sample *s, *x, *first, **l;
	vlong diff, absdiff, accuracy;
	uvlong hz;
	double dT, dt, minhz, maxhz;
	char *a;
	Tm tl, tg;

	type = Fs;
	debug = 0;
	minsecs = 60;		// most frequent resync
	accuracy = 1000000LL;	// default accuracy is 1 millisecond
	avgdiff = 1;
	diffsecs = 0;

	ARGBEGIN{
	case 'a':
		a = ARGF();
		if(a == nil)
			sysfatal("bad accuracy specified");
		accuracy = strtoll(a, 0, 0);	// accuracy specified in ns
		if(accuracy <= 1LL)
			sysfatal("bad accuracy specified");
		break;
	case 'f':
		type = Fs;
		stratum = 2;
		break;
	case 'r':
		type = Rtc;
		stratum = 1;
		break;
	case 'n':
		type = Ntp;
		break;
	case 'D':
		debug = 1;
		break;
	case 'd':
		dir = ARGF();
		break;
	case 'L':
		//
		// Assume time source in local time rather than GMT.
		// Calculate difference so that rtctime can return GMT.
		// This is useful with the rtc on PC's that run Windows
		// since Windows keeps the local time in the rtc.
		//
		t = time(0);
		tl = *localtime(t);
		tg = *gmtime(t);

		// if the years are different, we're at most a day off, so just rewrite
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
	case 'i':
		impotent = 1;
		break;
	case 'l':
		logging = 1;
		break;
	}ARGEND;

	fmtinstall('E', eipconv);
	fmtinstall('I', eipconv);
	fmtinstall('V', eipconv);
	getsysid();

	// initial sampling period
	secs = 60;

	if(argc > 0)
		timeserver = argv[0];
	else
		switch(type){
		case Fs:
			timeserver = "/srv/boot";
			break;
		case Ntp:
			timeserver = "$ntp";
			break;
		}

	setpriority();

	//
	//  detach from the current namespace
	//
	if(debug)
		rfork(RFNAMEG);
	else {
		switch(rfork(RFPROC|RFFDG|RFNAMEG|RFNOTEG|RFNOWAIT)){
		case -1:
			sysfatal("forking: %r");
			break;
		case 0:
			break;
		default:
			exits(0);
		}
	}

	// figure out our time interface
	inittime();
	gettime(0, 0, &hz);

	// some sanity limits
	minhz = hz * 0.8;
	maxhz = hz * 1.2;

	//
	//  bind in clocks
	//
	switch(type){
	case Fs:
		fd = open(timeserver, ORDWR);
		if(fd < 0)
			sysfatal("opening %s: %r\n", timeserver);
		if(mount(fd, "/n/boot", MREPL, "") < 0)
			sysfatal("mounting %s: %r\n", timeserver);
		close(fd);
		break;
	case Rtc:
		bind("#r", "/dev", MAFTER);
		if(access("/dev/rtc", AREAD) < 0)
			sysfatal("accessing /dev/rtc: %r\n");
		break;
	}

	//
	//  start an ntp server
	//
	if(!impotent){
		switch(rfork(RFPROC|RFFDG|RFMEM|RFNOWAIT)){
		case -1:
			sysfatal("forking: %r");
			break;
		case 0:
			ntpserver();
			_exits(0);
			break;
		default:
			break;
		}
	}

	fd = openfreqfile();

	// get the last known frequency from the file
	hz = readfreqfile(fd, hz, minhz, maxhz);

	first = nil;
	l = &first;
	for(;; sleep(secs*(1000))){
		s = mallocz(sizeof(*s), 1);
		diff = 0;

		// get times for this sample
		switch(type){
		case Fs:
			s->stime = sample(fstime);
			break;
		case Rtc:
			s->stime = sample(rtctime);
			break;
		case Ntp:
			diff = ntptimediff(timeserver);
			break;
		}

		// use fastest method to read local clock and ticks
		gettime(&s->ltime, &s->ticks, 0);
		if(type == Ntp)
			s->stime = s->ltime + diff;
		if(s->stime < 0)
			continue;

		// forget any old samples
		while(first != nil){
			if(first->next == nil)
				break;
			if(s->stime - first->stime <= (5LL*MAXSECS*SEC)/4LL)
				break;
			x = first->next;
			free(first);
			first = x;
		}
		
		diff = s->stime - s->ltime;
		if(debug)
			fprint(2, "diff = %lld\n", diff);
		if(diff > 10*SEC || diff < -10*SEC){
			// we're way off, set the time
			// and forget all previous samples
			settime(s->stime, 0, 0, 0);
			while(first != nil){
				x = first;
				first = x->next;
				free(x);
			}
			l = &first;
		} else {
			// adjust period to fit error
			absdiff = diff;
			if(diff < 0)
				absdiff = -diff;
			if(secs < minsecs || absdiff < (accuracy>>1))
				secs += 30;
			else if(absdiff > accuracy && secs > minsecs)
				secs >>= 1;

			// work off difference
			avgdiff = (avgdiff>>1) + (diff>>1);
			if(avgdiff == 0)
				avgdiff = 1;
			diffsecs = 2*(diff*secs)/avgdiff;
			if(diffsecs < 0)
				diffsecs = -diffsecs;
			else if(diffsecs == 0)
				diffsecs = 1;
			else if(diffsecs > 4*secs)
				diffsecs = 4*secs;
			settime(-1, 0, diff, diffsecs);
		}

		// lots of checking to avoid floating pt exceptions
		if(first != nil)
		if(s->ticks > first->ticks)
		if(s->stime > first->stime){
			dT = s->ticks - first->ticks;
			dt = s->stime - first->stime;
			dT = dT/dt;
			dT *= SEC;

			// use result only if it looks sane
			if(dT <= maxhz && dT > minhz){
				hz = (1-gain)*hz + gain*dT;
				settime(-1, hz, 0, 0);
				writefreqfile(fd, hz, (s->stime - first->stime)/SEC, diff);
			}
		}

		if(logging)
			syslog(0, logfile, "%lld %lld %lld %lld %d %s %lld %d",
				s->ltime, s->stime,
				s->ticks, hz,
				type, type == Ntp ? timeserver : "",
				avgdelay, diffsecs);

		*l = s;
		l = &s->next;
	}
}


//
//  kernel interface
//
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

	// bind in clocks
	if(access("/dev/time", 0) < 0)
		bind("#c", "/dev", MAFTER);
	if(access("/dev/rtc", 0) < 0)
		bind("#r", "/dev", MAFTER);

	// figure out what interface we have
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

//
//  convert binary numbers from/to kernel
//
static uvlong uvorder = 0x0001020304050607ULL;

static uchar*
be2vlong(vlong *to, uchar *f)
{
	uchar *t, *o;
	int i;

	t = (uchar*)to;
	o = (uchar*)&uvorder;
	for(i = 0; i < sizeof(vlong); i++)
		t[o[i]] = f[i];
	return f+sizeof(vlong);
}

static uchar*
vlong2be(uchar *t, vlong from)
{
	uchar *f, *o;
	int i;

	f = (uchar*)&from;
	o = (uchar*)&uvorder;
	for(i = 0; i < sizeof(vlong); i++)
		t[i] = f[o[i]];
	return t+sizeof(vlong);
}

static long order = 0x00010203;

static uchar*
be2long(long *to, uchar *f)
{
	uchar *t, *o;
	int i;

	t = (uchar*)to;
	o = (uchar*)&order;
	for(i = 0; i < sizeof(long); i++)
		t[o[i]] = f[i];
	return f+sizeof(long);
}

static uchar*
long2be(uchar *t, long from)
{
	uchar *f, *o;
	int i;

	f = (uchar*)&from;
	o = (uchar*)&order;
	for(i = 0; i < sizeof(long); i++)
		t[i] = f[o[i]];
	return t+sizeof(long);
}

//
// read ticks and local time in nanoseconds
//
static int
gettime(vlong *nsec, uvlong *ticks, uvlong *hz)
{
	int i, n;
	uchar ub[3*8], *p;
	char b[2*24+1];

	switch(ifc){
	case Ibintime:
		n = sizeof(vlong);
		if(hz != nil)
			n = 3*sizeof(vlong);
		if(ticks != nil)
			n = 2*sizeof(vlong);
		i = read(bintimefd, ub, n);
		if(i != n)
			break;
		p = ub;
		if(nsec != nil)
			be2vlong(nsec, ub);
		p += sizeof(vlong);
		if(ticks != nil)
			be2vlong((vlong*)ticks, p);
		p += sizeof(vlong);
		if(hz != nil)
			be2vlong((vlong*)hz, p);
		return 0;
	case Itiming:
		n = sizeof(vlong);
		if(ticks != nil)
			n = 2*sizeof(vlong);
		i = read(timingfd, ub, n);
		if(i != n)
			break;
		p = ub;
		if(nsec != nil)
			be2vlong(nsec, ub);
		p += sizeof(vlong);
		if(ticks != nil)
			be2vlong((vlong*)ticks, p);
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
settime(vlong now, uvlong hz, vlong delta, int n)
{
	uchar b[1+sizeof(vlong)+sizeof(long)], *p;

	if(debug)
		fprint(2, "settime(now=%lld, hz=%llud, delta=%lld, period=%d)\n", now, hz, delta, n);
	if(impotent)
		return;
	switch(ifc){
	case Ibintime:
		if(now >= 0){
			p = b;
			*p++ = 'n';
			p = vlong2be(p, now);
			if(write(bintimefd, b, p-b) < 0)
				sysfatal("writing /dev/bintime: %r");
		}
		if(delta != 0){
			p = b;
			*p++ = 'd';
			p = vlong2be(p, delta);
			p = long2be(p, n);
			if(write(bintimefd, b, p-b) < 0)
				sysfatal("writing /dev/bintime: %r");
		}
		if(hz != 0){
			p = b;
			*p++ = 'f';
			p = vlong2be(p, hz);
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

//
//  set priority high
//
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
	close(fd);
}
