#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"

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

QLock rtclock;	/* mutex on clock operations */

enum{
	Qrtc = 1,
	Qnvram,
};

#define	NRTC	1
Dirtab rtcdir[]={
	"rtc",		{Qrtc, 0},	0,	0666,
};

ulong rtc2sec(Rtc*);
void sec2rtc(ulong, Rtc*);
int *yrsize(int);

void
rtcreset(void)
{
}

void
rtcinit(void)
{
}

Chan*
rtcattach(char *spec)
{
	return devattach('r', spec);
}

Chan*
rtcclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int	 
rtcwalk(Chan *c, char *name)
{
	return devwalk(c, name, rtcdir, NRTC, devgen);
}

void	 
rtcstat(Chan *c, char *dp)
{
	devstat(c, dp, rtcdir, NRTC, devgen);
}

Chan*
rtcopen(Chan *c, int omode)
{
	omode = openmode(omode);
	switch(c->qid.path){
	case Qrtc:
		if(strcmp(u->p->user, eve)!=0 && omode!=OREAD)
			error(Eperm);
		break;
	case Qnvram:
		if(strcmp(u->p->user, eve)!=0 || !cpuserver)
			error(Eperm);
	}
	return devopen(c, omode, rtcdir, NRTC, devgen);
}

void	 
rtccreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void	 
rtcclose(Chan *c)
{
	USED(c);
}

#define GETBCD(o) ((bcdclock[o]&0xf) + 10*(bcdclock[o]>>4))

long	 
rtctime(void)
{
	uchar bcdclock[Nbcd];
	Rtc rtc;
	int i;

	for(i = 0; i < 10000; i++){
		outb(Paddr, Status);
		if((inb(Pdata) & 1) == 0)
			break;
	}
	outb(Paddr, Seconds);	bcdclock[0] = inb(Pdata);
	outb(Paddr, Minutes);	bcdclock[1] = inb(Pdata);
	outb(Paddr, Hours);	bcdclock[2] = inb(Pdata);
	outb(Paddr, Mday);	bcdclock[3] = inb(Pdata);
	outb(Paddr, Month);	bcdclock[4] = inb(Pdata);
	outb(Paddr, Year);	bcdclock[5] = inb(Pdata);

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

long	 
rtcread(Chan *c, void *buf, long n, ulong offset)
{
	ulong t, ot;

	if(c->qid.path & CHDIR)
		return devdirread(c, buf, n, rtcdir, NRTC, devgen);

	qlock(&rtclock);
	t = rtctime();
	do{
		ot = t;
		t = rtctime();	/* make sure there's no skew */
	}while(t != ot);
	qunlock(&rtclock);
	n = readnum(offset, buf, n, t, 12);
	return n;

}

#define PUTBCD(n,o) bcdclock[o] = (n % 10) | (((n / 10) % 10)<<4)

long	 
rtcwrite(Chan *c, void *buf, long n, ulong offset)
{
	Rtc rtc;
	ulong secs;
	uchar bcdclock[Nbcd];
	char *cp, *ep;

	USED(c);
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
	PUTBCD(rtc.sec, 0);
	PUTBCD(rtc.min, 1);
	PUTBCD(rtc.hour, 2);
	PUTBCD(rtc.mday, 3);
	PUTBCD(rtc.mon, 4);
	PUTBCD(rtc.year, 5);

	/*
	 *  write the clock
	 */
	qlock(&rtclock);
	outb(Paddr, Seconds);	outb(Pdata, bcdclock[0]);
	outb(Paddr, Minutes);	outb(Pdata, bcdclock[1]);
	outb(Paddr, Hours);	outb(Pdata, bcdclock[2]);
	outb(Paddr, Mday);	outb(Pdata, bcdclock[3]);
	outb(Paddr, Month);	outb(Pdata, bcdclock[4]);
	outb(Paddr, Year);	outb(Pdata, bcdclock[5]);
	qunlock(&rtclock);

	return n;
}

void	 
rtcremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void	 
rtcwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
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
int *
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
ulong
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
void
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
nvramread(int offset)
{
	outb(Paddr, offset);
	return inb(Pdata);
}
