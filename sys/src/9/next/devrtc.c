#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"

#include	"io.h"

typedef struct Rtc	Rtc;
typedef struct Time	Time;

struct Rtc
{
	QLock;
	int	new;		/* is a new chip */
};

struct Time
{
	int	sec;
	int	min;
	int	hour;
	int	mday;
	int	mon;
	int	year;
};

Rtc	rtc;
enum{
	Qrtc = 1,
	Qnvram,
};

#define	NRTC	1
Dirtab rtcdir[]={
	"rtc",		{Qrtc, 0},	0,	0666,
};

ulong rtc2sec(Time*);
void sec2rtc(ulong, Time*);
int *yrsize(int);

void
rtcreset(void)
{
	qlock(&rtc);
	qunlock(&rtc);
	if(getcc(0x30) & 0x80)
		rtc.new = 1;
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

#define GETBCD(v) (((v)&0xf) + 10*((v)>>4))
#define TOBCD(v) ((v) % 10) | ((((v) / 10) % 10)<<4)

long
rtctime(void)
{
	ulong t;
	Time r;

	if(rtc.new){
		t  = getcc(0x20)<<24;
		t |= getcc(0x21)<<16;
		t |= getcc(0x22)<< 8;
		t |= getcc(0x23)<< 0;
	}else{
	
		/*
		 *  convert from BCD
		 */
		r.sec = GETBCD(getcc(0x20)&0x7F);
		r.min = GETBCD(getcc(0x21)&0x7F);
		t = getcc(0x22);
		if(t & (1<<7)){	/* 12-hour mode */
			r.hour = GETBCD(t&0x1F);
			if(t & (1<<5))
				r.hour += 12;
		}else
			r.hour = GETBCD(t&0x7F);
		r.mday = GETBCD(getcc(0x24)&0x3F);
		r.mon = GETBCD(getcc(0x25)&0x1F);
		r.year = GETBCD(getcc(0x26));
	
		/*
		 *  the world starts jan 1 1970
		 */
		if(r.year < 70)
			r.year += 2000;
		else
			r.year += 1900;
		t = rtc2sec(&r);
	}
	return t;
}

long	 
rtcread(Chan *c, void *buf, long n, ulong offset)
{
	ulong t, ot;

	if(c->qid.path & CHDIR)
		return devdirread(c, buf, n, rtcdir, NRTC, devgen);

	qlock(&rtc);
	t = rtctime();
	do{
		ot = t;
		t = rtctime();	/* make sure there's no skew */
	}while(t != ot);
	qunlock(&rtc);
	n = readnum(offset, buf, n, t, 12);
	return n;

}

long	 
rtcwrite(Chan *c, void *buf, long n, ulong offset)
{
	Time r;
	ulong t;
	int i, cc;
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
	t = strtoul(cp, 0, 0);

	qlock(&rtc);
	cc = getcc(0x31);
	cc &= ~(1<<7);
	putcc(0x31, cc);		/* stop clock */
	if(rtc.new)
		for(i=4; --i>=0; )
			putcc(0x20+i, (t>>(24-8*i))&0xFF);
	else{
		sec2rtc(t, &r);
		putcc(0x20, TOBCD(r.sec));
		putcc(0x21, TOBCD(r.min));
		putcc(0x22, (1<<7)|TOBCD(r.hour));	/* put in 24hr mode */
		putcc(0x23, 0);
		putcc(0x24, TOBCD(r.mday));
		putcc(0x25, TOBCD(r.mon));
		putcc(0x26, TOBCD(r.year%100));
	}
	putcc(0x31, cc|(1<<7));		/* start clock */
	qunlock(&rtc);

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
rtc2sec(Time *t)
{
	ulong secs;
	int i;
	int *d2m;

	secs = 0;

	/*
	 *  seconds per year
	 */
	for(i = 1970; i < t->year; i++){
		d2m = yrsize(i);
		secs += d2m[0] * SEC2DAY;
	}

	/*
	 *  seconds per month
	 */
	d2m = yrsize(t->year);
	for(i = 1; i < t->mon; i++)
		secs += d2m[i] * SEC2DAY;

	secs += (t->mday-1) * SEC2DAY;
	secs += t->hour * SEC2HOUR;
	secs += t->min * SEC2MIN;
	secs += t->sec;

	return secs;
}

/*
 *  compute rtc from seconds since Jan 1 1970
 */
void
sec2rtc(ulong secs, Time *t)
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
	t->sec = hms % 60;
	d = hms / 60;
	t->min = d % 60;
	d /= 60;
	t->hour = d;

	/*
	 * year number
	 */
	if(day >= 0)
		for(d = 1970; day >= *yrsize(d); d++)
			day -= *yrsize(d);
	else
		for (d = 1970; day < 0; d--)
			day += *yrsize(d-1);
	t->year = d;

	/*
	 * generate month
	 */
	d2m = yrsize(t->year);
	for(d = 1; day >= d2m[d]; d++)
		day -= d2m[d];
	t->mday = day + 1;
	t->mon = d;
}
