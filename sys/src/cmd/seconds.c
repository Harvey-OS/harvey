/*
 * seconds absolute_date ... - convert absolute_date to seconds since epoch
 */

#include <u.h>
#include <libc.h>
#include <ctype.h>

typedef ulong Time;

enum {
	AM, PM, HR24,

	/* token types */
	Month = 1,
	Year,
	Day,
	Timetok,
	Tz,
	Dtz,
	Ignore,
	Ampm,

	Maxtok		= 6, /* only this many chars are stored in datetktbl */
	Maxdateflds	= 25,
};

/*
 * macros for squeezing values into low 7 bits of "value".
 * all timezones we care about are divisible by 10, and the largest value
 * (780) when divided is 78.
 */
#define TOVAL(tp, v)	((tp)->value = (v) / 10)
#define FROMVAL(tp)	((tp)->value * 10)	/* uncompress */

/* keep this struct small since we have an array of them */
typedef struct {
	char	token[Maxtok];
	char	type;
	schar	value;
} Datetok;

int dtok_numparsed;

/* forwards */
Datetok	*datetoktype(char *s, int *bigvalp);

static Datetok datetktbl[];
static unsigned szdatetktbl;

/* parse 1- or 2-digit number, advance *cpp past it */
static int
eatnum(char **cpp)
{
	int c, x;
	char *cp;

	cp = *cpp;
	c = *cp;
	if (!isascii(c) || !isdigit(c))
		return -1;
	x = c - '0';

	c = *++cp;
	if (isascii(c) && isdigit(c)) {
		x = 10*x + c - '0';
		cp++;
	}
	*cpp = cp;
	return x;
}

/* return -1 on failure */
int
parsetime(char *time, Tm *tm)
{
	tm->hour = eatnum(&time);
	if (tm->hour == -1 || *time++ != ':')
		return -1;			/* only hour; too short */

	tm->min = eatnum(&time);
	if (tm->min == -1)
		return -1;
	if (*time++ != ':') {
		tm->sec = 0;
		return 0;			/* no seconds; okay */
	}

	tm->sec = eatnum(&time);
	if (tm->sec == -1)
		return -1;

	/* this may be considered too strict.  garbage at end of time? */
	return *time == '\0' || isascii(*time) && isspace(*time)? 0: -1;
}

/*
 * try to parse pre-split timestr in fields as an absolute date
 */
int
tryabsdate(char **fields, int nf, Tm *now, Tm *tm)
{
	int i, mer = HR24, bigval = -1;
	long flg = 0, ty;
	char *p;
	char upzone[32];
	Datetok *tp;

	now = localtime(time(0));	/* default to local time (zone) */
	tm->tzoff = now->tzoff;
	strncpy(tm->zone, now->zone, sizeof tm->zone);

	tm->mday = tm->mon = tm->year = -1;	/* mandatory */
	tm->hour = tm->min = tm->sec = 0;
	dtok_numparsed = 0;

	for (i = 0; i < nf; i++) {
		if (fields[i][0] == '\0')
			continue;
		tp = datetoktype(fields[i], &bigval);
		ty = (1L << tp->type) & ~(1L << Ignore);
		if (flg & ty)
			return -1;		/* repeated type */
		flg |= ty;
		switch (tp->type) {
		case Year:
			tm->year = bigval;
			if (tm->year < 1970 || tm->year > 2106)
				return -1;	/* can't represent in ulong */
			/* convert 4-digit year to 1900 origin */
			if (tm->year >= 1900)
				tm->year -= 1900;
			break;
		case Day:
			tm->mday = bigval;
			break;
		case Month:
			tm->mon = tp->value - 1; /* convert to zero-origin */
			break;
		case Timetok:
			if (parsetime(fields[i], tm) < 0)
				return -1;
			break;
		case Dtz:
		case Tz:
			tm->tzoff = FROMVAL(tp);
			/* tm2sec needs the name in upper case */
			strcpy(upzone, fields[i]);
			for (p = upzone; *p; p++)
				if (isascii(*p) && islower(*p))
					*p = toupper(*p);
			strncpy(tm->zone, upzone, sizeof tm->zone);
			break;
		case Ignore:
			break;
		case Ampm:
			mer = tp->value;
			break;
		default:
			return -1;	/* bad token type: CANTHAPPEN */
		}
	}
	if (tm->year == -1 || tm->mon == -1 || tm->mday == -1)
		return -1;		/* missing component */
	if (mer == PM)
		tm->hour += 12;
	return 0;
}

int
prsabsdate(char *timestr, Tm *now, Tm *tm)
{
	int nf;
	char *fields[Maxdateflds];
	static char delims[] = "- \t\n/,";

	nf = gettokens(timestr, fields, nelem(fields), delims+1);
	if (nf > nelem(fields))
		return -1;
	if (tryabsdate(fields, nf, now, tm) < 0) {
		char *p = timestr;

		/*
		 * could be a DEC-date; glue it all back together, split it
		 * with dash as a delimiter and try again.  Yes, this is a
		 * hack, but so are DEC-dates.
		 */
		while (--nf > 0) {
			while (*p++ != '\0')
				;
			p[-1] = ' ';
		}
		nf = gettokens(timestr, fields, nelem(fields), delims);
		if (nf > nelem(fields) || tryabsdate(fields, nf, now, tm) < 0)
			return -1;
	}
	return 0;
}

int
validtm(Tm *tm)
{
	if (tm->year < 0 || tm->mon < 0 || tm->mon > 11 ||
	    tm->mday < 1 || tm->hour < 0 || tm->hour >= 24 ||
	    tm->min < 0 || tm->min > 59 ||
	    tm->sec < 0 || tm->sec > 61)	/* allow 2 leap seconds */
		return 0;
	return 1;
}

Time
seconds(char *timestr)
{
	Tm date;

	memset(&date, 0, sizeof date);
	if (prsabsdate(timestr, localtime(time(0)), &date) < 0)
		return -1;
	return validtm(&date)? tm2sec(&date): -1;
}

int
convert(char *timestr)
{
	char *copy;
	Time tstime;

	copy = strdup(timestr);
	if (copy == nil)
		sysfatal("out of memory");
	tstime = seconds(copy);
	free(copy);
	if (tstime == -1) {
		fprint(2, "%s: `%s' not a valid date\n", argv0, timestr);
		return 1;
	}
	print("%lud\n", tstime);
	return 0;
}

static void
usage(void)
{
	fprint(2, "usage: %s date-time ...\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i, sts;

	sts = 0;
	ARGBEGIN{
	default:
		usage();
	}ARGEND
	if (argc == 0)
		usage();
	for (i = 0; i < argc; i++)
		sts |= convert(argv[i]);
	exits(sts != 0? "bad": 0);
}

/*
 * Binary search -- from Knuth (6.2.1) Algorithm B.  Special case like this
 * is WAY faster than the generic bsearch().
 */
Datetok *
datebsearch(char *key, Datetok *base, unsigned nel)
{
	int cmp;
	Datetok *last = base + nel - 1, *pos;

	while (last >= base) {
		pos = base + ((last - base) >> 1);
		cmp = key[0] - pos->token[0];
		if (cmp == 0) {
			cmp = strncmp(key, pos->token, Maxtok);
			if (cmp == 0)
				return pos;
		}
		if (cmp < 0)
			last = pos - 1;
		else
			base = pos + 1;
	}
	return 0;
}

Datetok *
datetoktype(char *s, int *bigvalp)
{
	char *cp = s;
	char c = *cp;
	static Datetok t;
	Datetok *tp = &t;

	if (isascii(c) && isdigit(c)) {
		int len = strlen(cp);

		if (len > 3 && (cp[1] == ':' || cp[2] == ':'))
			tp->type = Timetok;
		else {
			if (bigvalp != nil)
				*bigvalp = atoi(cp); /* won't fit in tp->value */
			if (len == 4)
				tp->type = Year;
			else if (++dtok_numparsed == 1)
				tp->type = Day;
			else
				tp->type = Year;
		}
	} else if (c == '-' || c == '+') {
		int val = atoi(cp + 1);
		int hr =  val / 100;
		int min = val % 100;

		val = hr*60 + min;
		TOVAL(tp, c == '-'? -val: val);
		tp->type = Tz;
	} else {
		char lowtoken[Maxtok+1];
		char *ltp = lowtoken, *endltp = lowtoken+Maxtok;

		/* copy to lowtoken to avoid modifying s */
		while ((c = *cp++) != '\0' && ltp < endltp)
			*ltp++ = (isascii(c) && isupper(c)? tolower(c): c);
		*ltp = '\0';
		tp = datebsearch(lowtoken, datetktbl, szdatetktbl);
		if (tp == nil) {
			tp = &t;
			tp->type = Ignore;
		}
	}
	return tp;
}


/*
 * to keep this table reasonably small, we divide the lexval for Tz and Dtz
 * entries by 10 and truncate the text field at MAXTOKLEN characters.
 * the text field is not guaranteed to be NUL-terminated.
 */
static Datetok datetktbl[] = {
/*	text		token	lexval */
	"acsst",	Dtz,	63,	/* Cent. Australia */
	"acst",		Tz,	57,	/* Cent. Australia */
	"adt",		Dtz,	-18,	/* Atlantic Daylight Time */
	"aesst",	Dtz,	66,	/* E. Australia */
	"aest",		Tz,	60,	/* Australia Eastern Std Time */
	"ahst",		Tz,	60,	/* Alaska-Hawaii Std Time */
	"am",		Ampm,	AM,
	"apr",		Month,	4,
	"april",	Month,	4,
	"ast",		Tz,	-24,	/* Atlantic Std Time (Canada) */
	"at",		Ignore,	0,	/* "at" (throwaway) */
	"aug",		Month,	8,
	"august",	Month,	8,
	"awsst",	Dtz,	54,	/* W. Australia */
	"awst",		Tz,	48,	/* W. Australia */
	"bst",		Tz,	6,	/* British Summer Time */
	"bt",		Tz,	18,	/* Baghdad Time */
	"cadt",		Dtz,	63,	/* Central Australian DST */
	"cast",		Tz,	57,	/* Central Australian ST */
	"cat",		Tz,	-60,	/* Central Alaska Time */
	"cct",		Tz,	48,	/* China Coast */
	"cdt",		Dtz,	-30,	/* Central Daylight Time */
	"cet",		Tz,	6,	/* Central European Time */
	"cetdst",	Dtz,	12,	/* Central European Dayl.Time */
	"cst",		Tz,	-36,	/* Central Standard Time */
	"dec",		Month,	12,
	"decemb",	Month,	12,
	"dnt",		Tz,	6,	/* Dansk Normal Tid */
	"dst",		Ignore,	0,
	"east",		Tz,	-60,	/* East Australian Std Time */
	"edt",		Dtz,	-24,	/* Eastern Daylight Time */
	"eet",		Tz,	12,	/* East. Europe, USSR Zone 1 */
	"eetdst",	Dtz,	18,	/* Eastern Europe */
	"est",		Tz,	-30,	/* Eastern Standard Time */
	"feb",		Month,	2,
	"februa",	Month,	2,
	"fri",		Ignore,	5,
	"friday",	Ignore,	5,
	"fst",		Tz,	6,	/* French Summer Time */
	"fwt",		Dtz,	12,	/* French Winter Time  */
	"gmt",		Tz,	0,	/* Greenwish Mean Time */
	"gst",		Tz,	60,	/* Guam Std Time, USSR Zone 9 */
	"hdt",		Dtz,	-54,	/* Hawaii/Alaska */
	"hmt",		Dtz,	18,	/* Hellas ? ? */
	"hst",		Tz,	-60,	/* Hawaii Std Time */
	"idle",		Tz,	72,	/* Intl. Date Line, East */
	"idlw",		Tz,	-72,	/* Intl. Date Line, West */
	"ist",		Tz,	12,	/* Israel */
	"it",		Tz,	22,	/* Iran Time */
	"jan",		Month,	1,
	"januar",	Month,	1,
	"jst",		Tz,	54,	/* Japan Std Time,USSR Zone 8 */
	"jt",		Tz,	45,	/* Java Time */
	"jul",		Month,	7,
	"july",		Month,	7,
	"jun",		Month,	6,
	"june",		Month,	6,
	"kst",		Tz,	54,	/* Korea Standard Time */
	"ligt",		Tz,	60,	/* From Melbourne, Australia */
	"mar",		Month,	3,
	"march",	Month,	3,
	"may",		Month,	5,
	"mdt",		Dtz,	-36,	/* Mountain Daylight Time */
	"mest",		Dtz,	12,	/* Middle Europe Summer Time */
	"met",		Tz,	6,	/* Middle Europe Time */
	"metdst",	Dtz,	12,	/* Middle Europe Daylight Time*/
	"mewt",		Tz,	6,	/* Middle Europe Winter Time */
	"mez",		Tz,	6,	/* Middle Europe Zone */
	"mon",		Ignore,	1,
	"monday",	Ignore,	1,
	"mst",		Tz,	-42,	/* Mountain Standard Time */
	"mt",		Tz,	51,	/* Moluccas Time */
	"ndt",		Dtz,	-15,	/* Nfld. Daylight Time */
	"nft",		Tz,	-21,	/* Newfoundland Standard Time */
	"nor",		Tz,	6,	/* Norway Standard Time */
	"nov",		Month,	11,
	"novemb",	Month,	11,
	"nst",		Tz,	-21,	/* Nfld. Standard Time */
	"nt",		Tz,	-66,	/* Nome Time */
	"nzdt",		Dtz,	78,	/* New Zealand Daylight Time */
	"nzst",		Tz,	72,	/* New Zealand Standard Time */
	"nzt",		Tz,	72,	/* New Zealand Time */
	"oct",		Month,	10,
	"octobe",	Month,	10,
	"on",		Ignore,	0,	/* "on" (throwaway) */
	"pdt",		Dtz,	-42,	/* Pacific Daylight Time */
	"pm",		Ampm,	PM,
	"pst",		Tz,	-48,	/* Pacific Standard Time */
	"sadt",		Dtz,	63,	/* S. Australian Dayl. Time */
	"sast",		Tz,	57,	/* South Australian Std Time */
	"sat",		Ignore,	6,
	"saturd",	Ignore,	6,
	"sep",		Month,	9,
	"sept",		Month,	9,
	"septem",	Month,	9,
	"set",		Tz,	-6,	/* Seychelles Time ?? */
	"sst",		Dtz,	12,	/* Swedish Summer Time */
	"sun",		Ignore,	0,
	"sunday",	Ignore,	0,
	"swt",		Tz,	6,	/* Swedish Winter Time  */
	"thu",		Ignore,	4,
	"thur",		Ignore,	4,
	"thurs",	Ignore,	4,
	"thursd",	Ignore,	4,
	"tue",		Ignore,	2,
	"tues",		Ignore,	2,
	"tuesda",	Ignore,	2,
	"ut",		Tz,	0,
	"utc",		Tz,	0,
	"wadt",		Dtz,	48,	/* West Australian DST */
	"wast",		Tz,	42,	/* West Australian Std Time */
	"wat",		Tz,	-6,	/* West Africa Time */
	"wdt",		Dtz,	54,	/* West Australian DST */
	"wed",		Ignore,	3,
	"wednes",	Ignore,	3,
	"weds",		Ignore,	3,
	"wet",		Tz,	0,	/* Western Europe */
	"wetdst",	Dtz,	6,	/* Western Europe */
	"wst",		Tz,	48,	/* West Australian Std Time */
	"ydt",		Dtz,	-48,	/* Yukon Daylight Time */
	"yst",		Tz,	-54,	/* Yukon Standard Time */
	"zp4",		Tz,	-24,	/* GMT +4  hours. */
	"zp5",		Tz,	-30,	/* GMT +5  hours. */
	"zp6",		Tz,	-36,	/* GMT +6  hours. */
};
static unsigned szdatetktbl = nelem(datetktbl);
