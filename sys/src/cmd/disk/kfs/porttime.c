#include	"all.h"

static	int	sunday(Tm *t, int d);
static	int	dysize(int);
static	void	ct_numb(char*, int);
static	void	klocaltime(long tim, Tm *ct);
static	void	kgmtime(long tim, Tm *ct);

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
	87,	999,	97,	303,
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

static void
klocaltime(long tim, Tm *ct)
{
	int daylbegin, daylend, dayno, i;
	long copyt;

	copyt = tim - timezone.minuteswest*60L;
	kgmtime(copyt, ct);
	dayno = ct->yday;
	for(i=0;; i++)
		if(ct->year >= daytab[i].yrfrom &&
		   ct->year <= daytab[i].yrto) {
			daylbegin = sunday(ct, daytab[i].daylb);
			daylend = sunday(ct, daytab[i].dayle);
			break;
		}
	if(timezone.dsttime &&
	    (dayno>daylbegin || (dayno==daylbegin && ct->hour>=2)) &&
	    (dayno<daylend || (dayno==daylend && ct->hour<1))) {
		copyt += 60L*60L;
		kgmtime(copyt, ct);
	}
}

/*
 * The argument is a 0-origin day number.
 * The value is the day number of the last
 * Sunday before or after the day.
 */
static
sunday(Tm *t, int d)
{
	if(d >= 58)
		d += dysize(t->year) - 365;
	return d - (d - t->yday + t->wday + 700) % 7;
}

static void
kgmtime(long tim, Tm *ct)
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
		for(d1 = 70; day >= dysize(d1); d1++)
			day -= dysize(d1);
	else
		for (d1 = 70; day < 0; d1--)
			day += dysize(d1-1);
	ct->year = d1;
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
}

void
datestr(char *s, long t)
{
	Tm tm;

	klocaltime(t, &tm);
	sprint(s, "%.4d%.2d%.2d", tm.year+1900, tm.mon+1, tm.mday);
}

int
Tfmt(Fmt *f1)
{
	char s[30];
	char *cp;
	long t;
	Tm tm;

	t = va_arg(f1->args, long);
	if(t == 0)
		return fmtstrcpy(f1, "The Epoch");

	klocaltime(t, &tm);
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

	return fmtstrcpy(f1, s);
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
long
nextime(long t, int hr, int day)
{
	Tm tm;
	int nhr;

	if(hr < 0 || hr >= 24)
		hr = 5;
	if((day&0x7f) == 0x7f)
		day = 0;

loop:
	klocaltime(t, &tm);
	t -= tm.sec;
	t -= tm.min*60;
	nhr = tm.hour;
	do {
		t += 60*60;
		nhr++;
	} while(nhr%24 != hr);
	klocaltime(t, &tm);
	if(tm.hour != hr) {
		t += 60*60;
		klocaltime(t, &tm);
		if(tm.hour != hr) {
			t -= 60*60;
			klocaltime(t, &tm);
		}
	}
	if(day & (1<<tm.wday)) {
		t += 12*60*60;
		goto loop;
	}
	return t;
}
