#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/*
 *  real time clock and non-volatile ram
 */

enum {
	Addr=		0x70,	/* address port */
	Data=		0x71,	/* data port */

	Seconds=	0x00,
	Minutes=	0x02,
	Hours=		0x04,
	Mday=		0x07,
	Month=		0x08,
	Year=		0x09,
	Status=		0x0A,

	Nvoff=		128,	/* where usable nvram lives */
	Nvsize=		256,

	Nbcd=		6,
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
	Qnvram,
};

Dirtab rtcdir[]={
	".",	{Qdir, 0, QTDIR},	0,	0555,
	"nvram",	{Qnvram, 0},	Nvsize,	0664,
	"rtc",		{Qrtc, 0},	0,	0664,
};

static ulong rtc2sec(Rtc*);
static void sec2rtc(ulong, Rtc*);

void
rtcinit(void)
{
	if(ioalloc(Addr, 2, 0, "rtc/nvr") < 0)
		panic("rtcinit: ioalloc failed");
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

static long
rtcstat(Chan* c, uchar* dp, long n)
{
	return devstat(c, dp, n, rtcdir, nelem(rtcdir), devgen);
}

static Chan*
rtcopen(Chan* c, int omode)
{
	omode = openmode(omode);
	switch((ulong)c->qid.path){
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

#define GETBCD(o) ((bcdclock[o]&0xf) + 10*(bcdclock[o]>>4))

static long
rtcextract(void)
{
	uchar bcdclock[Nbcd];
	Rtc rtc;
	int i;

	/* don't do the read until the clock is no longer busy */
	for(i = 0; i < 10000; i++){
		outb(Addr, Status);
		if(inb(Data) & 0x80)
			continue;

		/* read clock values */
		outb(Addr, Seconds);	bcdclock[0] = inb(Data);
		outb(Addr, Minutes);	bcdclock[1] = inb(Data);
		outb(Addr, Hours);	bcdclock[2] = inb(Data);
		outb(Addr, Mday);	bcdclock[3] = inb(Data);
		outb(Addr, Month);	bcdclock[4] = inb(Data);
		outb(Addr, Year);	bcdclock[5] = inb(Data);

		outb(Addr, Status);
		if((inb(Data) & 0x80) == 0)
			break;
	}

	/*
	 *  convert from BCD
	 */
	rtc.sec = GETBCD(0);
	rtc.min = GETBCD(1);
	rtc.hour = GETBCD(2);
	rtc.mday = GETBCD(3);
	rtc.mon = GETBCD(4);
	rtc.year = GETBCD(5);

	/*
	 *  the world starts jan 1 1970
	 */
	if(rtc.year < 70)
		rtc.year += 2000;
	else
		rtc.year += 1900;
	return rtc2sec(&rtc);
}

static Lock nvrtlock;

long
rtctime(void)
{
	int i;
	long t, ot;

	ilock(&nvrtlock);

	/* loop till we get two reads in a row the same */
	t = rtcextract();
	for(i = 0; i < 100; i++){
		ot = rtcextract();
		if(ot == t)
			break;
	}
	iunlock(&nvrtlock);

	if(i == 100) print("we are boofheads\n");

	return t;
}

static long
rtcread(Chan* c, void* buf, long n, vlong off)
{
	ulong t;
	char *a, *start;
	ulong offset = off;

	if(c->qid.type & QTDIR)
		return devdirread(c, buf, n, rtcdir, nelem(rtcdir), devgen);

	switch((ulong)c->qid.path){
	case Qrtc:
		t = rtctime();
		n = readnum(offset, buf, n, t, 12);
		return n;
	case Qnvram:
		if(n == 0)
			return 0;
		if(n > Nvsize)
			n = Nvsize;
		a = start = smalloc(n);

		ilock(&nvrtlock);
		for(t = offset; t < offset + n; t++){
			if(t >= Nvsize)
				break;
			outb(Addr, Nvoff+t);
			*a++ = inb(Data);
		}
		iunlock(&nvrtlock);

		if(waserror()){
			free(start);
			nexterror();
		}
		memmove(buf, start, t - offset);
		poperror();

		free(start);
		return t - offset;
	}
	error(Ebadarg);
	return 0;
}

#define PUTBCD(n,o) bcdclock[o] = (n % 10) | (((n / 10) % 10)<<4)

static long
rtcwrite(Chan* c, void* buf, long n, vlong off)
{
	int t;
	char *a, *start;
	Rtc rtc;
	ulong secs;
	uchar bcdclock[Nbcd];
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
		PUTBCD(rtc.sec, 0);
		PUTBCD(rtc.min, 1);
		PUTBCD(rtc.hour, 2);
		PUTBCD(rtc.mday, 3);
		PUTBCD(rtc.mon, 4);
		PUTBCD(rtc.year, 5);

		/*
		 *  write the clock
		 */
		ilock(&nvrtlock);
		outb(Addr, Seconds);	outb(Data, bcdclock[0]);
		outb(Addr, Minutes);	outb(Data, bcdclock[1]);
		outb(Addr, Hours);	outb(Data, bcdclock[2]);
		outb(Addr, Mday);	outb(Data, bcdclock[3]);
		outb(Addr, Month);	outb(Data, bcdclock[4]);
		outb(Addr, Year);	outb(Data, bcdclock[5]);
		iunlock(&nvrtlock);
		return n;
	case Qnvram:
		if(n == 0)
			return 0;
		if(n > Nvsize)
			n = Nvsize;

		start = a = smalloc(n);
		if(waserror()){
			free(start);
			nexterror();
		}
		memmove(a, buf, n);
		poperror();

		ilock(&nvrtlock);
		for(t = offset; t < offset + n; t++){
			if(t >= Nvsize)
				break;
			outb(Addr, Nvoff+t);
			outb(Data, *a++);
		}
		iunlock(&nvrtlock);

		free(start);
		return t - offset;
	}
	error(Ebadarg);
	return 0;
}

Dev rtcdevtab = {
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

uchar
nvramread(int addr)
{
	uchar data;

	ilock(&nvrtlock);
	outb(Addr, addr);
	data = inb(Data);
	iunlock(&nvrtlock);

	return data;
}

void
nvramwrite(int addr, uchar data)
{
	ilock(&nvrtlock);
	outb(Addr, addr);
	outb(Data, data);
	iunlock(&nvrtlock);
}
