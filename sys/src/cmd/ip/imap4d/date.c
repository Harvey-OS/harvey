#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include "imap4d.h"

char *
wdayname[7] =
{
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char *
monname[12] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static void	time2tm(Tm *tm, char *s);
static void	zone2tm(Tm *tm, char *s);
static int	dateindex(char *d, char **tab, int n);

int
rfc822date(char *s, int n, Tm *tm)
{
	char *plus;
	int m;

	plus = "+";
	if(tm->tzoff < 0)
		plus = "";
	m = 0;
	if(0 <= tm->wday && tm->wday < 7){
		m = snprint(s, n, "%s, ", wdayname[tm->wday]);
		if(m < 0)
			return m;
	}
	return snprint(s+m, n-m, "%.2d %s %.4d %.2d:%.2d:%.2d %s%.4d",
		tm->mday, monname[tm->mon], tm->year+1900, tm->hour, tm->min, tm->sec,
		plus, (tm->tzoff/3600)*100 + (tm->tzoff/60)%60);
}

int
imap4date(char *s, int n, Tm *tm)
{
	char *plus;

	plus = "+";
	if(tm->tzoff < 0)
		plus = "";
	return snprint(s, n, "%2d-%s-%.4d %2.2d:%2.2d:%2.2d %s%4.4d",
		tm->mday, monname[tm->mon], tm->year+1900, tm->hour, tm->min, tm->sec, plus, (tm->tzoff/3600)*100 + (tm->tzoff/60)%60);
}

int
imap4Date(Tm *tm, char *date)
{
	char *flds[4];

	if(getfields(date, flds, 3, 0, "-") != 3)
		return 0;

	tm->mday = strtol(flds[0], nil, 10);
	tm->mon = dateindex(flds[1], monname, 12);
	tm->year = strtol(flds[2], nil, 10) - 1900;
	return 1;
}

/*
 * parse imap4 dates
 */
ulong
imap4DateTime(char *date)
{
	Tm tm;
	char *flds[4], *sflds[4];
	ulong t;

	if(getfields(date, flds, 4, 0, " ") != 3)
		return ~0;

	if(!imap4Date(&tm, flds[0]))
		return ~0;

	if(getfields(flds[1], sflds, 3, 0, ":") != 3)
		return ~0;

	tm.hour = strtol(sflds[0], nil, 10);
	tm.min = strtol(sflds[1], nil, 10);
	tm.sec = strtol(sflds[2], nil, 10);

	strcpy(tm.zone, "GMT");
	tm.yday = 0;
	t = tm2sec(&tm);
	zone2tm(&tm, flds[2]);
	t -= tm.tzoff;
	return t;
}

/*
 * parse dates of formats
 * [Wkd[,]] DD Mon YYYY HH:MM:SS zone
 * [Wkd] Mon ( D|DD) HH:MM:SS zone YYYY
 * plus anything similar
 * return nil for a failure
 */
Tm*
date2tm(Tm *tm, char *date)
{
	Tm gmt, *atm;
	char *flds[7], *s, dstr[64];
	int n;

	/*
	 * default date is Thu Jan  1 00:00:00 GMT 1970
	 */
	tm->wday = 4;
	tm->mday = 1;
	tm->mon = 1;
	tm->hour = 0;
	tm->min = 0;
	tm->sec = 0;
	tm->year = 70;
	strcpy(tm->zone, "GMT");
	tm->tzoff = 0;

	strncpy(dstr, date, sizeof(dstr));
	dstr[sizeof(dstr)-1] = '\0';
	n = tokenize(dstr, flds, 7);
	if(n != 6 && n != 5)
		return nil;

	if(n == 5){
		for(n = 5; n >= 1; n--)
			flds[n] = flds[n - 1];
		n = 5;
	}else{
		/*
		 * Wday[,]
		 */
		s = strchr(flds[0], ',');
		if(s != nil)
			*s = '\0';
		tm->wday = dateindex(flds[0], wdayname, 7);
		if(tm->wday < 0)
			return nil;
	}

	/*
	 * check for the two major formats:
	 * Month first or day first
	 */
	tm->mon = dateindex(flds[1], monname, 12);
	if(tm->mon >= 0){
		tm->mday = strtoul(flds[2], nil, 10);
		time2tm(tm, flds[3]);
		zone2tm(tm, flds[4]);
		tm->year = strtoul(flds[5], nil, 10);
		if(strlen(flds[5]) > 2)
			tm->year -= 1900;
	}else{
		tm->mday = strtoul(flds[1], nil, 10);
		tm->mon = dateindex(flds[2], monname, 12);
		tm->year = strtoul(flds[3], nil, 10);
		if(strlen(flds[3]) > 2)
			tm->year -= 1900;
		time2tm(tm, flds[4]);
		zone2tm(tm, flds[5]);
	}

	if(n == 5){
		gmt = *tm;
		strncpy(gmt.zone, "", 4);
		gmt.tzoff = 0;
		atm = gmtime(tm2sec(&gmt));
		tm->wday = atm->wday;
	}else{
		/*
		 * Wday[,]
		 */
		s = strchr(flds[0], ',');
		if(s != nil)
			*s = '\0';
		tm->wday = dateindex(flds[0], wdayname, 7);
		if(tm->wday < 0)
			return nil;
	}
	return tm;
}

/*
 * zone	: [A-Za-z][A-Za-z][A-Za-z]	some time zone names
 *	| [A-IK-Z]			military time; rfc1123 says the rfc822 spec is wrong.
 *	| "UT"				universal time
 *	| [+-][0-9][0-9][0-9][0-9]
 * zones is the rfc-822 list of time zone names
 */
static NamedInt zones[] =
{
	{"A",	-1 * 3600},
	{"B",	-2 * 3600},
	{"C",	-3 * 3600},
	{"CDT", -5 * 3600},
	{"CST", -6 * 3600},
	{"D",	-4 * 3600},
	{"E",	-5 * 3600},
	{"EDT", -4 * 3600},
	{"EST", -5 * 3600},
	{"F",	-6 * 3600},
	{"G",	-7 * 3600},
	{"GMT", 0},
	{"H",	-8 * 3600},
	{"I",	-9 * 3600},
	{"K",	-10 * 3600},
	{"L",	-11 * 3600},
	{"M",	-12 * 3600},
	{"MDT", -6 * 3600},
	{"MST", -7 * 3600},
	{"N",	+1 * 3600},
	{"O",	+2 * 3600},
	{"P",	+3 * 3600},
	{"PDT", -7 * 3600},
	{"PST", -8 * 3600},
	{"Q",	+4 * 3600},
	{"R",	+5 * 3600},
	{"S",	+6 * 3600},
	{"T",	+7 * 3600},
	{"U",	+8 * 3600},
	{"UT",	0},
	{"V",	+9 * 3600},
	{"W",	+10 * 3600},
	{"X",	+11 * 3600},
	{"Y",	+12 * 3600},
	{"Z",	0},
	{nil,	0}
};

static void
zone2tm(Tm *tm, char *s)
{
	Tm aux, *atm;
	int i;

	if(*s == '+' || *s == '-'){
		i = strtol(s, &s, 10);
		tm->tzoff = (i / 100) * 3600 + i % 100;
		strncpy(tm->zone, "", 4);
		return;
	}

	/*
	 * look it up in the standard rfc822 table
	 */
	strncpy(tm->zone, s, 3);
	tm->zone[3] = '\0';
	tm->tzoff = 0;
	for(i = 0; zones[i].name != nil; i++){
		if(cistrcmp(zones[i].name, s) == 0){
			tm->tzoff = zones[i].v;
			return;
		}
	}

	/*
	 * one last try: look it up in the current local timezone
	 * probe a couple of times to get daylight/standard time change.
	 */
	aux = *tm;
	memset(aux.zone, 0, 4);
	aux.hour--;
	for(i = 0; i < 2; i++){
		atm = localtime(tm2sec(&aux));
		if(cistrcmp(tm->zone, atm->zone) == 0){
			tm->tzoff = atm->tzoff;
			return;
		}
		aux.hour++;
	}

	strncpy(tm->zone, "GMT", 4);
	tm->tzoff = 0;
}

/*
 * hh[:mm[:ss]]
 */
static void
time2tm(Tm *tm, char *s)
{
	tm->hour = strtoul(s, &s, 10);
	if(*s++ != ':')
		return;
	tm->min = strtoul(s, &s, 10);
	if(*s++ != ':')
		return;
	tm->sec = strtoul(s, &s, 10);
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
