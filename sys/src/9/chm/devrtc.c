#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"

#include	"io.h"

#define RONVRAM		1

typedef struct Rtc	Rtc;
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

enum{
	Qrtc = 1,
	Qnvram,
};

QLock	rtclock;		/* mutex on clock operations */

static Dirtab rtcdir[]={
	"rtc",		{Qrtc, 0},	0,	0666,
	"nvram",	{Qnvram, 0},	0,	0666,
};
#define	NRTC	(sizeof(rtcdir)/sizeof(rtcdir[0]))

	void	setrtc(Rtc*);
	ulong	rtc2sec(Rtc*);
	void	sec2rtc(ulong, Rtc*);
	long	rtctime(void);
static ulong	nvget16(int reg);

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

long	 
rtcread(Chan *c, void *buf, long n, ulong offset)
{
	ulong t, ot;
	int i;

	if(c->qid.path & CHDIR)
		return devdirread(c, buf, n, rtcdir, NRTC, devgen);

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
		qlock(&rtclock);
		i = getnvram(offset, buf, n);
		qunlock(&rtclock);
		return i;
	}
	error(Egreg);
	return 0;		/* not reached */
}

long	 
rtcwrite(Chan *c, void *buf, long n, ulong offset)
{
	Rtc rtc;
	ulong secs;
	char *cp, *ep;
	int i;

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
		if(RONVRAM)
			error(Eperm);
		qlock(&rtclock);
		i = putnvram(offset, buf, n);
		qunlock(&rtclock);
		if(i < 0)
			error(Eio);
		return n;
	}
	error(Egreg);
	return 0;		/* not reached */
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

#define bcd2dec(bcd)	(((((bcd)>>4) & 0x0F) * 10) + ((bcd) & 0x0F))
#define dec2bcd(dec)	((((dec)/10)<<4)|((dec)%10))

void
setrtc(Rtc *rtc)
{
	RTCdev *dev;

	dev = IO(RTCdev, HPC3_RTC);

	dev->status = 0;	/* register set 0 */
	dev->savctl = 0;
	dev->pflag = 0;
	dev->status = 1<<6;	/* register set 1 */
	dev->int1ctl = 0x80;	/* enable power-fail interrupt */
	dev->outmode = 0;
	dev->rtime &= ~(3<<3);	/* stop */

	dev->year = dec2bcd(rtc->year - 1970);
	dev->mon = dec2bcd(rtc->mon);
	dev->mday = dec2bcd(rtc->mday);
	dev->hour = dec2bcd(rtc->hour);
	dev->min = dec2bcd(rtc->min);
	dev->sec = dec2bcd(rtc->sec);
	dev->hsec = 0;
	dev->rtime = (3<<3) | (rtc->year % 4);	/* start */
}

long
rtctime(void)
{
	Rtc rtc;
	RTCdev *dev;

	dev = IO(RTCdev, HPC3_RTC);

	rtc.sec = bcd2dec(dev->sec) & 0x7F;
	rtc.min = bcd2dec(dev->min & 0x7F);
	rtc.hour = bcd2dec(dev->hour & 0x3F);
	rtc.mday = bcd2dec(dev->mday & 0x3F);
	rtc.mon = bcd2dec(dev->mon & 0x3F);
	rtc.year = bcd2dec(dev->year);

	if (rtc.mon < 1 || rtc.mon > 12)
		return 0;
	/*
	 *  the world starts Jan 1 1970
	 */
	rtc.year += 1970;

	return rtc2sec(&rtc);
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

enum
{
	CPUSER		= 0x08,
	SERCLK		= 0x04,
	CONSCS		= 0x02,
	NVPRE		= 0x01,
	SERDATA		= 0x10,

	/* Commands */
	SER_READ	= (0xc0<<8),
	SER_WEN		= (0x98<<8),
	SER_WDS		= (0x80<<8),
	SER_WRITE	= (0xa0<<8),
};

static void
nvenable(int onoff)
{
	uchar *d;

	d = IO(uchar, HPC3_NVRTC);
	if(onoff) {
		*d &= ~CPUSER;
		*d &= ~SERCLK;
		*d &= ~NVPRE;
		Xdelay(100);
		*d |= CONSCS;
		*d |= SERCLK;
	}
	else {
		*d &= ~SERCLK;
		*d &= ~CONSCS;
		*d |= NVPRE;
		*d |= SERCLK;
	}
}

static void
nvcmd(int cmd, int reg)
{
	int i;
	uchar *d;
	ulong scmd;

	d = IO(uchar, HPC3_NVRTC);

	scmd = cmd | (reg << 5);
	for(i = 0; i < 11; i++) {
		if(scmd & 0x8000)
			*d |= CPUSER;
		else
			*d &= ~CPUSER;
		*d &= ~SERCLK;
		*d |= SERCLK;
		scmd <<= 1;
	}
	*d &= ~CPUSER;
}

static ulong
nvget16(int reg)
{
	int i;
	uchar *d;
	ulong s, data;

	d = IO(uchar, HPC3_NVRTC);
	s = splhi();

	*d &= ~NVPRE;
	nvenable(1);
	nvcmd(SER_READ, reg);

	data = 0;
	for(i = 0; i < 16; i++) {
		*d &= ~SERCLK;
		*d |= SERCLK;
		data <<= 1;
		data |= (*d & SERDATA) ? 1 : 0;
	}
	nvenable(0);
	splx(s);
	return data;
}

static int
nvpoll(void)
{
	uchar *d;
	int timeout, x;

	d = IO(uchar, HPC3_NVRTC);
	timeout = 100;

	nvenable(1);
	while(timeout--) {
		if(*d & SERDATA)
			break;
		Xdelay(5);
	}
	x = *d & SERDATA;
	nvenable(0);
	return x;
}

static void
nvout(int cmd)
{
	nvenable(1);
	nvcmd(cmd, 0);
	nvenable(0);
}

static ulong
nvput16(int reg, ulong data)
{
	int i, timo;
	uchar *d;
	ulong s;

	d = IO(uchar, HPC3_NVRTC);
	s = splhi();

	*d &= ~NVPRE;
	nvout(SER_WEN);
	nvenable(1);
	nvcmd(SER_WRITE, reg);

	for(i = 0; i < 16; i++) {
		if(data & 0x8000)
			*d |= CPUSER;
		else
			*d &= ~CPUSER;
		*d &= ~SERCLK;
		*d |= SERCLK;
		data <<= 1;
	}
	*d &= ~CPUSER;
		
	nvenable(0);
	timo = nvpoll();
	nvout(SER_WDS);
	splx(s);

	return timo;
}

static char
nvsum(void)
{
	int i;
	uchar sum;
	ulong data;

	sum = 0xa5;

	for(i = 0; i < NVSIZE/2; i ++) {
		data = nvget16(i);
		if(i == (NVSUM/2))
			sum ^= data >> 8;
		else
			sum ^= (data>>8)^(data&0xff);

		if(sum & 0x80) {
			sum <<= 1;
			sum |= 1;
		}
		else
			sum <<= 1;
	}
	return sum;
}

int
putnvram(ulong offset, void *buf, int n)
{
	int i;
	uchar *p;
	ulong data;

	if(offset+n > NVSIZE)
		n = NVSIZE-offset;

	p = buf;
	for(i = 0; i < n; i++) {
		data = nvget16(offset/2);
		if((offset & 1) == 0)
			data = (data&0xFF00)|(p[i]&0x00FF);
		else
			data = (data&0x00FF)|(p[i]<<8);
		if(nvput16(offset/2, data) < 0)
			break;
		offset++;
	}
	if(i != 0)
		nvsum();
	return i;
}

int
getnvram(ulong offset, void *buf, int n)
{
	int i;
	uchar *p;
	ulong data;

	if(offset+n > NVSIZE)
		n = NVSIZE-offset;

	p = buf;
	for(i = 0; i < n; i++) {
		data = nvget16(offset/2);
		if((offset & 1) == 0)
			data >>= 8;
		p[i] = data;
		offset++;
	}

	return i;
}

void
getnveaddr(void *p)
{
	Etherpkt *e;

	getnvram(NVEADDR, p, sizeof(e->s));
}
