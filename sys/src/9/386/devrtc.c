/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	Paddr=		0x70,	/* address port */
	Pdata=		0x71,	/* data port */

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
	{".",	{Qdir, 0, QTDIR},	0,	0555},
	{"nvram",	{Qnvram, 0},	Nvsize,	0664},
	{"rtc",		{Qrtc, 0},	0,	0664},
};

static uint32_t rtc2sec(Rtc*);
static void sec2rtc(uint32_t, Rtc*);

void
rtcinit(void)
{
	if(ioalloc(Paddr, 2, 0, "rtc/nvr") < 0)
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

static int32_t
rtcstat(Chan* c, uint8_t* dp, int32_t n)
{
	return devstat(c, dp, n, rtcdir, nelem(rtcdir), devgen);
}

static Chan*
rtcopen(Chan* c, int omode)
{
	Proc *up = externup();
	omode = openmode(omode);
	switch((uint32_t)c->qid.path){
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
rtcclose(Chan* c)
{
}

#define GETBCD(o) ((bcdclock[o]&0xf) + 10*(bcdclock[o]>>4))

static int32_t
rtcextract(void)
{
	uint8_t bcdclock[Nbcd];
	Rtc rtc;
	int i;

	/* don't do the read until the clock is no longer busy */
	for(i = 0; i < 10000; i++){
		outb(Paddr, Status);
		if(inb(Pdata) & 0x80)
			continue;

		/* read clock values */
		outb(Paddr, Seconds);	bcdclock[0] = inb(Pdata);
		outb(Paddr, Minutes);	bcdclock[1] = inb(Pdata);
		outb(Paddr, Hours);	bcdclock[2] = inb(Pdata);
		outb(Paddr, Mday);	bcdclock[3] = inb(Pdata);
		outb(Paddr, Month);	bcdclock[4] = inb(Pdata);
		outb(Paddr, Year);	bcdclock[5] = inb(Pdata);

		outb(Paddr, Status);
		if((inb(Pdata) & 0x80) == 0)
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

int32_t
rtctime(void)
{
	int i;
	int32_t t, ot;

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

static int32_t
rtcread(Chan* c, void* buf, int32_t n, int64_t off)
{
	Proc *up = externup();
	uint32_t t;
	char *a, *start;
	uint32_t offset = off;

	if(c->qid.type & QTDIR)
		return devdirread(c, buf, n, rtcdir, nelem(rtcdir), devgen);

	switch((uint32_t)c->qid.path){
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
			outb(Paddr, Nvoff+t);
			*a++ = inb(Pdata);
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

static int32_t
rtcwrite(Chan* c, void* buf, int32_t n, int64_t off)
{
	Proc *up = externup();
	int t;
	char *a, *start;
	Rtc rtc;
	uint32_t secs;
	uint8_t bcdclock[Nbcd];
	char *cp, *ep;
	uint32_t offset = off;

	if(offset!=0)
		error(Ebadarg);


	switch((uint32_t)c->qid.path){
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
		outb(Paddr, Seconds);	outb(Pdata, bcdclock[0]);
		outb(Paddr, Minutes);	outb(Pdata, bcdclock[1]);
		outb(Paddr, Hours);	outb(Pdata, bcdclock[2]);
		outb(Paddr, Mday);	outb(Pdata, bcdclock[3]);
		outb(Paddr, Month);	outb(Pdata, bcdclock[4]);
		outb(Paddr, Year);	outb(Pdata, bcdclock[5]);
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
			outb(Paddr, Nvoff+t);
			outb(Pdata, *a++);
		}
		iunlock(&nvrtlock);

		free(start);
		return t - offset;
	}
	error(Ebadarg);
	return 0;
}

Dev rtcdevtab = {
	.dc = 'r',
	.name = "rtc",

	.reset = devreset,
	.init = rtcinit,
	.shutdown = devshutdown,
	.attach = rtcattach,
	.walk = rtcwalk,
	.stat = rtcstat,
	.open = rtcopen,
	.create = devcreate,
	.close = rtcclose,
	.read = rtcread,
	.bread = devbread,
	.write = rtcwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
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
static uint32_t
rtc2sec(Rtc *rtc)
{
	uint32_t secs;
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
sec2rtc(uint32_t secs, Rtc *rtc)
{
	int d;
	int32_t hms, day;
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

uint8_t
nvramread(int addr)
{
	uint8_t data;

	ilock(&nvrtlock);
	outb(Paddr, addr);
	data = inb(Pdata);
	iunlock(&nvrtlock);

	return data;
}

void
nvramwrite(int addr, uint8_t data)
{
	ilock(&nvrtlock);
	outb(Paddr, addr);
	outb(Pdata, data);
	iunlock(&nvrtlock);
}
