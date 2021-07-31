#include <all.h>

double
leg(double a)
{
	a = 1 - a*a;
	if(a <= 0)
		return 0;
	return sqrt(a);
}

/*
 * calculate distance in radians
 */
double
dist(Loc *o1, Loc *o2)
{
	double a;

	a = o1->sinlat * o2->sinlat +
		o1->coslat * o2->coslat *
		cos(o1->lng - o2->lng);
	a = atan2(leg(a), a);
	if(a < 0)
		a = -a;
	return a;
}

/*
 * calculate true angle in radians
 * from location 1 to location 2
 */
double
angle(Loc *o1, Loc *o2)
{
	double a;

	a = sphere(PIO2 - o2->lat, PIO2 - o1->lat, MISSING,
		UNKNOWN, MISSING, fabs(o1->lng-o2->lng));
	if(o1->lng > o2->lng)
		a = TWOPI - a;
	return a;
}

/*
 * normalize a number between
 * 0 and d
 */
double
norm(double a, double d)
{

	while(a < 0)
		a += d;
	while(a >= d)
		a -= d;
	return a;
}

/*
 * give a new location
 * at true angle az radians and
 * distance d nautical miles.
 */
Loc
dmecalc(Loc *l, double az, double d)
{
	double t, u;
	Loc newl;

	if(d < 1.0e-3)
		return *l;
	d = d/RADE;

	t = fabs(az);
	if(t < 1.0e-10) {
		newl.lat = l->lat + d;
		newl.lng = l->lng;
		goto out;
	}
	t = fabs(PI - t);
	if(t < 1.0e-10) {
		newl.lat = l->lat - d;
		newl.lng = l->lng;
		goto out;
	}

	u = norm(PIO2 - l->lat, TWOPI);
	t = sphere(UNKNOWN, d, u,
		az, MISSING, MISSING);
	newl.lat = PIO2-t;
	t = sphere(t, d, u,
		az, UNKNOWN, MISSING);
	if(az > PI)
		t = l->lng - t;
	else
		t = l->lng + t;
	newl.lng = t;

out:
	newl.lat = norm(newl.lat, TWOPI);
	newl.lng = norm(newl.lng, TWOPI);
	newl.sinlat = sin(newl.lat);
	newl.coslat = cos(newl.lat);

	return newl;
}

void
getloc(Loc *l, char *p)
{

	l->lat = 0;
	l->lng = 0;
	if(p) {
		l->lat = getlat(p);
		p = colskip(p);
		if(p)
			l->lng = getlat(p);
	}
	l->sinlat = sin(l->lat);
	l->coslat = cos(l->lat);
}

/*
 * parse input as lat or lng
 */
double
getlat(char *p)
{
	double deg, min, sec, d;
	char *ap;
	Rune rune;
	int c;

	deg = 0;
	min = 0;
	sec = 0;
	ap = p;

loop:
	c = *p++;
	if(c == ' ')
		goto loop;
	d = qatof(p-1);
	while(c == '.' || (c >= '0' && c <= '9'))
		c = *p++;

	switch(c) {
	case ''':
		min = d;
		goto loop;

	case '"':
		sec = d;
		goto loop;

	case '#':
		deg = d;
		goto loop;

	case 'N':
	case 'E':
		d = (deg + min/60 + sec/3600) / DEGREE(1);
		return norm(d, TWOPI);

	case 'S':
	case 'W':
		d = (deg + min/60 + sec/3600) / DEGREE(1);
		return norm(-d, TWOPI);
	}

	p--;
	p += chartorune(&rune, p);
	if(rune == L'°') {
		deg = d;
		goto loop;
	}

	fprint(2, "getlat %s\n", ap);
	return 0;
}

double
qatof(char *p)
{
	long l, g;
	double d;
	int c;

	l = 0;
	for(;;) {
		c = *p++;
		if(c < '0' || c > '9')
			break;
		l = l*10 + (c-'0');
	}
	d = l;
	if(c != '.')
		return d;

	g = 1;
	l = 0;
	for(;;) {
		c = *p++;
		if(c < '0' || c > '9')
			break;
		l = l*10 + (c-'0');
		g = g*10;
	}
	return d + (double)l/(double)g;
}

char*
colskip(char *p)
{

	if(p) {
		while(*p == ' ')
			p++;
		p = strchr(p, ' ');
		if(p)
			while(*p == ' ')
				p++;
	}
	return p;
}
