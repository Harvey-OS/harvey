#include <time.h>
#include <string.h>

static char *awday[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static char *wday[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday",
			"Friday", "Saturday"};
static char *amon[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
			    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static char *mon[12] = {"January", "February", "March", "April", "May", "June",
		"July", "August", "September", "October", "November", "December"};
static char *ampm[2] = {"AM", "PM"};
static char *tz[2] = {"EST", "EDT"};

static int jan1(int);
static char *strval(char *, char *, char **, int, int);
static char *dval(char *, char *, int, int);

size_t
strftime(char *s, size_t maxsize, const char *format, const struct tm *t)
{
	char *sp, *se, *fp;
	int i;

	sp = s;
	se = s+maxsize;
	for(fp=(char *)format; *fp && sp<se; fp++){
		if(*fp != '%')
			*sp++ = *fp;
		else switch(*++fp){
			case 'a':
				sp = strval(sp, se, awday, t->tm_wday, 7);
				break;
			case 'A':
				sp = strval(sp, se, wday, t->tm_wday, 7);
				break;
			case 'b':
				sp = strval(sp, se, amon, t->tm_mon, 12);
				break;
			case 'B':
				sp = strval(sp, se, mon, t->tm_mon, 12);
				break;
			case 'c':
				sp += strftime(sp, se-sp, "%a %b %d %H:%M:%S %Y", t);
				break;
			case 'd':
				sp = dval(sp, se, t->tm_mday, 2);
				break;
			case 'H':
				sp = dval(sp, se, t->tm_hour, 2);
				break;
			case 'I':
				i = t->tm_hour;
				if(i == 0)
					i = 12;
				else if(i > 12)
					i -= 12;
				sp = dval(sp, se, i, 2);
				break;
			case 'j':
				sp = dval(sp, se, t->tm_yday+1, 3);
				break;
			case 'm':
				sp = dval(sp, se, t->tm_mon+1, 2);
				break;
			case 'M':
				sp = dval(sp, se, t->tm_min, 2);
				break;
			case 'p':
				i = (t->tm_hour < 12)? 0 : 1;
				sp = strval(sp, se, ampm, i, 2);
				break;
			case 'S':
				sp = dval(sp, se, t->tm_sec, 2);
				break;
			case 'U':
				i = 7-jan1(t->tm_year);
				if(i == 7)
					i = 0;
				/* Now i is yday number of first sunday in year */
				if(t->tm_yday < i)
					i = 0;
				else
					i = (t->tm_yday-i)/7 + 1;
				sp = dval(sp, se, i, 2);
				break;
			case 'w':
				sp = dval(sp, se, t->tm_wday, 1);
				break;
			case 'W':
				i = 8-jan1(t->tm_year);
				if(i >= 7)
					i -= 7;
				/* Now i is yday number of first monday in year */
				if(t->tm_yday < i)
					i = 0;
				else
					i = (t->tm_yday-i)/7 + 1;
				sp = dval(sp, se, i, 2);
				break;
			case 'x':
				sp += strftime(sp, se-sp, "%a %b %d, %Y", t);
				break;
			case 'X':
				sp += strftime(sp, se-sp, "%H:%M:%S", t);
				break;
			case 'y':
				sp = dval(sp, se, t->tm_year%100, 2);
				break;
			case 'Y':
				sp = dval(sp, se, t->tm_year+1900, 4);
				break;
			case 'Z':
				/* hack for now: assume eastern time zone */
				i = t->tm_isdst? 1 : 0;
				sp = strval(sp, se, tz, i, 2);
				break;
			case 0:
				fp--; /* stop loop after next fp incr */
				break;
			default:
				*sp++ = *fp;
			}
	}
	if(*fp)
		sp = s; /* format string didn't end: no room for conversion */
	if(sp<se)
		*sp = 0;
	return sp-s;
}

static char *
strval(char *start, char *end, char **array, int index, int alen)
{
	int n;

	if(index<0 || index>=alen){
		*start = '?';
		return start+1;
	}
	n = strlen(array[index]);
	if(n > end-start)
		n = end-start;
	memcpy(start, array[index], n);
	return start+n;
}

static char *
dval(char *start, char *end, int val, int width)
{
	char *p;

	if(val<0 || end-start<width){
		*start = '?';
		return start+1;
	}
	p = start+width-1;
	while(p>=start){
		*p-- = val%10 + '0';
		val /= 10;
	}
	if(val>0)
		*start = '*';
	return start+width;
}

/*
 *	return day of the week
 *	of jan 1 of given year
 */
static int
jan1(int yr)
{
	int y, d;

/*
 *	normal gregorian calendar
 *	one extra day per four years
 */

	y = yr+1900;
	d = 4+y+(y+3)/4;

/*
 *	julian calendar
 *	regular gregorian
 *	less three days per 400
 */

	if(y > 1800) {
		d -= (y-1701)/100;
		d += (y-1601)/400;
	}

/*
 *	great calendar changeover instant
 */

	if(y > 1752)
		d += 3;

	return(d%7);
}
