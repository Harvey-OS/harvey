#include <u.h>
#include <libc.h>
#include <bio.h>
#include <regexp.h>
#include <ctype.h>

typedef struct Date	Date;
struct Date {
	Reprog *p;
	Date *next;
};

Biobuf in;
int debug;

Date *dates(Date**, Tm*);
void upper2lower(char*, char*, int);

void
main(int argc, char *argv[])
{
	int fd;
	long now;
	char *line;
	Tm *tm;
	Date *first, *last, *d;
	char buf[1024];

	ARGBEGIN{
	case 'd':
		debug = 1;
		break;
	}ARGEND;

	if(argv[0])
		strcpy(buf, argv[0]);
	else
		snprint(buf, sizeof(buf), "/usr/%s/lib/calendar", getuser());
	fd = open(buf, OREAD);
	if(fd < 0){
		fprint(2, "calendar: can't open %s: %r\n", buf);
		exits("open");
	}
	Binit(&in, fd, OREAD);

	/* make a list of dates */
	now = time(0);
	tm = localtime(now);
	first = dates(&last, tm);
	tm = localtime(now+24*60*60);
	dates(&last, tm);
	if(tm->wday == 6){
		tm = localtime(now+2*24*60*60);
		dates(&last, tm);
	}
	if(tm->wday == 0){
		tm = localtime(now+3*24*60*60);
		dates(&last, tm);
	}

	/* go through the file */
	while(line = Brdline(&in, '\n')){
		line[Blinelen(&in) - 1] = 0;
		upper2lower(line, buf, sizeof buf);
		for(d = first; d; d = d->next)
			if(regexec(d->p, buf, 0, 0)){
				print("%s\n", line);
				break;
			}
	}
	exits(0);
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

Date*
dates(Date **last, Tm *tm)
{
	Date *first;
	Date *nd;
	char mo[128], buf[128];

	if(strlen(months[tm->mon]) > 3)
		sprint(mo, "%3.3s(%s)?",
			months[tm->mon], months[tm->mon]+3);
	else
		sprint(mo, "%3.3s", months[tm->mon]);
	snprint(buf, sizeof buf, "(^| |\t)((%s( |\t)+)|(%d/))%d( |\t|$)", mo, tm->mon+1, tm->mday);
	if(debug)
		print("%s\n", buf);
	first = malloc(sizeof(Date));
	if(first == 0){
		fprint(2, "calendar: out of memory\n");
		exits("memory");
	}
	if(*last)
		(*last)->next = first;
	first->p = regcomp(buf);	

	snprint(buf, sizeof buf, "(^| |\t)%d( |\t)+(%s)( |\t|$)",
		tm->mday, mo);
	if(debug)
		print("%s\n", buf);
	nd = malloc(sizeof(Date));
	if(nd == 0){
		fprint(2, "calendar: out of memory\n");
		exits("memory");
	}
	nd->p = regcomp(buf);	
	nd->next = 0;
	first->next = nd;
	*last = nd;

	return first;
}

void
upper2lower(char *from, char *to, int len)
{
	int c;

	while(--len > 0){
		c = *to++ = tolower(*from++);
		if(c == 0)
			return;
	}
	*to = 0;
}
