#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"io.h"

enum{
	/*
	 *  register offsets into time of day clock
	 */
	NVsec=	0,
	NVsecalarm,
	NVmin,
	NVminalarm,
	NVhour,	
	NVhouralarm,
	NVday,		/* (1 = Sun) */
	NVmday,		/* (0-31) */
	NVmon,		/* (1-12) */
	NVyear,		/* (0-99) */

	NVOFF=	0,		/* skip the first 1k, used by challenge */
	NVLEN=	4096,		/* length in bytes of NV RAM */
	NVCKSUM = 0,

	Qrtc = 1,
	Qnvram,
};

/*
 *  time of day clock
 */
typedef struct
{
	uchar	pad0[7];
	uchar	ptr;
	uchar	pad1[7];
	uchar	data;
} Nvtodc;
#define EPCTODC		EPCSWIN(Nvtodc, 0x3200)

/*
 *  Nvram is divided into pages of 32 entries
 */
typedef struct
{
	struct {
	 	uchar	pad[7];
	 	uchar	val;
	} data[0x20];
	uchar	pad[7];
	uchar	ptr;
} Nvram;
#define	EPCNVRAM	EPCSWIN(Nvram, 0x3000)

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
	ulong t, ot;
	int i;
	ulong offset = off;

	if(c->qid.path & CHDIR)
		return devdirread(c, buf, n, rtcdir, nelem(rtcdir), devgen);

	switch(c->qid.path){
	case Qrtc:
		qlock(&rtclock);
		t = rtctime();
		do{
			ot = t;
			t = rtctime();	/* make sure there's no skew */
		}while(t != ot);
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

	switch(c->qid.path){
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

static void
nvput(int offset, uchar val)
{
	Nvram *nv = EPCNVRAM;

	nv->ptr = offset>>5;
	nv->data[offset & 0x1f].val = val;
}

static uchar
nvget(int offset)
{
	Nvram *nv = EPCNVRAM;

	nv->ptr = offset>>5;
	return nv->data[offset & 0x1f].val;
}

static void
nvcksum(void)
{
	uchar cksum;
	int i;
	uchar x;

	/*
	 * Seed the checksum so all-zeroes (all-ones) nvram doesn't have a zero
	 * (all-ones) checksum.
	 */
	cksum = 0xa5;
	for(i = 0; i < NVLEN; i++){
		x = nvget(i);
		if(x != 0)
			cksum ^= x;
		cksum = (cksum << 1) | ((cksum >> 7) & 1);
	}
	nvput(NVCKSUM, cksum);
}

long	 
rtctime(void)
{
	Nvtodc *todc = EPCTODC;
	Rtc rtc;

	/*
	 *  convert from BCD
	 */
	todc->ptr = NVsec;
	rtc.sec = todc->data;
	todc->ptr = NVmin;
	rtc.min = todc->data;
	todc->ptr = NVhour;
	rtc.hour = todc->data;
	todc->ptr = NVmday;
	rtc.mday = todc->data;
	todc->ptr = NVmon;
	rtc.mon = todc->data;
	todc->ptr = NVyear;
	rtc.year = todc->data;
	if(rtc.year < 70)
		rtc.year += 2000;
	else
		rtc.year += 1900;

	return rtc2sec(&rtc);
}

static void
setrtc(Rtc *rtc)
{
	Nvtodc *todc = EPCTODC;

	todc->ptr = NVsec;
	todc->data = rtc->sec;
	todc->ptr = NVmin;
	todc->data = rtc->min;
	todc->ptr = NVhour;
	todc->data = rtc->hour;
	todc->ptr = NVmday;
	todc->data = rtc->mday;
	todc->ptr = NVmon;
	todc->data = rtc->mon;
	todc->ptr = NVyear;
	todc->data = rtc->year % 100;
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
