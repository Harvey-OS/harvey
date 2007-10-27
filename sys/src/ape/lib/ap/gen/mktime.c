#include <time.h>

/*
 * BUG: Doesn't do leap years in full glory,
 * or calendar changes. In 2038 the sign bit
 * will be needed in time_t, but we say it
 * can't be represented.
 */
static int
dysize(int y)
{
	y += 1900; /* arg is a tm_year, number of years since 1900 */
	if((y%4) == 0 && ((y%100) !=0 || (y%400) == 0))
		return 366;
	return 365;
}

static int
dmsize(int m, int y)
{
	static	char	sizes[12] =
		{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	if(m == 1)
		return (dysize(y)==366)? 29 : 28;
	else
		return sizes[m];
}

/* Reduce *v to [0, mult), adding 1 to *next for every mult
 * subtracted from *v, and return 1 if reduction worked (no overflow)
 */
static int
reduce(int *v, int *next, int mult)
{
	int oldnext;

	while(*v < 0){
		*v += mult;
		oldnext = *next;
		--*next;
		if(!(*next < oldnext))
			return 0;
	}
	while(*v >= mult){
		*v -= mult;
		oldnext = *next;
		++*next;
		if(!(*next > oldnext))
			return 0;
	}
	return 1;
}

static int
jan1(int yr)
{
	int y, d;

	y = yr+1900;
	d = (4+y+(y+3)/4-(y-1701)/100+(y-1601)/400+3)%7;
	return d;
}

time_t
mktime(struct tm *t)
{
	time_t a;
	int i, d;
	struct tm *ptm;

	if(!(reduce(&t->tm_sec, &t->tm_min, 60) &&
	     reduce(&t->tm_min, &t->tm_hour, 60) &&
	     reduce(&t->tm_hour, &t->tm_mday, 24) &&
	     reduce(&t->tm_mon, &t->tm_year, 12)))
		return -1;
	while(t->tm_mday < 1){
		if(--t->tm_mon == -1){
			t->tm_mon = 11;
			t->tm_year--;
		}
		t->tm_mday += dmsize(t->tm_mon, t->tm_year);
	}
	while(t->tm_mday > dmsize(t->tm_mon, t->tm_year)){
		t->tm_mday -= dmsize(t->tm_mon, t->tm_year);
		if(++t->tm_mon == 12){
			t->tm_mon = 0;
			t->tm_year++;
		}
	}
	a = t->tm_sec + 60*t->tm_min + 3600*t->tm_hour;
	t->tm_yday = t->tm_mday-1;
	for(i=0; i<t->tm_mon; i++)
		t->tm_yday += dmsize(i, t->tm_year);
	a += t->tm_yday*86400L;
	if(t->tm_year < 70){
		for(i=t->tm_year; i<70; i++)
			if((a -= dysize(i)*86400L) < 0)
				return -1;
	}else if(t->tm_year > 70){
		for(i=70; i<t->tm_year; i++)
			if((a += dysize(i)*86400L) < 0)
				return -1;
	}
	/*
	 * Now a is number of seconds past Jan 1 1970.
	 * Convert to GMT.
	 */
	ptm = gmtime(&a);
	d = ptm->tm_hour;
	ptm = localtime(&a);
	d -= ptm->tm_hour;
	if(d < 0)
		d += 24;
	if(t->tm_isdst == 0 && ptm->tm_isdst)
		d--;
	if(t->tm_isdst > 0 && !ptm->tm_isdst)
		d++;
	a += d*3600;
	t->tm_wday = (jan1(t->tm_year)+t->tm_yday)%7;
	return a;
}
