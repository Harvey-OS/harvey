#include	"all.h"
#include	"mem.h"

ulong
toytime(void)
{
	return mktime + TK2SEC(MACHP(0)->ticks);
}

#define SEC2MIN 60L
#define SEC2HOUR (60L*SEC2MIN)
#define SEC2DAY (24L*SEC2HOUR)

/*
 *  days per month plus days/year
 */
int	rtdmsize[] =
{
	365, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
int	rtldmsize[] =
{
	366, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/*
 *  return the days/month for the given year
 */
int*
yrsize(int y)
{

	if((y%4) == 0 && ((y%100) != 0 || (y%400) == 0))
		return rtldmsize;
	else
		return rtdmsize;
}

void
sec2rtc(ulong secs, Rtc *rtc)
{
	long hms, day;
	int d, *d2m;

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
		for(d = 1970; day < 0; d--)
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

ulong
rtc2sec(Rtc *rtc)
{
	ulong secs;
	int i, *d2m;

	secs = 0;

	/*
	 *  seconds per year
	 */
	for(i = 1970; i < rtc->year; i++) {
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

ulong
time(void)
{
	ulong t;
	long dt;

	t = toytime();
	while(tim.bias != 0) {				/* adjust at rate 1 sec/min */
		dt = t - tim.lasttoy;
		if(dt < MINUTE(1))
			break;
		if(tim.bias >= 0) {
			tim.bias -= SECOND(1);
			tim.offset += SECOND(1);
		} else {
			tim.bias += SECOND(1);
			tim.offset -= SECOND(1);
		}
		tim.lasttoy += MINUTE(1);
	}
	return t + tim.offset;
}

void
settime(ulong nt)
{
	long dt;

	dt = nt - time();
	tim.lasttoy = toytime();
	if(dt > MAXBIAS || dt < -MAXBIAS) {		/* too much, just set it */
		tim.bias = 0;
		tim.offset = nt - tim.lasttoy;
	} else
		tim.bias = dt;
}

void
prdate(void)
{
	ulong t;

	t = time();
	if(tim.bias >= 0)
		print("%T + %ld\n", t, tim.bias);
	else
		print("%T - %ld\n", t, -tim.bias);
}

static	int	dysize(int);
static	void	ct_numb(char*, int);
void		localtime(ulong, Tm*);
void		gmtime(ulong, Tm*);

static	char	dmsize[12] =
{
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/*
 * The following table is used for 1974 and 1975 and
 * gives the day number of the first day after the Sunday of the
 * change.
 */
static	struct
{
	short	yrfrom;
	short	yrto;
	short	daylb;
	short	dayle;
} daytab[] =
{
	87,	999,	90,	303,
	76,	86,	119,	303,
	75,	75,	58,	303,
	74,	74,	5,	333,
	0,	73,	119,	303,
};

static struct
{
	short	minuteswest;	/* minutes west of Greenwich */
	short	dsttime;	/* dst correction */
} timezone =
{
	5*60, 1
};

static
prevsunday(Tm *t, int d)
{
	if(d >= 58)
		d += dysize(t->year) - 365;
	return d - (d - t->yday + t->wday + 700) % 7;
}

static
succsunday(Tm *t, int d)
{
	int dd;

	if(d >= 58)
		d += dysize(t->year) - 365;
	dd = (d - t->yday + t->wday + 700) % 7;
	if(dd == 0)
		return d;
	else
		return d + 7 - dd;
}

void
localtime(ulong tim, Tm *ct)
{
	int daylbegin, daylend, dayno, i;
	ulong copyt;

	copyt = tim - timezone.minuteswest*60L;
	gmtime(copyt, ct);
	dayno = ct->yday;
	for(i=0;; i++)
		if(ct->year >= daytab[i].yrfrom &&
		   ct->year <= daytab[i].yrto) {
			daylbegin = succsunday(ct, daytab[i].daylb);
			daylend = prevsunday(ct, daytab[i].dayle);
			break;
		}
	if(timezone.dsttime &&
	    (dayno>daylbegin || (dayno==daylbegin && ct->hour>=2)) &&
	    (dayno<daylend || (dayno==daylend && ct->hour<1))) {
		copyt += 60L*60L;
		gmtime(copyt, ct);
		ct->isdst++;
	}
}

void
gmtime(ulong tim, Tm *ct)
{
	int d0, d1;
	long hms, day;

	/*
	 * break initial number into days
	 */
	hms = tim % 86400L;
	day = tim / 86400L;
	if(hms < 0) {
		hms += 86400L;
		day -= 1;
	}

	/*
	 * generate hours:minutes:seconds
	 */
	ct->sec = hms % 60;
	d1 = hms / 60;
	ct->min = d1 % 60;
	d1 /= 60;
	ct->hour = d1;

	/*
	 * day is the day number.
	 * generate day of the week.
	 * The addend is 4 mod 7 (1/1/1970 was Thursday)
	 */

	ct->wday = (day + 7340036L) % 7;

	/*
	 * year number
	 */
	if(day >= 0)
		for(d1 = 1970; day >= dysize(d1); d1++)
			day -= dysize(d1);
	else
		for(d1 = 1970; day < 0; d1--)
			day += dysize(d1-1);
	ct->year = d1-1900;
	ct->yday = d0 = day;

	/*
	 * generate month
	 */
	if(dysize(d1) == 366)
		dmsize[1] = 29;
	for(d1 = 0; d0 >= dmsize[d1]; d1++)
		d0 -= dmsize[d1];
	dmsize[1] = 28;
	ct->mday = d0 + 1;
	ct->mon = d1;
	ct->isdst = 0;
}

void
datestr(char *s, ulong t)
{
	Tm tm;

	localtime(t, &tm);
	sprint(s, "%.4d%.2d%.2d", tm.year+1900, tm.mon+1, tm.mday);
}

int
Tfmt(Fmt* fmt)
{
	char s[30];
	char *cp;
	ulong t;
	Tm tm;

	t = va_arg(fmt->args, ulong);
	if(t == 0)
		return fmtstrcpy(fmt, "The Epoch");

	localtime(t, &tm);
	strcpy(s, "Day Mon 00 00:00:00 1900");
	cp = &"SunMonTueWedThuFriSat"[tm.wday*3];
	s[0] = cp[0];
	s[1] = cp[1];
	s[2] = cp[2];
	cp = &"JanFebMarAprMayJunJulAugSepOctNovDec"[tm.mon*3];
	s[4] = cp[0];
	s[5] = cp[1];
	s[6] = cp[2];
	ct_numb(s+8, tm.mday);
	ct_numb(s+11, tm.hour+100);
	ct_numb(s+14, tm.min+100);
	ct_numb(s+17, tm.sec+100);
	if(tm.year >= 100) {
		s[20] = '2';
		s[21] = '0';
	}
	ct_numb(s+22, tm.year+100);

	return fmtstrcpy(fmt, s);
}

static
dysize(int y)
{

	if((y%4) == 0)
		return 366;
	return 365;
}

static
void
ct_numb(char *cp, int n)
{

	if(n >= 10)
		cp[0] = (n/10)%10 + '0';
	else
		cp[0] = ' ';
	cp[1] = n%10 + '0';
}

/*
 * compute the next time after t
 * that has hour hr and is not on
 * day in bitpattern --
 * for automatic dumps
 */
ulong
nextime(ulong t, int hr, int day)
{
	Tm tm;
	int nhr;

	if(hr < 0 || hr >= 24)
		hr = 5;
	if((day&0x7f) == 0x7f)
		day = 0;

loop:
	localtime(t, &tm);
	t -= tm.sec;
	t -= tm.min*60;
	nhr = tm.hour;
	do {
		t += 60*60;
		nhr++;
	} while(nhr%24 != hr);
	localtime(t, &tm);
	if(tm.hour != hr) {
		t += 60*60;
		localtime(t, &tm);
		if(tm.hour != hr) {
			t -= 60*60;
			localtime(t, &tm);
		}
	}
	if(day & (1<<tm.wday)) {
		t += 12*60*60;
		goto loop;
	}
	return t;
}
