#include <u.h>
#include <libc.h>

void	_readtimezone(_Timezone *);

_Timezone _timezone;

#define SEC2MIN 60UL
#define SEC2HOUR (60UL*SEC2MIN)
#define SEC2DAY (24UL*SEC2HOUR)

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
 * and convert to our _timezone.
 */
long
tm2sec(Tm *tm)
{
	ulong secs;
	int i, yday, year, *d2m;

	if(strcmp(tm->zone, "GMT") != 0 && _timezone.stname[0] == 0)
		_readtimezone(&_timezone);
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
	if(strcmp(tm->zone, _timezone.stname) == 0)
		secs -= _timezone.stdiff;
	else if(strcmp(tm->zone, _timezone.dlname) == 0)
		secs -= _timezone.dldiff;
	return secs;
}
