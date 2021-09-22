/*
 * parse /env/timezone into a _Timezone
 */
#include <u.h>
#include <libc.h>

static int
rd_name(char **f, char *p)
{
	int c, i;

	while ((c = *(*f)++) == ' ' || c == '\n')
		;
	for(i=0; i<3; i++) {
		if(c == ' ' || c == '\n')
			return 1;		/* no name */
		*p++ = c;
		c = *(*f)++;
	}
	if(c != ' ' && c != '\n')
		return 1;			/* name too long */
	*p = '\0';
	return 0;
}

static int
rd_long(char **f, long *p)
{
	int c;
	long l;
	char *st;

	while ((c = **f) == ' ' || c == '\n')
		(*f)++;
	l = 0;					/* default, in case eof */
	if(c != '\0') {				/* not eof? */
		st = *f;
		if (*st == '-')
			l = strtol(st, f, 10);
		else
			l = strtoul(st, f, 10);
		if (*f == st)
			return 1;		/* not a number */
	}
	*p = l;
	return 0;
}

static void
_tzdef(_Timezone *tz)
{
	tz->stdiff = 0;
	strcpy(tz->stname, "GMT");
	tz->dlpairs[0] = 0;
}

void
_readtimezone(_Timezone *tz)
{
	char buf[_TZSIZE*11+30], *p;
	int i;
	uint nb;

	_tzdef(tz);

	i = open("/env/timezone", 0);
	if(i < 0)
		return;
	nb = read(i, buf, sizeof buf);
	close(i);
	if(nb < sizeof buf) {		/* didn't fill buf? */
		buf[nb] = '\0';

		p = buf;
		if(rd_name(&p, tz->stname) || rd_long(&p, &tz->stdiff) ||
		   rd_name(&p, tz->dlname) || rd_long(&p, &tz->dldiff))
			goto error;
		for(i=0; i<_TZSIZE; i++) {
			if(rd_long(&p, (long *)&tz->dlpairs[i]))
				goto error;
			if(tz->dlpairs[i] == 0)		/* eof? */
				return;
		}
	}

	i = open("/sys/log/recompile", OWRITE);
	if (i >= 0) {
		fprint(i, "%s: ctime/tm2sec timezone table too small\n", argv0);
		close(i);
	}
error:
	_tzdef(tz);
}
