#include <u.h>
#include <libc.h>
#include <bio.h>
#include <regexp.h>
#include <ctype.h>

typedef struct Date	Date;
struct Date {
	Reprog *p;	/* an RE to match this date */
	Date *next;	/* pointer to next in list */
};

enum{
	Secondsperday = 24*60*60
};

Date *Base = nil;
Biobuf in;
int debug, matchyear;

void dates(Tm*);
void upper2lower(char*, char*, int);
void *emalloc(unsigned int);

void
main(int argc, char *argv[])
{
	int i, fd, ahead;
	long now;
	char *line;
	Tm *tm;
	Date *d;
	char buf[1024];

	ahead = 0;
	ARGBEGIN{
	case 'y':
		matchyear = 1;
		break;
	case 'd':
		debug = 1;
		break;
	case 'p':
		ahead = atoi(ARGF());
		break;
	default:
		fprint(2, "usage: calendar [-dy] [-p days] [files ...]\n");
		exits("usage");
	}ARGEND;

	/* make a list of dates */
	now = time(0);
	tm = localtime(now);
	dates(tm);
	now += Secondsperday;
	tm = localtime(now);
	dates(tm);
	if(tm->wday == 6){
		now += Secondsperday;
		tm = localtime(now);
		dates(tm);
	}
	if(tm->wday == 0){
		now += Secondsperday;
		tm = localtime(now);
		dates(tm);
	}
	if(ahead){
		now = time(0);
		now += ahead * Secondsperday;
		tm = localtime(now);
		dates(tm);
	}

	for(i=0; i<argc || (i==0 && argc==0); i++){
		if(i==0 && argc==0)
			snprint(buf, sizeof buf,
				"/usr/%s/lib/calendar", getuser());
		else
			strecpy(buf, buf+sizeof buf, argv[i]);
		fd = open(buf, OREAD);
		if(fd<0 || Binit(&in, fd, OREAD)<0){
			fprint(2, "calendar: can't open %s: %r\n", buf);
			exits("open");
		}

		/* go through the file */
		while(line = Brdline(&in, '\n')){
			line[Blinelen(&in) - 1] = 0;
			upper2lower(buf, line, sizeof buf);
			for(d=Base; d; d=d->next)
				if(regexec(d->p, buf, 0, 0)){
					print("%s\n", line);
					break;
				}
		}
		close(fd);
	}
	exits("");
}

char *months[] = 
{
	"january",
	"february",
	"march",
	"april",
	"may",
	"june",
	"july",
	"august",
	"september",
	"october",
	"november",
	"december"
};
char *nth[] = {
	"first", 
	"seccond",
	"third",
	"fourth",
	"fifth"
};
char *days[] = {
	"sunday",
	"monday",
	"tuesday",
	"wednesday",
	"thursday",
	"friday",
	"saturday"
};

/*
 * Generate two Date structures.  First has month followed by day;
 * second has day followed by month.  Link them into list after
 * last, and return the first.
 */
void
dates(Tm *tm)
{
	Date *nd;
	char mo[128], day[128], buf[128];

	if(utflen(days[tm->wday]) > 3)
		snprint(day, sizeof day, "%3.3s(%s)?",
			days[tm->wday], days[tm->wday]+3);
	else
		snprint(day, sizeof day, "%3.3s", days[tm->wday]);

	if(utflen(months[tm->mon]) > 3)
		snprint(mo, sizeof mo, "%3.3s(%s)?",
			months[tm->mon], months[tm->mon]+3);
	else
		snprint(mo, sizeof mo, "%3.3s", months[tm->mon]);
	if (matchyear)
		snprint(buf, sizeof buf,
			"^[\t ]*((%s( |\t)+)|(%d/))%d( |\t|$)(((%d|%d|%02d)( |\t|$))|[^0-9]|([0-9]+[^0-9 \t]))",
			mo, tm->mon+1, tm->mday, tm->year+1900, tm->year%100, tm->year%100);
	else
		snprint(buf, sizeof buf,
			"^[\t ]*((%s( |\t)+)|(%d/))%d( |\t|$)",
			mo, tm->mon+1, tm->mday);
	if(debug)
		print("%s\n", buf);

	nd = emalloc(sizeof(Date));
	nd->p = regcomp(buf);
	nd->next = Base;	
	Base = nd;

	if (matchyear)
		snprint(buf, sizeof buf,
			"^[\t ]*%d( |\t)+(%s)( |\t|$)(((%d|%d|%02d)( |\t|$))|[^0-9]|([0-9]+[^0-9 \t]))",
			tm->mday, mo, tm->year+1900, tm->year%100, tm->year%100);
	else
		snprint(buf, sizeof buf,
			"^[\t ]*%d( |\t)+(%s)( |\t|$)",
			tm->mday, mo);
	if(debug)
		print("%s\n", buf);
	nd = emalloc(sizeof(Date));
	nd->p = regcomp(buf);	
	nd->next = Base;	
	Base = nd;

	snprint(buf, sizeof buf, "^[\t ]*every[ \t]+%s", day);
	if(debug)
		print("%s\n", buf);
	nd = emalloc(sizeof(Date));
	nd->p = regcomp(buf);	
	nd->next = Base;	
	Base = nd;

	snprint(buf, sizeof buf, "[\t ]*the[\t ]+%s[\t ]+%s", nth[tm->mday/7], day);
	if(debug)
		print("%s\n", buf);
	nd = emalloc(sizeof(Date));
	nd->p = regcomp(buf);	
	nd->next = Base;	
	Base = nd;
}

/*
 * Copy 'from' to 'to', converting to lower case
 */
void
upper2lower(char *to, char *from, int len)
{
	while(--len>0 && *from!='\0')
		*to++ = tolower(*from++);
	*to = 0;
}

/*
 * Call malloc and check for errors
 */
void*
emalloc(unsigned int n)
{
	void *p;

	p = malloc(n);
	if(p == 0){
		fprint(2, "calendar: malloc failed: %r\n");
		exits("malloc");
	}
	return p;
}
