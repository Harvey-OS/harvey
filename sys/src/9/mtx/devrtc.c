/*
 *	M48T59/559 Timekeeper
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"io.h"

enum{
	STB0 = 0x74,
	STB1 = 0x75,
	Data = 0x77,

	NVOFF=	0,
	NVLEN=	0x1ff0,		/* length in bytes of NV RAM */

	/*
	 *  register offsets into time of day clock
	 */
	NVflags=		0x1ff0,
	NVwatchdog=	0x1ff7,
	NVctl=		0x1ff8,
	NVsec,
	NVmin,
	NVhour,	
	NVday,		/* (1 = Sun) */
	NVmday,		/* (1-31) */
	NVmon,		/* (1-12) */
	NVyear,		/* (0-99) */

	/* NVctl */
	RTwrite = (1<<7),
	RTread = (1<<6),
	RTsign = (1<<5),
	RTcal = 0x1f,

	/* NVwatchdog */
	WDsteer = (1<<7),		/* 0 -> intr, 1 -> reset */
	WDmult = (1<<2),		/* 5 bits of multiplier */
	WDres0 = (0<<0),		/* 1/16 sec resolution */
	WDres1 = (1<<0),		/* 1/4 sec resolution */
	WDres2 = (2<<0),		/* 1 sec resolution */
	WDres3 = (3<<0),		/* 4 sec resolution */

	Qdir = 0,
	Qrtc,
	Qnvram,
};

/*
 *  broken down time
 */
typedef struct
{
	int	sec;
	int	min;
	int	hour;
	int	mday;
	int	mon;
	int	year;
} Rtc;

QLock	rtclock;		/* mutex on nvram operations */

static Dirtab rtcdir[]={
	".",		{Qdir, 0, QTDIR},	0,	DMDIR|0555,
	"rtc",		{Qrtc, 0},	0,	0644,
	"nvram",	{Qnvram, 0},	0,	0600,
};

static ulong	rtc2sec(Rtc*);
static void	sec2rtc(ulong, Rtc*);
static void	setrtc(Rtc*);
static void	nvcksum(void);
static void	nvput(int, uchar);
static uchar	nvget(int);

static Chan*
rtcattach(char *spec)
{
	return devattach('r', spec);
}

static Walkqid*
rtcwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, rtcdir, nelem(rtcdir), devgen);
}

static int	 
rtcstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, rtcdir, nelem(rtcdir), devgen);
}

static Chan*
rtcopen(Chan *c, int omode)
{
	omode = openmode(omode);
	switch((ulong)c->qid.path){
	case Qrtc:
		if(strcmp(up->user, eve)!=0 && omode!=OREAD)
			error(Eperm);
		break;
	case Qnvram:
		if(strcmp(up->user, eve)!=0 || !cpuserver)
			error(Eperm);
	}
	return devopen(c, omode, rtcdir, nelem(rtcdir), devgen);
}

static void	 
rtcclose(Chan*)
{
}

static long	 
rtcread(Chan *c, void *buf, long n, vlong off)
{
	char *p;
	ulong t;
	int i;
	ulong offset = off;

	if(c->qid.type & QTDIR)
		return devdirread(c, buf, n, rtcdir, nelem(rtcdir), devgen);

	switch((ulong)c->qid.path){
	case Qrtc:
		qlock(&rtclock);
		t = rtctime();
		qunlock(&rtclock);
		n = readnum(offset, buf, n, t, 12);
		return n;
	case Qnvram:
		offset += NVOFF;
		if(offset > NVLEN)
			return 0;
		if(n > NVLEN - offset)
			n = NVLEN - offset;
		p = buf;
		qlock(&rtclock);
		for(i = 0; i < n; i++)
			p[i] = nvget(i+offset);
		qunlock(&rtclock);
		return n;
	}
	error(Egreg);
	return -1;		/* never reached */
}

static long	 
rtcwrite(Chan *c, void *buf, long n, vlong off)
{
	Rtc rtc;
	ulong secs;
	char *cp, *ep;
	int i;
	ulong offset = off;

	switch((ulong)c->qid.path){
	case Qrtc:
		if(offset!=0)
			error(Ebadarg);
		/*
		 *  read the time
		 */
		cp = ep = buf;
		ep += n;
		while(cp < ep){
			if(*cp>='0' && *cp<='9')
				break;
			cp++;
		}
		secs = strtoul(cp, 0, 0);
		/*
		 *  convert to bcd
		 */
		sec2rtc(secs, &rtc);
		/*
		 * write it
		 */
		qlock(&rtclock);
		setrtc(&rtc);
		qunlock(&rtclock);
		return n;
	case Qnvram:
		offset += NVOFF;
		if(offset > NVLEN)
			return 0;
		if(n > NVLEN - offset)
			n = NVLEN - offset;
		qlock(&rtclock);
		for(i = 0; i < n; i++)
			nvput(i+offset, ((uchar*)buf)[i]);
		nvcksum();
		qunlock(&rtclock);
		return n;
	}
	error(Egreg);
	return -1;		/* never reached */
}

long
rtcbwrite(Chan *c, Block *bp, ulong offset)
{
	return devbwrite(c, bp, offset);
}

Dev rtcdevtab = {
	'r',
	"rtc",

	devreset,
	devinit,
	devshutdown,
	rtcattach,
	rtcwalk,
	rtcstat,
	rtcopen,
	devcreate,
	rtcclose,
	rtcread,
	devbread,
	rtcwrite,
	devbwrite,
	devremove,
	devwstat,
};

static void
nvput(int offset, uchar val)
{
	outb(STB0, offset);
	outb(STB1, offset>>8);
	outb(Data, val);
}

static uchar
nvget(int offset)
{
	outb(STB0, offset);
	outb(STB1, offset>>8);
	return inb(Data);
}

static void
nvcksum(void)
{
}

void
watchreset(void)
{
	splhi();
	nvput(NVwatchdog, WDsteer|(1*WDmult)|WDres0);
	for(;;);
}

static int
getbcd(int bcd)
{
	return (bcd&0x0f) + 10 * (bcd>>4);
}

static int
putbcd(int val)
{
	return (val % 10) | (((val/10) % 10) << 4);
}

long	 
rtctime(void)
{
	int ctl;
	Rtc rtc;

	/*
	 *  convert from BCD
	 */
	ctl = nvget(NVctl);
	ctl &= RTsign|RTcal;
	nvput(NVctl, ctl|RTread);

	rtc.sec = getbcd(nvget(NVsec) & 0x7f);
	rtc.min = getbcd(nvget(NVmin));
	rtc.hour = getbcd(nvget(NVhour));
	rtc.mday = getbcd(nvget(NVmday));
	rtc.mon = getbcd(nvget(NVmon));
	rtc.year = getbcd(nvget(NVyear));
	if(rtc.year < 70)
		rtc.year += 2000;
	else
		rtc.year += 1900;

	nvput(NVctl, ctl);

	return rtc2sec(&rtc);
}

static void
setrtc(Rtc *rtc)
{
	int ctl;

	ctl = nvget(NVctl);
	ctl &= RTsign|RTcal;
	nvput(NVctl, ctl|RTwrite);

	nvput(NVsec, putbcd(rtc->sec));
	nvput(NVmin, putbcd(rtc->min));
	nvput(NVhour, putbcd(rtc->hour));
	nvput(NVmday, putbcd(rtc->mday));
	nvput(NVmon, putbcd(rtc->mon));
	nvput(NVyear, putbcd(rtc->year % 100));

	nvput(NVctl, ctl);
}

#define SEC2MIN 60L
#define SEC2HOUR (60L*SEC2MIN)
#define SEC2DAY (24L*SEC2HOUR)

/*
 *  days per month plus days/year
 */
static	int	dmsize[] =
{
	365, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
static	int	ldmsize[] =
{
	366, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/*
 *  return the days/month for the given year
 */
static int *
yrsize(int y)
{

	if((y%4) == 0 && ((y%100) != 0 || (y%400) == 0))
		return ldmsize;
	else
		return dmsize;
}

/*
 *  compute seconds since Jan 1 1970
 */
static ulong
rtc2sec(Rtc *rtc)
{
	ulong secs;
	int i;
	int *d2m;

	secs = 0;

	/*
	 *  seconds per year
	 */
	for(i = 1970; i < rtc->year; i++){
		d2m = yrsize(i);
		secs += d2m[0] * SEC2DAY;
	}

	/*
	 *  seconds per month
	 */
	d2m = yrsize(rtc->year);
	for(i = 1; i < rtc->mon; i++)
		secs += d2m[i] * SEC2DAY;

	secs += (rtc->mday-1) * SEC2DAY;
	secs += rtc->hour * SEC2HOUR;
	secs += rtc->min * SEC2MIN;
	secs += rtc->sec;

	return secs;
}

/*
 *  compute rtc from seconds since Jan 1 1970
 */
static void
sec2rtc(ulong secs, Rtc *rtc)
{
	int d;
	long hms, day;
	int *d2m;

	/*
	 * break initial number into days
	 */
	hms = secs % SEC2DAY;
	day = secs / SEC2DAY;
	if(hms < 0) {
		hms += SEC2DAY;
		day -= 1;
	}

	/*
	 * generate hours:minutes:seconds
	 */
	rtc->sec = hms % 60;
	d = hms / 60;
	rtc->min = d % 60;
	d /= 60;
	rtc->hour = d;

	/*
	 * year number
	 */
	if(day >= 0)
		for(d = 1970; day >= *yrsize(d); d++)
			day -= *yrsize(d);
	else
		for (d = 1970; day < 0; d--)
			day += *yrsize(d-1);
	rtc->year = d;

	/*
	 * generate month
	 */
	d2m = yrsize(rtc->year);
	for(d = 1; day >= d2m[d]; d++)
		day -= d2m[d];
	rtc->mday = day + 1;
	rtc->mon = d;

	return;
}
