#include	"all.h"
#include	"mem.h"

Timet
toytime(void)
{
	return mktime + TK2SEC(MACHP(0)->ticks);
}

#define SEC2MIN 60L
#define SEC2HOUR (60L*SEC2MIN)
#define SEC2DAY  (24L*SEC2HOUR)

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
sec2rtc(Timet secs, Rtc *rtc)
{
	int d, *d2m;
	Timet hms, day;

	/*
	 * break initial number into days
	 */
	hms = (uvlong)secs % SEC2DAY;
	day = (uvlong)secs / SEC2DAY;
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

Timet
rtc2sec(Rtc *rtc)
{
	Timet secs;
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

Timet
time(void)
{
	Timet t, dt;

	t = toytime();
	while(tim.bias != 0) {			/* adjust at rate 1 sec/min */
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
settime(Timet nt)
{
	Timet dt;

	dt = nt - time();
	tim.lasttoy = toytime();
	if(dt > MAXBIAS || dt < -MAXBIAS) {	/* too much, just set it */
		tim.bias = 0;
		tim.offset = nt - tim.lasttoy;
	} else
		tim.bias = dt;
}

void
prdate(void)
{
	Timet t;

	t = time();
	if(tim.bias >= 0)
		print("%T + %ld\n", t, tim.bias);
	else
		print("%T - %ld\n", t, -tim.bias);
}

static	int	dysize(int);
static	void	ct_numb(char*, int);
void		localtime(Timet, Tm*);
void		gmtime(Timet, Tm*);

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
	107,	~(ushort)0>>1, 66, 310,
	87,	106,	90,	303,
	76,	86,	119,	303,
	75,	75,	58,	303,
	74,	74,	5,	333,
	-1,	73,	119,	303,
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
localtime(Timet tim, Tm *ct)
{
	int daylbegin, daylend, dayno, i;
	Timet copyt;

	copyt = tim - conf.minuteswest*60L;
	gmtime(copyt, ct);
	dayno = ct->yday;

	/* enforce sane bounds for daytab */
	if (ct->year < -1) /* 1 jan 1970 00:00 GMT can be 31 dec 1969 locally */
		ct->year = -1;
	else if (ct->year > 60000)
		ct->year = 60000;

	for(i = 0; ; i++)
		if(ct->year >= daytab[i].yrfrom &&
		   ct->year <= daytab[i].yrto) {
			daylbegin = succsunday(ct, daytab[i].daylb);
			daylend = prevsunday(ct, daytab[i].dayle);
			break;
		}
	if(conf.dsttime &&
	    (dayno>daylbegin || (dayno==daylbegin && ct->hour >= 2)) &&
	    (dayno<daylend   || (dayno==daylend   && ct->hour < 1))) {
		copyt += 60L*60L;
		gmtime(copyt, ct);
		ct->isdst++;
	}
}

void
gmtime(Timet tim, Tm *ct)
{
	int d0, d1;
	Timet hms, day;

	/*
	 * break initial number into days
	 */
	hms = (uvlong)tim % 86400L;
	day = (uvlong)tim / 86400L;
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
datestr(char *s, Timet t)
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
	Timet t;
	Tm tm;

	t = va_arg(fmt->args, Timet);
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
Timet
nextime(Timet t, int hr, int day)
{
	int nhr;
	Tm tm;

	if(hr < 0 || hr >= 24)
		hr = 5;
	if((day&0x7f) == 0x7f)
		day = 0;
	for (;;) {
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
		if(day & (1<<tm.wday))
			t += 12*60*60;
		else
			return t;
	}
}
