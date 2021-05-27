/*
 * Maxim DS3231 realtime clock (accessed via rtc)
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

enum {
	/* DS3231 registers */
	Seconds=	0,
	Minutes=	1,
	Hours=		2,
	Weekday=	3,
	Mday=		4,
	Month=		5,
	Year=		6,
	Nbcd=		7,

	/* Hours register may be in 12-hour or 24-hour mode */
	Twelvehr=	1<<6,
	Pm=		1<<5,

	I2Caddr=	0x68,
	
};

typedef struct Rtc	Rtc;

struct Rtc
{
	int	sec;
	int	min;
	int	hour;
	int	mday;
	int	mon;
	int	year;
};

enum{
	Qdir = 0,
	Qrtc,
};

Dirtab rtcdir[]={
	".",	{Qdir, 0, QTDIR},	0,	0555,
	"rtc",		{Qrtc, 0},	0,	0664,
};

static ulong rtc2sec(Rtc*);
static void sec2rtc(ulong, Rtc*);

static void
i2cread(uint addr, void *buf, int len)
{
	I2Cdev d;

	d.addr = addr;
	d.tenbit = 0;
	d.salen = 0;
	i2crecv(&d, buf, len, 0);
}

static void
i2cwrite(uint addr, void *buf, int len)
{
	I2Cdev d;

	d.addr = addr;
	d.tenbit = 0;
	d.salen = 0;
	i2csend(&d, buf, len, 0);
}

static void
rtcinit()
{
	i2csetup(0);
}

static Chan*
rtcattach(char* spec)
{
	return devattach('r', spec);
}

static Walkqid*	 
rtcwalk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, rtcdir, nelem(rtcdir), devgen);
}

static int	 
rtcstat(Chan* c, uchar* dp, int n)
{
	return devstat(c, dp, n, rtcdir, nelem(rtcdir), devgen);
}

static Chan*
rtcopen(Chan* c, int omode)
{
	char dummy;

	omode = openmode(omode);
	switch((ulong)c->qid.path){
	case Qrtc:
		if(strcmp(up->user, eve)!=0 && omode!=OREAD)
			error(Eperm);
		/* if it's not there, this will throw an error */
		i2cread(I2Caddr, &dummy, 1);
		break;
	}
	return devopen(c, omode, rtcdir, nelem(rtcdir), devgen);
}

static void	 
rtcclose(Chan*)
{
}

static int
bcd(int n)
{
	return (n & 0xF) + (10 * (n >> 4));
}

long	 
rtctime(void)
{
	uchar clk[Nbcd];
	Rtc rtc;

	clk[0] = 0;
	i2cwrite(I2Caddr, clk, 1);
	i2cread(I2Caddr, clk, Nbcd);

	/*
	 *  convert from BCD
	 */
	rtc.sec = bcd(clk[Seconds]);
	rtc.min = bcd(clk[Minutes]);
	rtc.hour = bcd(clk[Hours]);
	if(clk[Hours] & Twelvehr){
		rtc.hour = bcd(clk[Hours] & 0x1F);
		if(clk[Hours] & Pm)
			rtc.hour += 12;
	}
	rtc.mday = bcd(clk[Mday]);
	rtc.mon = bcd(clk[Month] & 0x1F);
	rtc.year = bcd(clk[Year]);

	/*
	 *  the world starts jan 1 1970
	 */
	if(rtc.year < 70)
		rtc.year += 2000;
	else
		rtc.year += 1900;
	return rtc2sec(&rtc);
}


static long	 
rtcread(Chan* c, void* buf, long n, vlong off)
{
	ulong t;
	ulong offset = off;

	if(c->qid.type & QTDIR)
		return devdirread(c, buf, n, rtcdir, nelem(rtcdir), devgen);

	switch((ulong)c->qid.path){
	case Qrtc:
		t = rtctime();
		n = readnum(offset, buf, n, t, 12);
		return n;
	}
	error(Ebadarg);
	return 0;
}

#define PUTBCD(n,o) bcdclock[1+o] = (n % 10) | (((n / 10) % 10)<<4)

static long	 
rtcwrite(Chan* c, void* buf, long n, vlong off)
{
	Rtc rtc;
	ulong secs;
	uchar bcdclock[1+Nbcd];
	char *cp, *ep;
	ulong offset = off;

	if(offset!=0)
		error(Ebadarg);


	switch((ulong)c->qid.path){
	case Qrtc:
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
		PUTBCD(rtc.sec, Seconds);
		PUTBCD(rtc.min, Minutes);	/* forces 24 hour mode */
		PUTBCD(rtc.hour, Hours);
		PUTBCD(0, Weekday);		/* hope no other OS uses this */
		PUTBCD(rtc.mday, Mday);
		PUTBCD(rtc.mon, Month);
		PUTBCD(rtc.year, Year);

		/*
		 *  write the clock
		 */
		bcdclock[0] = 0;
		i2cwrite(I2Caddr, bcdclock, 1+Nbcd);
		return n;
	}
	error(Ebadarg);
	return 0;
}

Dev rtc3231devtab = {
	'r',
	"rtc",

	devreset,
	rtcinit,
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
static int*
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
