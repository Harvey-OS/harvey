#include "all.h"

Timet
toytime(void)
{
	return time(nil);
}

void
datestr(char *s, Timet t)
{
	Tm *tm;

	tm = localtime(t);
	sprint(s, "%.4d%.2d%.2d", tm->year+1900, tm->mon+1, tm->mday);
}

void
prdate(void)
{
	print("%T\n", time(nil));
}

static void
ct_numb(char *cp, int n)
{
	if(n >= 10)
		cp[0] = (n/10)%10 + '0';
	else
		cp[0] = ' ';
	cp[1] = n%10 + '0';
}

int
Tfmt(Fmt* fmt)
{
	char s[30];
	char *cp;
	Timet t;
	Tm *tm;

	t = va_arg(fmt->args, Timet);
	if(t == 0)
		return fmtstrcpy(fmt, "The Epoch");

	tm = localtime(t);
	strcpy(s, "Day Mon 00 00:00:00 1900");
	cp = &"SunMonTueWedThuFriSat"[tm->wday*3];
	s[0] = cp[0];
	s[1] = cp[1];
	s[2] = cp[2];
	cp = &"JanFebMarAprMayJunJulAugSepOctNovDec"[tm->mon*3];
	s[4] = cp[0];
	s[5] = cp[1];
	s[6] = cp[2];
	ct_numb(s+8, tm->mday);
	ct_numb(s+11, tm->hour+100);
	ct_numb(s+14, tm->min+100);
	ct_numb(s+17, tm->sec+100);
	if(tm->year >= 100) {
		s[20] = '2';
		s[21] = '0';
	}
	ct_numb(s+22, tm->year+100);

	return fmtstrcpy(fmt, s);
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
	Tm *tm;

	if(hr < 0 || hr >= 24)
		hr = 5;
	if((day&0x7f) == 0x7f)
		day = 0;
	for (;;) {
		tm = localtime(t);
		t -= tm->sec;
		t -= tm->min*60;
		nhr = tm->hour;
		do {
			t += 60*60;
			nhr++;
		} while(nhr%24 != hr);
		tm = localtime(t);
		if(tm->hour != hr) {
			t += 60*60;
			tm = localtime(t);
			if(tm->hour != hr) {
				t -= 60*60;
				tm = localtime(t);
			}
		}
		if(day & (1<<tm->wday))
			t += 12*60*60;
		else
			return t;
	}
}

/*
 *  delay for l milliseconds more or less.
 */
void
delay(int l)
{
	sleep(l);
}
