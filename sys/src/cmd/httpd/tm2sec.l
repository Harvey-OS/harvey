#include <alef.h>

#define SEC2MIN 60
#define SEC2HOUR (60*SEC2MIN)
#define SEC2DAY (24*SEC2HOUR)

/*
 *  days per month plus days/year
 */
intern	int	dmsize[] =
{
	365, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
intern	int	ldmsize[] =
{
	366, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/*
 *  return the days/month for the given year
 */
intern int *
yrsize(int yr)
{
	if(yr % 4 == 0 && yr % 400)
		return ldmsize;
	else
		return dmsize;
}

/*
 * compute seconds since Jan 1 1970 GMT
 */
int
gmtm2sec(Tm tm)
{
	int secs, i, *d2m;

	secs = 0;

	/*
	 *  seconds per year
	 */
	tm.year += 1900;
	if(tm.year < 1970)
		return 0;
	for(i = 1970; i < tm.year; i++){
		d2m = yrsize(i);
		secs += d2m[0] * SEC2DAY;
	}

	/*
	 *  seconds per month
	 */
	d2m = yrsize(tm.year);
	for(i = 0; i < tm.mon; i++)
		secs += d2m[i+1] * SEC2DAY;

	/*
	 * secs in last month
	 */
	secs += (tm.mday-1) * SEC2DAY;

	/*
	 * hours, minutes, seconds
	 */
	secs += tm.hour * SEC2HOUR;
	secs += tm.min * SEC2MIN;
	secs += tm.sec;

	return secs;
}
