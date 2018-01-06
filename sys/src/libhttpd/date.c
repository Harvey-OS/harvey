/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <httpd.h>

/*
 * print dates in the format
 * Wkd, DD Mon YYYY HH:MM:SS GMT
 * parse dates of formats
 * Wkd, DD Mon YYYY HH:MM:SS GMT
 * Weekday, DD-Mon-YY HH:MM:SS GMT
 * Wkd Mon ( D|DD) HH:MM:SS YYYY
 * plus anything similar
 */
static char *
weekdayname[7] =
{
	"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};
static char *
wdayname[7] =
{
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static char *
monname[12] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static	int	dateindex(char*, char**, int);

static int
dtolower(int c)
{
	if(c >= 'A' && c <= 'Z')
		return c - 'A' + 'a';
	return c;
}

static int
disalpha(int c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static int
disdig(int c)
{
	return c >= '0' && c <= '9';
}

int
hdatefmt(Fmt *f)
{
	Tm *tm;
	uint32_t t;

	t = va_arg(f->args, uint32_t);
	tm = gmtime(t);
	return fmtprint(f, "%s, %.2d %s %.4d %.2d:%.2d:%.2d GMT",
		wdayname[tm->wday], tm->mday, monname[tm->mon], tm->year+1900,
		tm->hour, tm->min, tm->sec);
}

static char*
dateword(char *date, char *buf)
{
	char *p;
	int c;

	p = buf;
	while(!disalpha(c = *date) && !disdig(c) && c)
		date++;
	while(disalpha(c = *date)){
		if(p - buf < 30)
			*p++ = dtolower(c);
		date++;
	}
	*p = 0;
	return date;
}

static int
datenum(char **d)
{
	char *date;
	int c, n;

	date = *d;
	while(!disdig(c = *date) && c)
		date++;
	if(c == 0){
		*d = date;
		return -1;
	}
	n = 0;
	while(disdig(c = *date)){
		n = n * 10 + c - '0';
		date++;
	}
	*d = date;
	return n;
}

/*
 * parse a date and return the seconds since the epoch
 * return 0 for a failure
 */
uint32_t
hdate2sec(char *date)
{
	Tm tm;
	char buf[32];

	/*
	 * Weekday|Wday
	 */
	date = dateword(date, buf);
	tm.wday = dateindex(buf, wdayname, 7);
	if(tm.wday < 0)
		tm.wday = dateindex(buf, weekdayname, 7);
	if(tm.wday < 0)
		return 0;

	/*
	 * check for the two major formats
	 */
	date = dateword(date, buf);
	tm.mon = dateindex(buf, monname, 12);
	if(tm.mon >= 0){
		/*
		 * MM
		 */
		tm.mday = datenum(&date);
		if(tm.mday < 1 || tm.mday > 31)
			return 0;

		/*
		 * HH:MM:SS
		 */
		tm.hour = datenum(&date);
		if(tm.hour < 0 || tm.hour >= 24)
			return 0;
		tm.min = datenum(&date);
		if(tm.min < 0 || tm.min >= 60)
			return 0;
		tm.sec = datenum(&date);
		if(tm.sec < 0 || tm.sec >= 60)
			return 0;

		/*
		 * YYYY
		 */
		tm.year = datenum(&date);
		if(tm.year < 70 || (tm.year > 99 && tm.year < 1970))
			return 0;
		if(tm.year >= 1970)
			tm.year -= 1900;
	}else{
		/*
		 * MM-Mon-(YY|YYYY)
		 */
		tm.mday = datenum(&date);
		if(tm.mday < 1 || tm.mday > 31)
			return 0;
		date = dateword(date, buf);
		tm.mon = dateindex(buf, monname, 12);
		if(tm.mon < 0 || tm.mon >= 12)
			return 0;
		tm.year = datenum(&date);
		if(tm.year < 70 || (tm.year > 99 && tm.year < 1970))
			return 0;
		if(tm.year >= 1970)
			tm.year -= 1900;

		/*
		 * HH:MM:SS
		 */
		tm.hour = datenum(&date);
		if(tm.hour < 0 || tm.hour >= 24)
			return 0;
		tm.min = datenum(&date);
		if(tm.min < 0 || tm.min >= 60)
			return 0;
		tm.sec = datenum(&date);
		if(tm.sec < 0 || tm.sec >= 60)
			return 0;

		/*
		 * timezone
		 */
		dateword(date, buf);
		if(strncmp(buf, "gmt", 3) != 0)
			return 0;
	}

	strcpy(tm.zone, "GMT");
	tm.tzoff = 0;
	tm.yday = 0;
	return tm2sec(&tm);
}

static int
dateindex(char *d, char **tab, int n)
{
	int i;

	for(i = 0; i < n; i++)
		if(cistrcmp(d, tab[i]) == 0)
			return i;
	return -1;
}
