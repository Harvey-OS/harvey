/*
 * This routine converts time as follows.
 * The epoch is 0000 Jan 1 1970 GMT.
 * The argument time is in seconds since then.
 * The localtime(t) entry returns a pointer to an array
 * containing
 *
 *	seconds (0-59)
 *	minutes (0-59)
 *	hours (0-23)
 *	day of month (1-31)
 *	month (0-11)
 *	year-1970
 *	weekday (0-6, Sun is 0)
 *	day of the year
 *	daylight savings flag
 *
 * The routine gets the daylight savings time from the environment.
 *
 * asctime(tvec))
 * where tvec is produced by localtime
 * returns a ptr to a character string
 * that has the ascii time in the form
 *
 *	                            \\
 *	Thu Jan 01 00:00:00 1970n0
 *	01234567890123456789012345
 *	0	  1	    2
 *
 * ctime(t) just calls localtime, then asctime.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

static	char	dmsize[12] =
{
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/*
 * The following table is used for 1974 and 1975 and
 * gives the day number of the first day after the Sunday of the
 * change.
 */

static	int	dysize(int);
static	void	ct_numb(char*, int);

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

char*
ctime(const time_t *t)
{
	return asctime(localtime(t));
}

struct tm*
localtime(const time_t *timp)
{
	struct tm *ct;
	time_t t, tim;
	long *p;
	int i, dlflag;

	tim = *timp;
	if(timezone.stname[0] == 0)
		readtimezone();
	t = tim + timezone.stdiff;
	dlflag = 0;
	for(p = timezone.dlpairs; *p; p += 2)
		if(t >= p[0])
		if(t < p[1]) {
			t = tim + timezone.dldiff;
			dlflag++;
			break;
		}
	ct = gmtime(&t);
	ct->tm_isdst = dlflag;
	return ct;
}

struct tm*
gmtime(const time_t *timp)
{
	int d0, d1;
	long hms, day;
	time_t tim;
	static struct tm xtime;

	tim = *timp;
	/*
	 * break initial number into days
	 */
	hms = tim % 86400L;
	day = tim / 86400L;
	if(hms < 0) {
		hms += 86400L;
		day -= 1;
	}

	/*
	 * generate hours:minutes:seconds
	 */
	xtime.tm_sec = hms % 60;
	d1 = hms / 60;
	xtime.tm_min = d1 % 60;
	d1 /= 60;
	xtime.tm_hour = d1;

	/*
	 * day is the day number.
	 * generate day of the week.
	 * The addend is 4 mod 7 (1/1/1970 was Thursday)
	 */

	xtime.tm_wday = (day + 7340036L) % 7;

	/*
	 * year number
	 */
	if(day >= 0)
		for(d1 = 70; day >= dysize(d1); d1++)
			day -= dysize(d1);
	else
		for (d1 = 70; day < 0; d1--)
			day += dysize(d1-1);
	xtime.tm_year = d1;
	xtime.tm_yday = d0 = day;

	/*
	 * generate month
	 */

	if(dysize(d1) == 366)
		dmsize[1] = 29;
	for(d1 = 0; d0 >= dmsize[d1]; d1++)
		d0 -= dmsize[d1];
	dmsize[1] = 28;
	xtime.tm_mday = d0 + 1;
	xtime.tm_mon = d1;
	xtime.tm_isdst = 0;
	return &xtime;
}

char*
asctime(const struct tm *t)
{
	char *ncp;
	static char cbuf[30];

	strcpy(cbuf, "Thu Jan 01 00:00:00 1970\n");
	ncp = &"SunMonTueWedThuFriSat"[t->tm_wday*3];
	cbuf[0] = *ncp++;
	cbuf[1] = *ncp++;
	cbuf[2] = *ncp;
	ncp = &"JanFebMarAprMayJunJulAugSepOctNovDec"[t->tm_mon*3];
	cbuf[4] = *ncp++;
	cbuf[5] = *ncp++;
	cbuf[6] = *ncp;
	ct_numb(cbuf+8, t->tm_mday);
	ct_numb(cbuf+11, t->tm_hour+100);
	ct_numb(cbuf+14, t->tm_min+100);
	ct_numb(cbuf+17, t->tm_sec+100);
	if(t->tm_year >= 100) {
		cbuf[20] = '2';
		cbuf[21] = '0';
	}
	ct_numb(cbuf+22, t->tm_year+100);
	return cbuf;
}

static
dysize(int y)
{

	if((y%4) == 0)
		return 366;
	return 365;
}

static
void
ct_numb(char *cp, int n)
{

	cp[0] = ' ';
	if(n >= 10)
		cp[0] = (n/10)%10 + '0';
	cp[1] = n%10 + '0';
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
