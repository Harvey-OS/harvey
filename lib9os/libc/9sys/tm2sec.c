#include <u.h>
#include <libc.h>

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

#define SEC2MIN 60L
#define SEC2HOUR (60L*SEC2MIN)
#define SEC2DAY (24L*SEC2HOUR)

/*
 *  days per month plus days/year
 */
static	int	dmsize[] =
{
	365, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
static	int	ldmsize[] =
{
	366, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/*
 *  return the days/month for the given year
 */
static int *
yrsize(int y)
{
	if((y%4) == 0 && ((y%100) != 0 || (y%400) == 0))
		return ldmsize;
	else
		return dmsize;
}

/*
 * compute seconds since Jan 1 1970 GMT
 * and convert to our timezone.
 */
long
tm2sec(Tm *tm)
{
	long secs;
	int i, yday, year, *d2m;

	if(strcmp(tm->zone, "GMT") != 0 && timezone.stname[0] == 0)
		readtimezone();
	secs = 0;

	/*
	 *  seconds per year
	 */
	year = tm->year + 1900;
	for(i = 1970; i < year; i++){
		d2m = yrsize(i);
		secs += d2m[0] * SEC2DAY;
	}

	/*
	 *  if mday is set, use mon and mday to compute yday
	 */
	if(tm->mday){
		yday = 0;
		d2m = yrsize(year);
		for(i=0; i<tm->mon; i++)
			yday += d2m[i+1];
		yday += tm->mday-1;
	}else{
		yday = tm->yday;
	}
	secs += yday * SEC2DAY;

	/*
	 * hours, minutes, seconds
	 */
	secs += tm->hour * SEC2HOUR;
	secs += tm->min * SEC2MIN;
	secs += tm->sec;

	/*
	 * Only handles zones mentioned in /env/timezone,
	 * but things get too ambiguous otherwise.
	 */
	if(strcmp(tm->zone, timezone.stname) == 0)
		secs -= timezone.stdiff;
	else if(strcmp(tm->zone, timezone.dlname) == 0)
		secs -= timezone.dldiff;
	if(secs < 0)
		secs = 0;
	return secs;
}

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

static int
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

static int
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
