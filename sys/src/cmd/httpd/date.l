#include <alef.h>
#include <bio.h>
#include "httpd.h"

/*
 * print dates in the format
 * Wkd, DD Mon YYYY HH:MM:SS GMT
 * parse dates of formats
 * Wkd, DD Mon YYYY HH:MM:SS GMT
 * Weekday, DD-Mon-YY HH:MM:SS GMT
 * Wkd Mon ( D|DD) HH:MM:SS YYYY
 * plus anything similar
 */
byte *
weekdayname[7] =
{
	"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};
byte *
wdayname[7] =
{
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

byte *
monname[12] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

intern	int	dateindex(byte*, byte**, int);

intern int
tolower(int c)
{
	if(c >= 'A' && c <= 'Z')
		return c - 'A' + 'a';
	return c;
}

intern int
isalpha(int c)
{
	return c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z';
}

intern int
isdig(int c)
{
	return c >= '0' && c <= '9';
}

int
dateconv(Printspec *p)
{
	Tm *tm;
	byte buf[128];
	int t;

	t = *(int*)p->o;
	tm = gmtime(t);
	snprint(buf, sizeof(buf), "%s, %.2d %s %.4d %.2d:%.2d:%.2d GMT",
		wdayname[tm->wday], tm->mday, monname[tm->mon], tm->year+1900,
		tm->hour, tm->min, tm->sec);
	strconv(p, buf);
	return sizeof(int);
}

intern byte*
dateword(byte *date, byte *buf)
{
	byte *p;
	int c;

	p = buf;
	while(!isalpha(c = *date) && !isdig(c) && c)
		date++;
	while(isalpha(c = *date)){
		if(p - buf < 30)
			*p++ = tolower(c);
		date++;
	}
	*p = 0;
	return date;
}

intern (byte*, int)
datenum(byte *date)
{
	int c, n;

	while(!isdig(c = *date) && c)
		date++;
	if(c == 0)
		return (date, -1);
	n = 0;
	while(isdig(c = *date)){
		n = n * 10 + c - '0';
		date++;
	}
	return (date, n);
}

/*
 * parse a date and return the seconds since the epoch
 * return 0 for a failure
 */
uint
date2sec(byte *date)
{
	Tm tm;
	byte buf[32];

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
		(date, tm.mday) = datenum(date);
		if(tm.mday < 1 || tm.mday > 31)
			return 0;

		/*
		 * HH:MM:SS
		 */
		(date, tm.hour) = datenum(date);
		if(tm.hour < 0 || tm.hour >= 24)
			return 0;
		(date, tm.min) = datenum(date);
		if(tm.min < 0 || tm.min >= 60)
			return 0;
		(date, tm.sec) = datenum(date);
		if(tm.sec < 0 || tm.sec >= 60)
			return 0;

		/*
		 * YYYY
		 */
		(nil, tm.year) = datenum(date);
		if(tm.year < 70 || tm.year > 99 && tm.year < 1970)
			return 0;
		if(tm.year >= 1970)
			tm.year -= 1900;
	}else{
		/*
		 * MM-Mon-(YY|YYYY)
		 */
		(date, tm.mday) = datenum(date);
		if(tm.mday < 1 || tm.mday > 31)
			return 0;
		date = dateword(date, buf);
		tm.mon = dateindex(buf, monname, 12);
		if(tm.mon < 0 || tm.mon >= 12)
			return 0;
		(date, tm.year) = datenum(date);
		if(tm.year < 70 || tm.year > 99 && tm.year < 1970)
			return 0;
		if(tm.year >= 1970)
			tm.year -= 1900;

		/*
		 * HH:MM:SS
		 */
		(date, tm.hour) = datenum(date);
		if(tm.hour < 0 || tm.hour >= 24)
			return 0;
		(date, tm.min) = datenum(date);
		if(tm.min < 0 || tm.min >= 60)
			return 0;
		(date, tm.sec) = datenum(date);
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
	return gmtm2sec(tm);
}

intern int
dateindex(byte *d, byte **tab, int n)
{
	byte *s, *t;
	int i;

	for(i = 0; i < n; i++){
		t = tab[i];
		for(s = d; *s && tolower(*s) == tolower(*t); s++)
			t++;
		if(*s == 0 && *t == 0)
			return i;
	}
	return -1;
}
