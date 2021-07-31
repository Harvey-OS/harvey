#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#include "dat.h"

static int then, now;
static long timelocal(Tm *);

int warning[] = {
	24*60*60,
	60*60,
	15*60,
	 1*60,
	 0,
	-1
};

char *month[] = {
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
	"december",
	0
};

int
alpha(int c)
{
	return 'a'<=c && c<='z' || 'A'<=c && c<='Z';
}

int ismonth;	/* flag for nfield to tell timefields that it saw a month name */

char*
nfield(char *line, int *v)
{
	char *field, **mon;
	int nlet;

	while(*line!='\0' && strchr(" \t./:,", *line))
		line++;
	if(alpha(*line)){
		ismonth = 1;
		field = line;
		while(alpha(*line)){
			if('A'<=*line && *line<='Z')
				*line+='a'-'A';
			line++;
		}
		nlet = line-field;
		if(nlet<3)
			return 0;
		for(mon=month; *mon ;mon++)
			if(strncmp(*mon, field, nlet) == 0){
				*v = mon-month+1;
				return line;
			}
		return 0;
	}
	ismonth = 0;
	if(*line<'0' || '9'<*line)
		return 0;
	*v=0;
	while('0'<=*line && *line<='9')
		*v = *v*10+*line++-'0';
	return line;
}

char*
timefields(char *line, Tm *tp)
{
	int wasmonth, t;

	line = nfield(line, &tp->mon);
	if(line == 0)
		return 0;
	wasmonth = ismonth;
	line = nfield(line, &tp->mday);
	if(line == 0)
		return 0;
	if(ismonth){
		if(wasmonth)
			return 0;
		t = tp->mon;
		tp->mon = tp->mday;
		tp->mday = t;
	}
	--tp->mon;
	line = nfield(line, &tp->hour);
	if(line==0 || ismonth)
		return 0;
	line = nfield(line, &tp->min);
	if(line==0 || ismonth)
		return 0;
	while(*line==' ' || *line=='\t')
		line++;
	return line;
}

void
showmsg(char *msg)
{
	Point p;

	p.x = screen.r.min.x + DATEX;
	p.y = screen.r.min.y + DATEY + medifont->height+1;
	string(&screen, p, font, msg, S);
	bflush();
}

void
remind(char *file)
{
	char *line, *rest, *room, *end;
	Biobuf *rf;
	long tline;
	Tm tmline, tmnow;
	int i;
	char tbuf[20];

	if(file==0)
		return;
	then = now;
	now = time(0);
	rf = Bopen(file, OREAD);
	if(rf==0)
		error("can't open reminders");
	tmnow = *localtime(now);
	while(line=Brdline(rf, '\n')){
		room = timefields(line, &tmline);
		if(room == 0)
			continue;
		if(tmline.mon!=tmnow.mon || tmline.mday!=tmnow.mday)
			continue;
		tmline.year = tmnow.year;
		tline = timelocal(&tmline);
		for(end=room;*end!='\n' && *end!='\0';end++)
			;
		*end = '\0';
		for(i=0;warning[i]>=0;i++){
			if(then<tline-warning[i] && tline-warning[i]<=now){
				showmsg(room);
				sprint(tbuf, "%d %.02d", tmline.hour, tmline.min);
				sayit(tbuf, room);
				sprint(tbuf, "%d:%.02d", tmline.hour, tmline.min);
				for(rest=room;*rest!=' ' && *rest!='\t' && *rest!='\0'; rest++)
					;
				*rest = '\0';
				newicon("", "remind");
				puticon(room, tbuf);
				break;
			}
		}
	}
	Bterm(rf);
}
#define	TZSIZE	150
static	void	readtimezone(void);
static	int	rd_name(char**, char*);
static	int	rd_long(char**, long*);
static
struct
{
	char	stname[4];
	char	dlname[4];
	long	stdiff;
	long	dldiff;
	long	dlpairs[TZSIZE];
} timezone;
/*
 * timelocal is the inverse of localtime.
 *
 * Tm represents dates redundantly.  To accomodate partially
 * filled-in Tm values, this code assumes that if tp->mday is
 * non-zero, that it and tp->mon are valid.  Otherwise, it uses
 * tp->yday.
 *
 * Timelocal does not examine tm->zone, so if t is a time during
 * the hour after the Fall DST change, timelocal(localtime(t))!=t.
 */
static long
timelocal(Tm *tp)
{
	long days, secs, years;
	long *p;

	if(timezone.stname[0] == '\0')
		readtimezone();
	years = tp->year+1900;
	/*
	 * It's safe to ignore the 100 and 400 year rules, because
	 * the dates representable in 32 bits of seconds since 1970
	 * run from Fri Dec 13 20:45:52 GMT 1901 to Tue Jan 19 03:14:07 GMT 2038
	 */
	days = 365*(years-1970)+(years-1)/4-1970/4;
	if(tp->mday){
		switch(tp->mon){
		case 11: days+=30;
		case 10: days+=31;
		case  9: days+=30;
		case  8: days+=31;
		case  7: days+=31;
		case  6: days+=30;
		case  5: days+=31;
		case  4: days+=30;
		case  3: days+=31;
		case  2: days+=28+(years%4==0);
		case  1: days+=31;
		case  0: break;
		}
		days += tp->mday-1;
	}
	else
		days += tp->yday;
	secs = ((days*24+tp->hour)*60+tp->min)*60+tp->sec;
	for(p=timezone.dlpairs; *p; p+=2)
		if(p[0]<=secs && secs<p[1])
			return secs-timezone.dldiff;
	return secs-timezone.stdiff;
}
/*
 * readtimezone, rd_name and rd_long are copied from
 * /sys/src/libc/9sys/ctime.c
 * It would be nice if they weren't declared static there.
 */
static
void
readtimezone(void)
{
	char buf[TZSIZE*11+30], *p;
	int i;

	memset(buf, 0, sizeof(buf));
	i = open("/env/timezone", 0);
	if(i < 0)
		goto error;
	if(read(i, buf, sizeof(buf)) >= sizeof(buf))
		goto error;
	close(i);
	p = buf;
	if(rd_name(&p, timezone.stname))
		goto error;
	if(rd_long(&p, &timezone.stdiff))
		goto error;
	if(rd_name(&p, timezone.dlname))
		goto error;
	if(rd_long(&p, &timezone.dldiff))
		goto error;
	for(i=0; i<TZSIZE; i++) {
		if(rd_long(&p, &timezone.dlpairs[i]))
			goto error;
		if(timezone.dlpairs[i] == 0)
			return;
	}

error:
	timezone.stdiff = 0;
	strcpy(timezone.stname, "GMT");
	timezone.dlpairs[0] = 0;
}

static
rd_name(char **f, char *p)
{
	int c, i;

	for(;;) {
		c = *(*f)++;
		if(c != ' ' && c != '\n')
			break;
	}
	for(i=0; i<3; i++) {
		if(c == ' ' || c == '\n')
			return 1;
		*p++ = c;
		c = *(*f)++;
	}
	if(c != ' ' && c != '\n')
		return 1;
	*p = 0;
	return 0;
}

static
rd_long(char **f, long *p)
{
	int c, s;
	long l;

	s = 0;
	for(;;) {
		c = *(*f)++;
		if(c == '-') {
			s++;
			continue;
		}
		if(c != ' ' && c != '\n')
			break;
	}
	if(c == 0) {
		*p = 0;
		return 0;
	}
	l = 0;
	for(;;) {
		if(c == ' ' || c == '\n')
			break;
		if(c < '0' || c > '9')
			return 1;
		l = l*10 + c-'0';
		c = *(*f)++;
	}
	if(s)
		l = -l;
	*p = l;
	return 0;
}
