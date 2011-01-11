/*
 * devrtc - real-time clock, for kirkwood
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"

typedef	struct	RtcReg	RtcReg;
typedef	struct	Rtc	Rtc;

struct RtcReg
{
	ulong	time;
	ulong	date;
	ulong	alarmtm;
	ulong	alarmdt;
	ulong	intrmask;
	ulong	intrcause;
};

struct Rtc
{
	int	sec;
	int	min;
	int	hour;
	int	wday;
	int	mday;
	int	mon;
	int	year;
};

enum {
	Qdir,
	Qrtc,
};

static Dirtab rtcdir[] = {
	".",	{Qdir, 0, QTDIR},	0,		0555,
	"rtc",	{Qrtc},			NUMSIZE,	0664,
};
static	RtcReg	*rtcreg;		/* filled in by attach */
static	Lock	rtclock;

#define SEC2MIN	60
#define SEC2HOUR (60*SEC2MIN)
#define SEC2DAY (24L*SEC2HOUR)

/*
 * days per month plus days/year
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

	/*
	 *  seconds per year
	 */
	secs = 0;
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
	 * 19700101 was thursday
	 */
	rtc->wday = (day + 7340036L) % 7;

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
}

enum {
	Rtcsec	= 0x00007f,
	Rtcmin	= 0x007f00,
	Rtcms	= 8,
	Rtchr12	= 0x1f0000,
	Rtchr24	= 0x3f0000,
	Rtchrs	= 16,

	Rdmday	= 0x00003f,
	Rdmon	= 0x001f00,
	Rdms	= 8,
	Rdyear	= 0x7f0000,
	Rdys	= 16,

	Rtcpm	= 1<<21,		/* pm bit */
	Rtc12	= 1<<22,		/* 12 hr clock */
};

static ulong
bcd2dec(ulong bcd)
{
	ulong d, m, i;

	d = 0;
	m = 1;
	for(i = 0; i < 2 * sizeof d; i++){
		d += ((bcd >> (4*i)) & 0xf) * m;
		m *= 10;
	}
	return d;
}

static ulong
dec2bcd(ulong d)
{
	ulong bcd, i;

	bcd = 0;
	for(i = 0; d != 0; i++){
		bcd |= (d%10) << (4*i);
		d /= 10;
	}
	return bcd;
}

static long
_rtctime(void)
{
	ulong t, d;
	Rtc rtc;

	t = rtcreg->time;
	d = rtcreg->date;

	rtc.sec = bcd2dec(t & Rtcsec);
	rtc.min = bcd2dec((t & Rtcmin) >> Rtcms);

	if(t & Rtc12){
		rtc.hour = bcd2dec((t & Rtchr12) >> Rtchrs) - 1; /* 1—12 */
		if(t & Rtcpm)
			rtc.hour += 12;
	}else
		rtc.hour = bcd2dec((t & Rtchr24) >> Rtchrs);	/* 0—23 */

	rtc.mday = bcd2dec(d & Rdmday);				/* 1—31 */
	rtc.mon = bcd2dec((d & Rdmon) >> Rdms);			/* 1—12 */
	rtc.year = bcd2dec((d & Rdyear) >> Rdys) + 2000;	/* year%100 */

//	print("%0.2d:%0.2d:%.02d %0.2d/%0.2d/%0.2d\n", /* HH:MM:SS YY/MM/DD */
//		rtc.hour, rtc.min, rtc.sec, rtc.year, rtc.mon, rtc.mday);
	return rtc2sec(&rtc);
}

long
rtctime(void)
{
	int i;
	long t, ot;

	ilock(&rtclock);

	/* loop until we get two reads in a row the same */
	t = _rtctime();
	ot = ~t;
	for(i = 0; i < 100 && ot != t; i++){
		ot = t;
		t = _rtctime();
	}
	if(ot != t)
		print("rtctime: we are boofheads\n");

	iunlock(&rtclock);
	return t;
}

static void
setrtc(Rtc *rtc)
{
	ilock(&rtclock);
	rtcreg->time = dec2bcd(rtc->wday) << 24 | dec2bcd(rtc->hour) << 16 |
		dec2bcd(rtc->min) << 8 | dec2bcd(rtc->sec);
	rtcreg->date = dec2bcd(rtc->year - 2000) << 16 |
		dec2bcd(rtc->mon) << 8 | dec2bcd(rtc->mday);
	iunlock(&rtclock);
}

static Chan*
rtcattach(char *spec)
{
	rtcreg = (RtcReg*)soc.rtc;
	return devattach(L'r', spec);
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
	return devopen(c, omode, rtcdir, nelem(rtcdir), devgen);
}

static void
rtcclose(Chan*)
{
}

static long
rtcread(Chan *c, void *buf, long n, vlong off)
{
	if(c->qid.type & QTDIR)
		return devdirread(c, buf, n, rtcdir, nelem(rtcdir), devgen);

	switch((ulong)c->qid.path){
	default:
		error(Egreg);
	case Qrtc:
		return readnum(off, buf, n, rtctime(), NUMSIZE);
	}
}

static long
rtcwrite(Chan *c, void *buf, long n, vlong off)
{
	ulong offset = off;
	char *cp, sbuf[32];
	Rtc rtc;

	switch((ulong)c->qid.path){
	default:
		error(Egreg);
	case Qrtc:
		if(offset != 0 || n >= sizeof(sbuf)-1)
			error(Ebadarg);
		memmove(sbuf, buf, n);
		sbuf[n] = '\0';
		for(cp = sbuf; *cp != '\0'; cp++)
			if(*cp >= '0' && *cp <= '9')
				break;
		sec2rtc(strtoul(cp, 0, 0), &rtc);
		setrtc(&rtc);
		return n;
	}
}

Dev rtcdevtab = {
	L'r',
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
	devpower,
};
