#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"io.h"

/*
 *  real time clock and non-volatile ram
 */

extern ulong boottime;

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

enum
{
	Qrtc = 1,
	Qnvram,

	Seconds=	0x00,
	Minutes=	0x02,
	Hours=		0x04, 
	Mday=		0x07,
	Month=		0x08,
	Year=		0x09,
	Status=		0x0A,
	StatusB=	0x0B,
	StatusC=	0x0C,
	StatusD=	0x0D,

	Update=		0x80,

	Nvsize = 4096,

	Nclock=		6,
};

#define GETBCD(v)	(((v) & 0x0F) + 10*((v)>>4))
#define PUTBCD(v)	((v) % 10)|((((v)/10) % 10)<<4)

Dirtab rtcdir[]={
	"nvram",	{Qnvram, 0},	Nvsize,	0664,
	"rtc",		{Qrtc, 0},	0,	0664,
};

static ulong rtc2sec(Rtc*);
static void sec2rtc(ulong, Rtc*);

static int isbinary;
static Lock rtclock;

static void
rtcreset(void)
{
	int i, x;
	uchar r;

	for(i = 0; i < 10000; i++){
		x = (*(uchar*)Rtcindex)&~0x7f;
		*(uchar*)Rtcindex = x|Status;
		r = *(uchar*)Rtcdata;
		if(r & 0x80)
			continue;

		*(uchar*)Rtcindex = x|StatusB;
		isbinary = *(uchar*)Rtcdata & 0x04;
		break;
	}
}

static Chan*
rtcattach(char *spec)
{
	return devattach('r', spec);
}

static int	 
rtcwalk(Chan *c, char *name)
{
	return devwalk(c, name, rtcdir, nelem(rtcdir), devgen);
}

static void	 
rtcstat(Chan *c, char *dp)
{
	devstat(c, dp, rtcdir, nelem(rtcdir), devgen);
}

static Chan*
rtcopen(Chan *c, int omode)
{
	omode = openmode(omode);
	switch(c->qid.path){
	case Qrtc:
		if(strcmp(up->user, eve)!=0 && omode!=OREAD)
			error(Eperm);
		break;
	case Qnvram:
		if(strcmp(up->user, eve)!=0)
			error(Eperm);
	}
	return devopen(c, omode, rtcdir, nelem(rtcdir), devgen);
}

static void	 
rtcclose(Chan*)
{
}

static long	 
_rtctime(void)
{
	Rtc rtc;
	int i, x;
	uchar r;

	/* don't do the read until the clock is no longer busy */
	for(i = 0; i < 10000; i++){
		x = (*(uchar*)Rtcindex)&~0x7f;
		*(uchar*)Rtcindex = x|Status;
		r = *(uchar*)Rtcdata;
		if(r & Update)
			continue;

		/* read clock values */
		*(uchar*)Rtcindex = x|Seconds;
		rtc.sec = *(uchar*)Rtcdata;
		*(uchar*)Rtcindex = x|Minutes;
		rtc.min = *(uchar*)Rtcdata;
		*(uchar*)Rtcindex = x|Hours;
		rtc.hour = *(uchar*)Rtcdata;
		*(uchar*)Rtcindex = x|Mday;
		rtc.mday = *(uchar*)Rtcdata;
		*(uchar*)Rtcindex = x|Month;
		rtc.mon = *(uchar*)Rtcdata;
		*(uchar*)Rtcindex = x|Year;
		rtc.year = *(uchar*)Rtcdata;

		*(uchar*)Rtcindex = x|Status;
		r = *(uchar*)Rtcdata;
		if((r & Update) == 0)
			break;
	}

	if(!isbinary){
		/*
		 *  convert from BCD
		 */
		rtc.sec = GETBCD(rtc.sec);
		rtc.min = GETBCD(rtc.min);
		rtc.hour = GETBCD(rtc.hour);
		rtc.mday = GETBCD(rtc.mday);
		rtc.mon = GETBCD(rtc.mon);
		rtc.year = GETBCD(rtc.year);
	}

	/*
	 *  the world starts jan 1 1970
	 */
	rtc.year += 1970;

	return rtc2sec(&rtc);
}

long
rtctime(void)
{
	int i;
	long t, ot;

	ilock(&rtclock);

	/* loop till we get two reads in a row the same */
	t = _rtctime();
	for(i = 0; i < 100; i++){
		ot = t;
		t = _rtctime();
		if(ot == t)
			break;
	}
	iunlock(&rtclock);

	return t;
}

static long	 
rtcread(Chan *c, void *buf, long n, vlong off)
{
	uchar *f, *to, *e;
	ulong offset = off;

	if(c->qid.path & CHDIR)
		return devdirread(c, buf, n, rtcdir, nelem(rtcdir), devgen);

	switch(c->qid.path){
	case Qrtc:
		return readnum(offset, buf, n, rtctime(), 12);
	case Qnvram:
		if(offset > Nvsize)
			return -1;
		if(offset + n > Nvsize)
			n = Nvsize - offset;
		f = (uchar*)Nvram+offset;
		to = buf;
		e = f + n;
		while(f < e)
			*to++ = *f++;
		return n;
	}
	error(Ebadarg);
	return 0;
}

static long	 
rtcwrite(Chan *c, void *buf, long n, vlong off)
{
	Rtc rtc;
	ulong secs;
	char *cp, *ep;
	uchar *f, *t, *e;
	uchar x;
	ulong offset = off;

	switch(c->qid.path){
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
		sec2rtc(secs, &rtc);
		rtc.year -= 1970;

		if(!isbinary){	
			/*
			 *  convert to bcd
			 */
			rtc.sec = PUTBCD(rtc.sec);
			rtc.min = PUTBCD(rtc.min);
			rtc.hour = PUTBCD(rtc.hour);
			rtc.mday = PUTBCD(rtc.mday);
			rtc.mon = PUTBCD(rtc.mon);
			rtc.year = PUTBCD(rtc.year);
		}

		ilock(&rtclock);
		/* set clock values */
		x = (*(uchar*)Rtcindex)&~0x7f;
		*(uchar*)Rtcindex = x|Seconds;
		*(uchar*)Rtcdata = rtc.sec;
		*(uchar*)Rtcindex = x|Minutes;
		*(uchar*)Rtcdata = rtc.min;
		*(uchar*)Rtcindex = x|Hours;
		*(uchar*)Rtcdata = rtc.hour;
		*(uchar*)Rtcindex = x|Mday;
		*(uchar*)Rtcdata = rtc.mday;
		*(uchar*)Rtcindex = x|Month;
		*(uchar*)Rtcdata = rtc.mon;
		*(uchar*)Rtcindex = x|Year;
		*(uchar*)Rtcdata = rtc.year;
		iunlock(&rtclock);

		return n;
	case Qnvram:
		if(offset > Nvsize)
			return -1;
		if(offset + n > Nvsize)
			n = Nvsize - offset;
		t = (uchar*)Nvram+offset;
		f = buf;
		e = f + n;
		while(f < e)
			*t++ = *f++;
		return n;
	}
	error(Ebadarg);
	return 0;
}

Dev rtcdevtab = {
	'r',
	"rtc",

	rtcreset,
	devinit,
	rtcattach,
	devclone,
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
yrsize(int yr)
{
	if((yr % 4) == 0)
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
